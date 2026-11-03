/* ---------------------------------------------------------------------------
 * adc_dma.c — pilote ADC1 + DMA1 au niveau des registres (CMSIS, sans HAL)
 *             (CEG 3536, laboratoire 3, E4) — SQUELETTE À COMPLÉTER
 *
 * Séquence (RM0438, sections ADC, DMA, DMAMUX ; énoncé 4.8) :
 *   1. horloges ADC (AHB2), DMA1 et DMAMUX1 (AHB1), GPIOC ; PC0, PC1 analogiques  [FOURNI]
 *   2. horloge de l'ADC : ADC12_COMMON->CCR, CKMODE = 01 (HCLK/1, synchrone)
 *   3. sortie du mode basse consommation (DEEPPWD = 0), régulateur (ADVREGEN,
 *      attendre > 20 µs : delay_ms(1)), étalonnage (ADCAL puis attendre sa
 *      retombée, ADEN = 0 pendant l'étalonnage), activation (ISR = ADRDY (w1c),
 *      ADEN, attendre ISR.ADRDY)
 *   4. CFGR : RES = 00 (12 bits), CONT, DMAEN, DMACFG (circulaire), OVRMOD ;
 *      SMPR1 : SMP1 = SMP2 = 111 (640,5 cycles) ; SQR1 : L = 1 (2 conversions),
 *      SQ1 = 1 (IN1 = PC0), SQ2 = 2 (IN2 = PC1)
 *   5. DMAMUX1_Channel0->CCR = 5 (requête ADC1) ; DMA1_Channel1 : CPAR =
 *      &ADC1->DR, CM0AR = tampon (registre nommé CM0AR sur STM32L5), CNDTR =
 *      ADC_N_VOIES x n_par_voie, CCR = MINC | PSIZE_0 | MSIZE_0 | CIRC
 *      (DIR = 0 : périphérique vers mémoire), puis EN
 *   6. ADSTART
 * Débit par voie = 4 MHz / (2 x (640,5 + 12,5)) ~ 3,06 kHz ; un tour du tampon
 * (16 échantillons par voie) dure ~ 5,2 ms (vérifié par adc_dma_tours, T4).
 * Vérification au débogueur : ADC1->CR = 0x1000000D (ADVREGEN, ADSTART, ADEN),
 * ISR.ADRDY = 1, DMA1_Channel1->CNDTR qui décroît et se recharge, tampon qui
 * évolue quand on bouge le potentiomètre.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab3.h"
#include "adc_dma.h"

static uint32_t tours;

void adc_dma_init(volatile uint16_t *tampon, uint32_t n_par_voie)
{
    (void)tampon; (void)n_par_voie;      /* à retirer une fois les étapes écrites */

    /* 1. horloges et broches analogiques [FOURNI] */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_ADCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    (void)RCC->AHB1ENR;
    GPIOC->MODER |= GPIO_MODER_MODE0 | GPIO_MODER_MODE1;      /* 11 = analogique */
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);

    /* 2. horloge ADC — À COMPLÉTER */

    /* 3. régulateur, étalonnage, activation — À COMPLÉTER */

    /* 4. configuration de la conversion — À COMPLÉTER */

    /* 5. DMAMUX et DMA — À COMPLÉTER */
    tours = 0u;

    /* 6. départ — À COMPLÉTER */
}

void adc_dma_step(void)
{
    if (DMA1->ISR & DMA_ISR_TCIF1) {                          /* tour complet  */
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        tours++;
    }
}

uint32_t adc_dma_tours(void)
{
    return tours;
}
