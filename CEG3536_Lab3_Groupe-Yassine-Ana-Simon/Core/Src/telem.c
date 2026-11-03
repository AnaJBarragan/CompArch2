/* ---------------------------------------------------------------------------
 * telem.c — télémétrie LPUART1 au niveau des registres (CMSIS, sans HAL)
 *           (CEG 3536, laboratoire 3, E10)
 *
 * LPUART1 : TX = PG7, RX = PG8 (AF8), 115 200 bauds, 8N1, vers le port COM
 * virtuel du ST-LINK (guide, p. 54-55). Le port G (broches 2 à 15) est alimenté
 * par VDDIO2 : PWR_CR2.IOSV doit être mis à 1 (RM0438, section PWR).
 * Horloge noyau = PCLK1 = 4 MHz (RCC_CCIPR1.LPUART1SEL = 00) :
 *   BRR = 256 x fck / bauds (vérifier fck >= 3 x bauds).
 * FIFO activée (CR1.FIFOEN, 8 octets) : telem_step remplit la FIFO tant que
 * ISR.TXFNF est à 1, sans jamais attendre.
 *
 * File logicielle : telem_put accepte une ligne entière ou la refuse entière
 * (jamais de trame tronquée, E12) ; telem_step la vide dans la FIFO.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "telem.h"

#define TX_TAILLE 512u
static char     tx_file[TX_TAILLE];
static uint32_t tx_tete, tx_queue, tx_nombre, pertes;

void telem_init(uint32_t bauds)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR2 |= PWR_CR2_IOSV;                         /* alimentation du port G */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    RCC->CCIPR1 &= ~RCC_CCIPR1_LPUART1SEL;            /* 00 = PCLK1             */
    (void)RCC->APB1ENR2;

    /* PG7 (TX) : AFRL champ 7 = 8 ; PG8 (RX) : AFRH champ 0 = 8 ; MODER = 10 */
    GPIOG->AFR[0] = (GPIOG->AFR[0] & ~GPIO_AFRL_AFSEL7) | (8u << GPIO_AFRL_AFSEL7_Pos);
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~GPIO_AFRH_AFSEL8) | (8u << GPIO_AFRH_AFSEL8_Pos);
    GPIOG->MODER = (GPIOG->MODER & ~(GPIO_MODER_MODE7 | GPIO_MODER_MODE8))
                 | GPIO_MODER_MODE7_1 | GPIO_MODER_MODE8_1;
    GPIOG->OSPEEDR |= GPIO_OSPEEDR_OSPEED7_0;
    GPIOG->PUPDR = (GPIOG->PUPDR & ~GPIO_PUPDR_PUPD8) | GPIO_PUPDR_PUPD8_0; /* RX tiré à 1 */

    /* LPUART1 : configuré UE = 0, puis activé.
     * BRR = 256 x fck / bauds, arrondi : (256 x 4 000 000 + 57 600) / 115 200
     * = 8889 (0x22B9). 256 x 4 000 000 = 1,024e9 tient sur 32 bits.
     * Condition de la RM0438 : fck (4 MHz) >= 3 x bauds (345 600) : respectée. */
    LPUART1->CR1 = 0u;                                /* UE = 0              */
    LPUART1->BRR = (256u * HORLOGE_HZ_C + bauds / 2u) / bauds;
    LPUART1->CR2 = 0u;                                /* 1 bit d'arrêt       */
    LPUART1->CR3 = 0u;                                /* sans contrôle de flux */
    LPUART1->CR1 = USART_CR1_FIFOEN                   /* FIFO de 8 octets    */
                 | USART_CR1_TE | USART_CR1_RE;       /* 8 bits, sans parité */
    LPUART1->CR1 |= USART_CR1_UE;                     /* activation          */

    tx_tete = tx_queue = tx_nombre = pertes = 0u;
}

/* Ajoute une ligne entière à la file d'émission, ou la refuse entière si la
 * place manque (les caractères refusés sont comptés dans pertes).          */
bool telem_put(const char *s)
{
    uint32_t n = 0u;
    while (s[n] != '\0') {
        n++;                                          /* longueur de la ligne */
    }
    if (n > TX_TAILLE - tx_nombre) {
        pertes += n;                                  /* ligne refusée entière */
        return false;
    }
    while (*s) {
        tx_file[tx_queue] = *s++;
        tx_queue = (tx_queue + 1u) % TX_TAILLE;
        tx_nombre++;
    }
    return true;
}

