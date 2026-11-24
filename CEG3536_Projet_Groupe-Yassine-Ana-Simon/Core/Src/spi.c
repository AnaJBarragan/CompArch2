/* ---------------------------------------------------------------------------
 * spi.c — pilote SPI1 maître au niveau des registres (CMSIS, sans HAL)
 *         (CEG 3536, laboratoire 4, E7) — complété par l'équipe
 *
 * Séquence (RM0438, section SPI ; énoncé 4.7) :
 *   1. horloges : RCC_AHB2ENR (GPIOAEN, GPIODEN), RCC_APB2ENR (SPI1EN) ;
 *   2. PA5, PA6, PA7 : MODER = 10, AFRL champs 5, 6 et 7 = 5 (AF5) ;
 *      PD14 : niveau haut (BSRR) PUIS sortie (MODER = 01), pour ne jamais
 *      sélectionner le capteur par accident ;
 *   3. CR1 = MSTR | SSM | SSI | BR = 001 (PCLK2 / 4 = 1 MHz), CPOL = CPHA = 0 ;
 *   4. CR2 = DS = 0111 (8 bits) | FRXTH (RXNE dès un octet reçu) ;
 *   5. CR1 |= SPE.
 * Échange d'un octet : attendre SR.TXE, écrire l'octet dans DR par un accès de
 * 8 bits (*(volatile uint8_t *)&SPI1->DR), attendre SR.RXNE, lire DR de la
 * même façon. PD14 passe à 0 avant le premier octet et revient à 1 après le
 * dernier, une fois SR.BSY retombé.
 * Attentes : bornées (compteur de boucle ou ticks_ms) — un SPI mal configuré
 * ne doit pas bloquer la boucle principale.
 * Vérification au débogueur : SPI1->CR1, CR2, SR ; GPIOD->ODR bit 14 ; octet
 * reçu en lisant l'identifiant du capteur (env.c).
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab4.h"
#include "spi.h"

#define SPI_BOUCLES_MAX     2000u   /* ~ 2 à 3 ms à 4 MHz : bien plus qu'un octet */

static uint32_t erreurs;

void spi_init(void)
{
    /* 1. horloges */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIODEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    /* 2. PD14 : niveau haut AVANT le passage en sortie (capteur non choisi) */
    GPIOD->BSRR  = GPIO_BSRR_BS14;
    GPIOD->OTYPER &= ~GPIO_OTYPER_OT14;                         /* push-pull */
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODE14) | GPIO_MODER_MODE14_0;  /* 01 */
    /*    PA5 (SCK), PA6 (MISO), PA7 (MOSI) : AF5 */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(GPIO_AFRL_AFSEL5 | GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7))
                  | (5u << GPIO_AFRL_AFSEL5_Pos) | (5u << GPIO_AFRL_AFSEL6_Pos)
                  | (5u << GPIO_AFRL_AFSEL7_Pos);
    GPIOA->MODER  = (GPIOA->MODER & ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7))
                  | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1;   /* 10 */

    /* 3. maître, NSS logiciel (SSM = SSI = 1), BR = 001 : 4 MHz / 4 = 1 MHz,
     *    mode 0 (CPOL = CPHA = 0), MSB en premier                            */
    SPI1->CR1 = 0u;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
    /* 4. octets de 8 bits (DS = 0111), RXNE dès 8 bits reçus (FRXTH)          */
    SPI1->CR2 = (7u << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;
    /* 5. activation */
    SPI1->CR1 |= SPI_CR1_SPE;
    erreurs = 0u;
}

/* Attend que (SR & masque) == valeur, au plus SPI_BOUCLES_MAX tours. */
static bool attendre(uint32_t masque, uint32_t valeur)
{
    for (uint32_t i = 0u; i < SPI_BOUCLES_MAX; i++) {
        if ((SPI1->SR & masque) == valeur) {
            return true;
        }
    }
    return false;
}

void spi_echange(const uint8_t *emis, uint8_t *recus, uint32_t n)
{
    volatile uint8_t *dr8 = (volatile uint8_t *)&SPI1->DR;  /* accès de 8 bits */
    bool ok = true;

    /* reste d'un échange interrompu : on vide la FIFO de réception (bornée) */
    for (uint32_t i = 0u; i < 4u && (SPI1->SR & SPI_SR_RXNE); i++) {
        (void)*dr8;
    }
    GPIOD->BSRR = GPIO_BSRR_BR14;                    /* sélection : PD14 = 0 */
    for (uint32_t i = 0u; i < n && ok; i++) {
        ok = attendre(SPI_SR_TXE, SPI_SR_TXE);
        if (ok) {
            *dr8 = emis ? emis[i] : 0x00u;
            ok = attendre(SPI_SR_RXNE, SPI_SR_RXNE);
        }
        if (ok) {
            uint8_t b = *dr8;
            if (recus) {
                recus[i] = b;
            }
        }
    }
    if (ok) {
        ok = attendre(SPI_SR_BSY, 0u);               /* dernier bit sorti    */
    }
    GPIOD->BSRR = GPIO_BSRR_BS14;                    /* fin : PD14 = 1       */
    if (!ok) {
        erreurs++;
    }
}

uint32_t spi_erreurs(void) { return erreurs; }
