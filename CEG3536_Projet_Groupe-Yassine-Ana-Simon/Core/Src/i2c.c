/* ---------------------------------------------------------------------------
 * i2c.c — pilote I2C2 maître au niveau des registres (CMSIS, sans HAL)
 *         (CEG 3536, laboratoire 4, E1)
 *
 * Configuration (RM0438, section I2C ; énoncé 4.7) :
 *   1. horloges GPIOF (AHB2) et I2C2 (APB1) ; PF0 et PF1 en AF4, drain ouvert
 *   2. horloge noyau = PCLK1 = 4 MHz (RCC_CCIPR1.I2C2SEL = 00)
 *   3. CR1.PE = 0, TIMINGR = 0x00420F13, puis CR1.PE = 1
 *      PRESC = 0     -> t_PRESC = (0 + 1) / 4 MHz = 250 ns
 *      SCLL  = 0x13  -> t_SCLL  = (19 + 1) x 250 ns = 5,0 µs
 *      SCLH  = 0x0F  -> t_SCLH  = (15 + 1) x 250 ns = 4,0 µs
 *      SDADEL = 2 (500 ns), SCLDEL = 4 (1,25 µs)
 *      période = 9,0 µs + délais de synchronisation (~ 2 à 3 cycles de
 *      I2CCLK sur chaque front + montée) -> un peu plus de 10 µs, soit 90 à
 *      100 kHz (à confirmer par la mesure DWT d'une écriture de 2 octets).
 *
 * Toutes les attentes passent par attendre() : elle surveille en même temps
 * le drapeau voulu, NACKF, BERR, ARLO et l'échéance I2C_DELAI_MS. Tous les
 * chemins de sortie passent par terminer() : drapeaux effacés, bus libéré.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab4.h"
#include "i2c.h"

#define I2C_TIMINGR_100K    0x00420F13u
#define I2C_NBYTES_MAX      255u

static uint32_t erreurs, transactions;
static uint32_t absent_actif, absent_adr, absent_debut, absent_duree;

/* Remet le périphérique dans un état connu : PE = 0 pendant au moins trois
 * cycles d'APB (RM0438 : « software reset »), ce qui relâche SCL et SDA et
 * efface les drapeaux internes, puis PE = 1.                               */
static void reinitialiser(void)
{
    I2C2->CR1 &= ~I2C_CR1_PE;
    (void)I2C2->CR1;                    /* lectures : délai de quelques cycles */
    (void)I2C2->CR1;
    (void)I2C2->CR1;
    I2C2->CR1 |= I2C_CR1_PE;
}

void i2c_init(void)
{
    /* 1. horloges et broches */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOFEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
    (void)RCC->APB1ENR1;
    GPIOF->OTYPER |= GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1;            /* drain ouvert */
    GPIOF->PUPDR  &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);       /* tirage externe */
    GPIOF->AFR[0]  = (GPIOF->AFR[0] & ~(GPIO_AFRL_AFSEL0 | GPIO_AFRL_AFSEL1))
                   | (4u << GPIO_AFRL_AFSEL0_Pos) | (4u << GPIO_AFRL_AFSEL1_Pos);
    GPIOF->MODER   = (GPIOF->MODER & ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1))
                   | GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1;      /* 10 = AF       */

    /* 2. horloge noyau */
    RCC->CCIPR1 &= ~RCC_CCIPR1_I2C2SEL;                            /* 00 = PCLK1    */

    /* 3. TIMINGR (modifiable seulement avec PE = 0), puis activation */
    I2C2->CR1 &= ~I2C_CR1_PE;
    I2C2->TIMINGR = I2C_TIMINGR_100K;
    I2C2->CR1 |= I2C_CR1_PE;

    erreurs = transactions = 0u;
    absent_actif = 0u;
}

/* essai E12 : module simulé absent pendant duree_ms */
void i2c_simuler_absence(uint8_t adr7, uint32_t duree_ms)
{
    absent_adr = adr7;
    absent_debut = ticks_ms;
    absent_duree = duree_ms;
    absent_actif = 1u;
}

