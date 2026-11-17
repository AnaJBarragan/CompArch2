/* ---------------------------------------------------------------------------
 * bme680_comp.h — formules de compensation entières du capteur BME680
 *                 (CEG 3536, laboratoire 4, E7) — FOURNI, NE PAS MODIFIER
 * Module sans matériel : aucun registre, aucun en-tête CMSIS (compilable sur
 * hôte). Source : fiche technique Bosch Sensortec BME680 (calcul des grandeurs
 * en arithmétique entière) et pilote de référence du fabricant (BME680 sensor
 * API, licence BSD-3-Clause), à citer dans le rapport.
 * ------------------------------------------------------------------------- */
#ifndef BME680_COMP_H
#define BME680_COMP_H

#include <stdint.h>

#define BME680_N_COEF   41u     /* 25 octets depuis 0x89, puis 16 depuis 0xE1 */
#define BME680_N_BRUT   8u      /* registres 0x1F à 0x26                      */

typedef struct {
    uint16_t par_t1;  int16_t par_t2;  int8_t  par_t3;
    uint16_t par_p1;  int16_t par_p2;  int8_t  par_p3;  int16_t par_p4;
    int16_t  par_p5;  int8_t  par_p6;  int8_t  par_p7;  int16_t par_p8;
    int16_t  par_p9;  uint8_t par_p10;
    uint16_t par_h1;  uint16_t par_h2; int8_t  par_h3;  int8_t  par_h4;
    int8_t   par_h5;  uint8_t par_h6;  int8_t  par_h7;
    int32_t  t_fine;            /* calculé par bme680_temperature_c100        */
} bme680_coef_t;

/* brut[0..24] = registres 0x89 à 0xA1 ; brut[25..40] = registres 0xE1 à 0xF0 */
void bme680_coef_decoder(const uint8_t brut[BME680_N_COEF], bme680_coef_t *c);

/* d[0..7] = registres 0x1F à 0x26 : pression et température sur 20 bits,
 * humidité sur 16 bits                                                        */
void bme680_brut_decoder(const uint8_t d[BME680_N_BRUT],
                         uint32_t *adc_p, uint32_t *adc_t, uint32_t *adc_h);

/* Température en centièmes de degré Celsius (2345 = 23,45 °C).
 * À APPELER EN PREMIER : met c->t_fine à jour pour les deux autres formules.  */
int32_t  bme680_temperature_c100(bme680_coef_t *c, uint32_t adc_t);
/* Pression en pascals (100325 = 1003,25 hPa).                                 */
uint32_t bme680_pression_pa(const bme680_coef_t *c, uint32_t adc_p);
/* Humidité relative en millièmes de pour cent (45200 = 45,2 %), bornée à
 * [0, 100000].                                                                */
uint32_t bme680_humidite_x1000(const bme680_coef_t *c, uint32_t adc_h);

#endif /* BME680_COMP_H */
