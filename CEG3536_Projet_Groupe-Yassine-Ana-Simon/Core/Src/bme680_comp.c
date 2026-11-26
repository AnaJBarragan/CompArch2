/* ---------------------------------------------------------------------------
 * bme680_comp.c — formules de compensation entières du capteur BME680
 *                 (CEG 3536, laboratoire 4, E7) — FOURNI, NE PAS MODIFIER
 * Transcription des formules en arithmétique entière de la fiche technique
 * Bosch Sensortec BME680 et du pilote de référence du fabricant (BME680
 * sensor API, Copyright (c) Bosch Sensortec GmbH, licence BSD-3-Clause).
 * Les décalages à droite de valeurs négatives sont arithmétiques avec GCC
 * (cible et hôte), comme le suppose le pilote de référence.
 * ------------------------------------------------------------------------- */
#include "bme680_comp.h"

static uint16_t u16(const uint8_t *b, uint32_t poids_faible)
{
    return (uint16_t)((uint32_t)b[poids_faible] | ((uint32_t)b[poids_faible + 1u] << 8));
}

void bme680_coef_decoder(const uint8_t b[BME680_N_COEF], bme680_coef_t *c)
{
    c->par_t2  = (int16_t)u16(b, 1u);
    c->par_t3  = (int8_t)b[3];
    c->par_p1  = u16(b, 5u);
    c->par_p2  = (int16_t)u16(b, 7u);
    c->par_p3  = (int8_t)b[9];
    c->par_p4  = (int16_t)u16(b, 11u);
    c->par_p5  = (int16_t)u16(b, 13u);
    c->par_p7  = (int8_t)b[15];
    c->par_p6  = (int8_t)b[16];
    c->par_p8  = (int16_t)u16(b, 19u);
    c->par_p9  = (int16_t)u16(b, 21u);
    c->par_p10 = b[23];
    c->par_h2  = (uint16_t)(((uint32_t)b[25] << 4) | ((uint32_t)b[26] >> 4));
    c->par_h1  = (uint16_t)(((uint32_t)b[27] << 4) | ((uint32_t)b[26] & 0x0Fu));
    c->par_h3  = (int8_t)b[28];
    c->par_h4  = (int8_t)b[29];
    c->par_h5  = (int8_t)b[30];
    c->par_h6  = b[31];
    c->par_h7  = (int8_t)b[32];
    c->par_t1  = u16(b, 33u);
    c->t_fine  = 0;
}

void bme680_brut_decoder(const uint8_t d[BME680_N_BRUT],
                         uint32_t *adc_p, uint32_t *adc_t, uint32_t *adc_h)
{
    *adc_p = ((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | ((uint32_t)d[2] >> 4);
    *adc_t = ((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | ((uint32_t)d[5] >> 4);
    *adc_h = ((uint32_t)d[6] << 8) | (uint32_t)d[7];
}

int32_t bme680_temperature_c100(bme680_coef_t *c, uint32_t adc_t)
{
    int64_t var1, var2, var3;

    var1 = (int64_t)((int32_t)adc_t >> 3) - (int64_t)((int32_t)c->par_t1 * 2);
    var2 = (var1 * (int32_t)c->par_t2) >> 11;
    var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
    var3 = (var3 * ((int32_t)c->par_t3 * 16)) >> 14;
    c->t_fine = (int32_t)(var2 + var3);
    return (int32_t)(((int64_t)c->t_fine * 5 + 128) >> 8);
}

uint32_t bme680_pression_pa(const bme680_coef_t *c, uint32_t adc_p)
{
    int32_t var1, var2, var3, p;

    var1 = (c->t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)c->par_p6) >> 2;
    var2 = var2 + ((var1 * (int32_t)c->par_p5) * 2);
    var2 = (var2 >> 2) + ((int32_t)c->par_p4 * 65536);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)c->par_p3 * 32)) >> 3)
         + (((int32_t)c->par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (int32_t)c->par_p1) >> 15;
    if (var1 == 0) {
        return 0u;                              /* coefficients non lus      */
    }
    p = 1048576 - (int32_t)adc_p;
    p = (int32_t)((uint32_t)(p - (var2 >> 12)) * 3125u);
    if (p >= 0x40000000) {
        p = (p / var1) * 2;
    } else {
        p = (p * 2) / var1;
    }
    var1 = ((int32_t)c->par_p9 * (((p >> 3) * (p >> 3)) >> 13)) >> 12;
    var2 = ((p >> 2) * (int32_t)c->par_p8) >> 13;
    var3 = (int32_t)(((int64_t)(p >> 8) * (p >> 8) * (p >> 8) * (int32_t)c->par_p10) >> 17);
    p = p + ((var1 + var2 + var3 + ((int32_t)c->par_p7 * 128)) >> 4);
    return (uint32_t)p;
}

uint32_t bme680_humidite_x1000(const bme680_coef_t *c, uint32_t adc_h)
{
    int32_t var1, var2, var3, var4, var5, var6, t, h;

    t = (int32_t)(((int64_t)c->t_fine * 5 + 128) >> 8);
    var1 = ((int32_t)adc_h - ((int32_t)c->par_h1 * 16))
         - (((t * (int32_t)c->par_h3) / 100) >> 1);
    var2 = ((int32_t)c->par_h2
            * (((t * (int32_t)c->par_h4) / 100)
               + (((t * ((t * (int32_t)c->par_h5) / 100)) >> 6) / 100)
               + (1 << 14))) >> 10;
    var3 = var1 * var2;
    var4 = (int32_t)c->par_h6 * 128;
    var4 = (var4 + ((t * (int32_t)c->par_h7) / 100)) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    h = (((var3 + var6) >> 10) * 1000) >> 12;
    if (h > 100000) {
        h = 100000;
    } else if (h < 0) {
        h = 0;
    }
    return (uint32_t)h;
}
