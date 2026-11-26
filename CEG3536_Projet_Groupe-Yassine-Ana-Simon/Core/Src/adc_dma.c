/* ---------------------------------------------------------------------------
 * adc_dma.c — pilote ADC1 + DMA1 au niveau des registres (CMSIS, sans HAL)
 *             (CEG 3536, laboratoire 3) — CORRIGÉ
 *
 * Séquence (RM0438, section ADC) :
 *   1. horloges ADC (AHB2), DMA1 et DMAMUX1 (AHB1), GPIOC ; PC0, PC1 analogiques
 *   2. horloge de l'ADC : ADC_CCR.CKMODE = 01 (HCLK/1, synchrone), prescaler 1
 *   3. sortie du mode basse consommation (DEEPPWD = 0), régulateur (ADVREGEN),
 *      attente > 20 µs ; étalonnage (ADCAL) ; activation (ADEN, attente ADRDY)
 *   4. CFGR : 12 bits, conversion continue, DMA circulaire, écrasement en cas
 *      de débordement ; SMPR1 : 640,5 cycles sur IN1 et IN2 ; SQR1 : 2 conversions
 *   5. DMAMUX1 canal 0 <- requête 5 (ADC1) ; DMA1 canal 1 : périph -> mémoire,
 *      16 bits, incrément mémoire, circulaire, 2 x n_par_voie transferts
 *   6. ADSTART
 * Débit par voie = 4 MHz / (2 x (640,5 + 12,5)) ~ 3,06 kHz ; un tour du tampon
 * (16 échantillons par voie) dure ~ 5,2 ms.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab3.h"
#include "adc_dma.h"

static uint32_t tours;

void adc_dma_init(volatile uint16_t *tampon, uint32_t n_par_voie)
{
    /* 1. horloges et broches analogiques */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_ADCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    (void)RCC->AHB1ENR;
    GPIOC->MODER |= GPIO_MODER_MODE0 | GPIO_MODER_MODE1;      /* 11 = analogique */
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);

    /* 2. horloge ADC synchrone HCLK/1 */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) | ADC_CCR_CKMODE_0;

    /* 3. régulateur, étalonnage, activation */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delay_ms(1);                                              /* >= 20 µs      */
    ADC1->CR |= ADC_CR_ADCAL;                                 /* ADEN = 0 ici  */
    while (ADC1->CR & ADC_CR_ADCAL) { }
    ADC1->ISR = ADC_ISR_ADRDY;                                /* w1c           */
    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0u) { }

    /* 4. configuration de la conversion */
    ADC1->CFGR = (ADC1->CFGR & ~ADC_CFGR_RES)                 /* 12 bits       */
               | ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG | ADC_CFGR_OVRMOD;
    ADC1->SMPR1 = (ADC1->SMPR1 & ~(ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2))
                | ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2;            /* 111 = 640,5 cycles */
    ADC1->SQR1 = (1u << ADC_SQR1_L_Pos)                       /* L = 1 : 2 conversions */
               | (1u << ADC_SQR1_SQ1_Pos)                     /* IN1 = PC0     */
               | (2u << ADC_SQR1_SQ2_Pos);                    /* IN2 = PC1     */

    /* 5. DMAMUX et DMA */
    DMAMUX1_Channel0->CCR = 5u;                               /* requête ADC1  */
    DMA1_Channel1->CCR = 0u;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CM0AR = (uint32_t)tampon;         /* CM0AR sur STM32L5 */
    DMA1_Channel1->CNDTR = ADC_N_VOIES * n_par_voie;
    DMA1_Channel1->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0
                       | DMA_CCR_CIRC;                        /* DIR = 0       */
    DMA1->IFCR = DMA_IFCR_CGIF1;
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    tours = 0u;

    /* 6. départ */
    ADC1->CR |= ADC_CR_ADSTART;
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