static bool absence_simulee(uint8_t adr7)
{
    if (absent_actif && (ticks_ms - absent_debut) >= absent_duree) {
        absent_actif = 0u;
    }
    return absent_actif && adr7 == absent_adr;
}

/* toute opération en échec passe par ici */
static i2c_statut_t echec(i2c_statut_t statut)
{
    erreurs++;
    return statut;
}

/* Attend que le drapeau (bit de ISR) passe à 1. Retourne I2C_OK, ou l'erreur
 * détectée pendant l'attente : NACKF -> I2C_NACK ; BERR ou ARLO -> I2C_BUS ;
 * plus de I2C_DELAI_MS écoulées depuis t0 -> I2C_DELAI.                     */
static i2c_statut_t attendre(uint32_t drapeau, uint32_t t0)
{
    for (;;) {
        uint32_t isr = I2C2->ISR;
        if (isr & drapeau) {
            return I2C_OK;
        }
        if (isr & I2C_ISR_NACKF) {
            return I2C_NACK;
        }
        if (isr & (I2C_ISR_BERR | I2C_ISR_ARLO)) {
            return I2C_BUS;
        }
        if ((ticks_ms - t0) > I2C_DELAI_MS) {
            return I2C_DELAI;
        }
    }
}

/* Fin commune de toutes les opérations : remet le bus et les drapeaux dans
 * l'état de repos, compte l'erreur s'il y a lieu, retourne le statut.       */
static i2c_statut_t terminer(i2c_statut_t s)
{
    if (s == I2C_NACK) {
        /* En mode maître, le matériel émet lui-même STOP après un NACK
         * (RM0438, « master transmitter »), avec ou sans AUTOEND : on
         * attend STOPF, borné ; s'il ne vient pas, on force le repos.       */
        I2C2->ICR = I2C_ICR_NACKCF;
        if (attendre(I2C_ISR_STOPF, ticks_ms) != I2C_OK) {
            reinitialiser();            /* STOP jamais vu : on force le repos */
        }
    } else if (s == I2C_BUS) {
        I2C2->ICR = I2C_ICR_BERRCF | I2C_ICR_ARLOCF;
        reinitialiser();
    } else if (s == I2C_DELAI) {
        reinitialiser();                /* SCL ou SDA tenu : on relâche tout  */
    }
    I2C2->ICR = I2C_ICR_STOPCF;
    I2C2->ISR = I2C_ISR_TXE;            /* vide TXDR s'il reste un octet      */
    I2C2->CR2 = 0u;
    return (s == I2C_OK) ? I2C_OK : echec(s);
}

/* Lance un START : adresse, sens, nombre d'octets, AUTOEND ou non. */
static void demarrer(uint8_t adr7, uint32_t lecture, uint32_t n, uint32_t autoend)
{
    I2C2->CR2 = ((uint32_t)adr7 << 1)
              | (lecture ? I2C_CR2_RD_WRN : 0u)
              | (n << I2C_CR2_NBYTES_Pos)
              | (autoend ? I2C_CR2_AUTOEND : 0u)
              | I2C_CR2_START;
}

/* Bus occupé au départ (BUSY) : attente bornée, sinon remise à zéro. */
static i2c_statut_t bus_libre(void)
{
    uint32_t t0 = ticks_ms;
    while (I2C2->ISR & I2C_ISR_BUSY) {
        if ((ticks_ms - t0) > I2C_DELAI_MS) {
            return I2C_DELAI;
        }
    }
    return I2C_OK;
}

static i2c_statut_t emettre(const uint8_t *d, uint32_t n, uint32_t t0)
{
    for (uint32_t i = 0u; i < n; i++) {
        i2c_statut_t s = attendre(I2C_ISR_TXIS, t0);
        if (s != I2C_OK) {
            return s;
        }
        I2C2->TXDR = d[i];
    }
    return I2C_OK;
}

