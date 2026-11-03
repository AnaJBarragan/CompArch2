/* ---------------------------------------------------------------------------
 * adc_dma.h — pilote ADC1 (IN1 = PC0, IN2 = PC1) avec DMA1 circulaire
 *             (CEG 3536, laboratoire 3, E4)
 * ------------------------------------------------------------------------- */
#ifndef ADC_DMA_H
#define ADC_DMA_H

#include <stdint.h>

/* tampon : n_par_voie échantillons pour chacune des ADC_N_VOIES voies,
 * entrelacés (IN1, IN2, IN1, IN2, ...). Conversion continue, DMA circulaire. */
void     adc_dma_init(volatile uint16_t *tampon, uint32_t n_par_voie);
void     adc_dma_step(void);          /* à appeler chaque tick : compte les tours du tampon */
uint32_t adc_dma_tours(void);         /* nombre de remplissages complets du tampon        */

#endif /* ADC_DMA_H */
