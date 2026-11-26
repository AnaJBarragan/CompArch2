/* ---------------------------------------------------------------------------
 * acq.c — objet d'acquisition d'une voie analogique (CEG 3536, lab. 3)
 *         — CORRIGÉ (version 2 : hystérésis et zone morte configurables)
 * Aucun registre : les échantillons viennent d'un tampon mémoire (DMA).
 * Toutes les opérations en entiers 32 bits.
 * ------------------------------------------------------------------------- */
#include "acq.h"

#define ACQ_PLEINE_ECHELLE 4095
#define ACQ_VREF_MV        3300

void acq_init(acq_t *a, const volatile uint16_t *src, uint32_t pas, uint32_t n_moy,
              int32_t gain_num, int32_t gain_den, int32_t decalage)
{
    a->src = src;
    a->pas = (pas == 0u) ? 1u : pas;
    a->n_moy = (n_moy == 0u) ? 1u : n_moy;
    a->gain_num = gain_num;
    a->gain_den = (gain_den == 0) ? 1 : gain_den;
    a->decalage = decalage;
    a->hysteresis = 0;
    a->zone_morte = 0;
    a->v_min = 0;
    a->v_max = 0;
    a->derniere = 0;
    a->initialisee = 0u;
}

void acq_config_filtre(acq_t *a, int32_t hysteresis, int32_t zone_morte,
                       int32_t v_min, int32_t v_max)
{
    a->hysteresis = hysteresis;
    a->zone_morte = zone_morte;
    a->v_min = v_min;
    a->v_max = v_max;
    a->initialisee = 0u;      /* la prochaine valeur est prise telle quelle   */
}

uint32_t acq_brut(const acq_t *a)
{
    uint32_t somme = 0u;
    for (uint32_t i = 0u; i < a->n_moy; i++) {
        somme += a->src[i * a->pas];          /* lecture 16 bits atomique      */
    }
    return (somme + a->n_moy / 2u) / a->n_moy;
}

uint32_t acq_mv(const acq_t *a)
{
    return (acq_brut(a) * ACQ_VREF_MV + ACQ_PLEINE_ECHELLE / 2) / ACQ_PLEINE_ECHELLE;
}

int32_t acq_valeur(acq_t *a)
{
    int32_t v = ((int32_t)acq_mv(a) * a->gain_num) / a->gain_den + a->decalage;

    if (a->zone_morte > 0) {                  /* version 2 : zone morte       */
        if (v < a->v_min + a->zone_morte) v = a->v_min;
        if (v > a->v_max - a->zone_morte) v = a->v_max;
    }
    if (a->hysteresis > 0 && a->initialisee) { /* version 2 : hystérésis      */
        int32_t ecart = v - a->derniere;
        if (ecart < 0) ecart = -ecart;
        if (ecart <= a->hysteresis) {
            return a->derniere;
        }
    }
    a->derniere = v;
    a->initialisee = 1u;
    return v;
}