static i2c_statut_t recevoir(uint8_t *d, uint32_t n, uint32_t t0)
{
    for (uint32_t i = 0u; i < n; i++) {
        i2c_statut_t s = attendre(I2C_ISR_RXNE, t0);
        if (s != I2C_OK) {
            return s;
        }
        d[i] = (uint8_t)I2C2->RXDR;
    }
    return I2C_OK;
}

i2c_statut_t i2c_ecrire(uint8_t adr7, const uint8_t *d, uint32_t n)
{
    i2c_statut_t s;
    uint32_t t0 = ticks_ms;

    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    if (n > I2C_NBYTES_MAX) {
        return echec(I2C_BUS);
    }
    s = bus_libre();
    if (s == I2C_OK) {
        demarrer(adr7, 0u, n, 1u);
        s = emettre(d, n, t0);          /* n = 0 : rien à émettre (balayage)  */
    }
    if (s == I2C_OK) {
        s = attendre(I2C_ISR_STOPF, t0);
    }
    return terminer(s);
}

i2c_statut_t i2c_lire(uint8_t adr7, uint8_t *d, uint32_t n)
{
    i2c_statut_t s;
    uint32_t t0 = ticks_ms;

    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    if (n == 0u || n > I2C_NBYTES_MAX) {
        return echec(I2C_BUS);
    }
    s = bus_libre();
    if (s == I2C_OK) {
        demarrer(adr7, 1u, n, 1u);
        s = recevoir(d, n, t0);
    }
    if (s == I2C_OK) {
        s = attendre(I2C_ISR_STOPF, t0);
    }
    return terminer(s);
}

i2c_statut_t i2c_ecrire_lire(uint8_t adr7, const uint8_t *e, uint32_t ne,
                             uint8_t *l, uint32_t nl)
{
    i2c_statut_t s;
    uint32_t t0 = ticks_ms;

    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    if (ne == 0u || nl == 0u || ne > I2C_NBYTES_MAX || nl > I2C_NBYTES_MAX) {
        return echec(I2C_BUS);
    }
    s = bus_libre();
    if (s == I2C_OK) {
        demarrer(adr7, 0u, ne, 0u);     /* phase 1 : écriture, SANS AUTOEND   */
        s = emettre(e, ne, t0);
    }
    if (s == I2C_OK) {
        s = attendre(I2C_ISR_TC, t0);   /* TC : octets émis, bus gardé        */
    }
    if (s == I2C_OK) {
        demarrer(adr7, 1u, nl, 1u);     /* phase 2 : redémarrage en lecture   */
        s = recevoir(l, nl, t0);
    }
    if (s == I2C_OK) {
        s = attendre(I2C_ISR_STOPF, t0);
    }
    return terminer(s);
}

/* Ajout de l'équipe : sonde UNE adresse (écriture de zéro octet) sans compter
 * de transaction ni d'erreur. Sert au balayage étalé sur plusieurs ticks
 * (commande « i ») : une transaction par tick, comme l'exige E11.           */
bool i2c_sonder(uint8_t adr7)
{
    uint32_t e = erreurs, t = transactions;
    bool present = (i2c_ecrire(adr7, (const uint8_t *)0, 0u) == I2C_OK);

    erreurs = e;
    transactions = t;
    return present;
}

/* [FOURNI] balayage : une écriture de zéro octet par adresse. Les NACK du
 * balayage ne sont pas des erreurs : les compteurs sont restaurés.           */
uint32_t i2c_scan(uint8_t *adresses, uint32_t max)
{
    uint32_t trouvees = 0u;
    uint32_t e = erreurs, t = transactions;

    for (uint8_t a = 0x08u; a <= 0x77u; a++) {
        if (i2c_ecrire(a, (const uint8_t *)0, 0u) == I2C_OK) {
            if (trouvees < max) {
                adresses[trouvees] = a;
            }
            trouvees++;
        }
    }
    erreurs = e;
    transactions = t;
    return trouvees;
}

uint32_t i2c_erreurs(void)      { return erreurs; }
uint32_t i2c_transactions(void) { return transactions; }