/* Appelée à chaque tick : transfère dans la FIFO matérielle (8 octets) tant
 * qu'elle a de la place (TXFNF = 1). Aucune attente : au plus 8 octets par
 * appel, soit 8 ko/s, plus que les ~300 o/s de la télémétrie.              */
void telem_step(void)
{
    while (tx_nombre > 0u && (LPUART1->ISR & USART_ISR_TXE_TXFNF)) {
        LPUART1->TDR = (uint8_t)tx_file[tx_tete];
        tx_tete = (tx_tete + 1u) % TX_TAILLE;
        tx_nombre--;
    }
}

/* Réception facultative d'un caractère de commande : -1 si rien n'est reçu.
 * Un débordement (ORE) bloquerait la réception : il est effacé.            */
int telem_rx(void)
{
    uint32_t isr = LPUART1->ISR;
    if (isr & USART_ISR_ORE) {
        LPUART1->ICR = USART_ICR_ORECF;
    }
    if (isr & USART_ISR_RXNE_RXFNE) {
        return (int)(LPUART1->RDR & 0xFFu);
    }
    return -1;
}

uint32_t telem_pertes(void) { return pertes; }

/* ---- formatage entier [FOURNI] ---------------------------------------- */
char *telem_fmt_u32(char *p, uint32_t v)
{
    char tmp[11];
    int n = 0;
    do { tmp[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n) *p++ = tmp[--n];
    *p = '\0';
    return p;
}

char *telem_fmt_i32(char *p, int32_t v)
{
    if (v < 0) { *p++ = '-'; v = -v; }
    return telem_fmt_u32(p, (uint32_t)v);
}

/* ---- trames ------------------------------------------------------------ */
void telem_entete(void)
{
    telem_put("H,ticks_ms,etat,sens,consigne_pct,pwm_pct,temp_c_x10,vitesse_rpm_x100,nb_evenements,evq_debordements\r\n");
}

/* Ajoute ",<v>" à la ligne en construction ; retourne la nouvelle fin.   */
static char *champ_u32(char *p, uint32_t v)
{
    *p++ = ',';
    return telem_fmt_u32(p, v);
}

/* Ligne T,<ticks_ms>,<etat>,<sens>,<consigne_pct>,<pwm_pct>,<temp_c_x10>,
 * <vitesse_rpm_x100>,<nb_evenements>,<evq_debordements>\r\n (E10).
 * Longueur maximale : 2 + 9 x 12 + 2 = 112 caractères < 128.
 * Q16.16 -> centièmes : (q16 x 100 + 32768) >> 16, calcul sur 64 bits car
 * q16 x 100 peut dépasser 2^32.                                           */
void telem_trame(const etat_vehicule_t *v, int32_t temp_c_x10,
                 uint32_t nb_evenements, uint32_t debordements)
{
    char ligne[128];
    char *p = ligne;
    uint32_t rpm_x100 = (uint32_t)(((uint64_t)v->vitesse_rpm_q16 * 100u + 32768u) >> 16);

    *p++ = 'T';
    p = champ_u32(p, ticks_ms);
    p = champ_u32(p, v->etat);
    p = champ_u32(p, v->sens);
    p = champ_u32(p, v->consigne_pct);
    p = champ_u32(p, v->pwm_pct);
    *p++ = ',';
    p = telem_fmt_i32(p, temp_c_x10);
    p = champ_u32(p, rpm_x100);
    p = champ_u32(p, nb_evenements);
    p = champ_u32(p, debordements);
    *p++ = '\r';
    *p++ = '\n';
    *p = '\0';
    (void)telem_put(ligne);       /* refusée entière si la file est pleine */
}

/* Ligne d'événement E,<t_ms>,<nom>\r\n (E9 : E-Stop consigné). Ignorée par
 * telemetrie.py, qui ne vérifie que les lignes H et T.                    */
void telem_evenement(const char *nom, uint32_t t_ms)
{
    char ligne[48];
    char *p = ligne;
    *p++ = 'E';
    p = champ_u32(p, t_ms);
    *p++ = ',';
    while (*nom && p < &ligne[sizeof ligne - 3]) {
        *p++ = *nom++;
    }
    *p++ = '\r';
    *p++ = '\n';
    *p = '\0';
    (void)telem_put(ligne);
}
