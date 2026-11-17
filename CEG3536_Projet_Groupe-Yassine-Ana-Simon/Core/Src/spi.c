/* ---------------------------------------------------------------------------
 * spi.c — pilote SPI1 maître au niveau des registres (CMSIS, sans HAL)
 *         (CEG 3536, laboratoire 4, E7) — SQUELETTE À COMPLÉTER
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

void spi_init(void)
{
    /* À COMPLÉTER : étapes 1 à 5 */
}

void spi_echange(const uint8_t *emis, uint8_t *recus, uint32_t n)
{
    /* À COMPLÉTER */
    (void)emis; (void)recus; (void)n;
}
