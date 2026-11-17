/* ---------------------------------------------------------------------------
 * i2c.c — pilote I2C2 maître au niveau des registres (CMSIS, sans HAL)
 *         (CEG 3536, laboratoire 4, E1) — SQUELETTE À COMPLÉTER
 *
 * Séquence (RM0438, section I2C ; énoncé 4.7) :
 *   1. horloges GPIOF (AHB2) et I2C2 (APB1) ; PF0 et PF1 en fonction
 *      alternative AF4, drain ouvert                                  [FOURNI]
 *   2. horloge noyau = PCLK1 = 4 MHz (RCC_CCIPR1.I2C2SEL = 00)        [FOURNI]
 *   3. I2C2 désactivé (CR1.PE = 0), TIMINGR pour 100 kHz, puis CR1 |= PE.
 *      À 4 MHz : PRESC = 0 (pas de 250 ns), SCLL = 0x13 (5 µs), SCLH = 0x0F
 *      (4 µs), SDADEL = 2, SCLDEL = 4. Calcul à refaire et à justifier.
 * Écriture de n octets :
 *   CR2 = (adr7 << 1) | (n << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START
 *   (RD_WRN = 0) ; pour chaque octet : attendre ISR.TXIS (ou NACKF), écrire
 *   TXDR ; attendre ISR.STOPF ; ICR = STOPCF.
 * Lecture de n octets : même séquence avec I2C_CR2_RD_WRN ; attendre ISR.RXNE,
 *   lire RXDR.
 * Écriture puis lecture : première phase SANS AUTOEND, attendre ISR.TC, puis
 *   nouveau START en lecture AVEC AUTOEND (redémarrage, pas d'arrêt entre les
 *   deux phases).
 * Erreurs : NACKF -> ICR = NACKCF, attendre STOPF, ICR = STOPCF, I2C_NACK ;
 *   BERR ou ARLO -> ICR = BERRCF | ARLOCF, I2C_BUS ; échéance -> I2C_DELAI.
 *   Après une échéance, remettre le périphérique dans un état connu
 *   (CR1.PE = 0 puis 1) pour ne jamais laisser le bus occupé.
 * Chaque attente compare ticks_ms à une échéance (I2C_DELAI_MS).
 * Vérification au débogueur : GPIOF->IDR bits 0 et 1 à 1 au repos ; I2C2->ISR
 * après une transaction (TXE = 1, BUSY = 0) ; I2C2->CR2 au moment d'un échec.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab4.h"
#include "i2c.h"

static uint32_t erreurs, transactions;
static uint32_t absent_actif, absent_adr, absent_debut, absent_duree;

void i2c_init(void)
{
    /* 1. horloges et broches [FOURNI] */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOFEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
    (void)RCC->APB1ENR1;
    GPIOF->OTYPER |= GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1;            /* drain ouvert */
    GPIOF->PUPDR  &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);       /* tirage externe */
    GPIOF->AFR[0]  = (GPIOF->AFR[0] & ~(GPIO_AFRL_AFSEL0 | GPIO_AFRL_AFSEL1))
                   | (4u << GPIO_AFRL_AFSEL0_Pos) | (4u << GPIO_AFRL_AFSEL1_Pos);
    GPIOF->MODER   = (GPIOF->MODER & ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1))
                   | GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1;      /* 10 = AF       */

    /* 2. horloge noyau [FOURNI] */
    RCC->CCIPR1 &= ~RCC_CCIPR1_I2C2SEL;                            /* 00 = PCLK1    */

    /* 3. TIMINGR et activation — À COMPLÉTER */

    erreurs = transactions = 0u;
    absent_actif = 0u;
}

/* [FOURNI] essai E12 : module simulé absent pendant duree_ms */
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

/* [FOURNI] toute opération en échec passe par ici */
static i2c_statut_t echec(i2c_statut_t statut)
{
    erreurs++;
    return statut;
}

i2c_statut_t i2c_ecrire(uint8_t adr7, const uint8_t *d, uint32_t n)
{
    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    /* À COMPLÉTER : séquence d'écriture, attentes bornées, traitement des
     * erreurs. n peut valoir 0 (adresse seule : aucune attente de TXIS).     */
    (void)d; (void)n;
    return echec(I2C_DELAI);
}

i2c_statut_t i2c_lire(uint8_t adr7, uint8_t *d, uint32_t n)
{
    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    /* À COMPLÉTER : séquence de lecture */
    (void)d; (void)n;
    return echec(I2C_DELAI);
}

i2c_statut_t i2c_ecrire_lire(uint8_t adr7, const uint8_t *e, uint32_t ne,
                             uint8_t *l, uint32_t nl)
{
    transactions++;
    if (absence_simulee(adr7)) {
        return echec(I2C_NACK);
    }
    /* À COMPLÉTER : écriture sans AUTOEND, attente de TC, redémarrage en lecture */
    (void)e; (void)ne; (void)l; (void)nl;
    return echec(I2C_DELAI);
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
