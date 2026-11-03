/* ---------------------------------------------------------------------------
 * acq.c — objet d'acquisition d'une voie analogique (CEG 3536, lab. 3, L3-C)
 *         — SQUELETTE À COMPLÉTER
 * Aucun registre : les échantillons viennent d'un tampon mémoire rempli par
 * le DMA. Toutes les opérations en entiers 32 bits.
 * Version 1 : acq_init, acq_brut, acq_mv, acq_valeur (relation linéaire).
 * Version 2 (changement d'exigence, énoncé 4.6) : acq_config_filtre et, dans
 * acq_valeur, zone morte puis hystérésis, sans toucher aux appelants.
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

/* À COMPLÉTER (E4) : moyenne arrondie des n_moy échantillons src[i * pas]. */
uint32_t acq_brut(const acq_t *a)
{
    return a->src[0];                 /* squelette : dernier échantillon seul */
}

uint32_t acq_mv(const acq_t *a)
{
    return (acq_brut(a) * ACQ_VREF_MV + ACQ_PLEINE_ECHELLE / 2) / ACQ_PLEINE_ECHELLE;
}

/* À COMPLÉTER (E5, E6) : v = mv x gain_num / gain_den + decalage ;
 * version 2 : si zone_morte > 0, v < v_min + zone_morte -> v_min et
 * v > v_max - zone_morte -> v_max ; si hysteresis > 0 et initialisee,
 * retourner derniere tant que |v - derniere| <= hysteresis ; sinon
 * derniere = v, initialisee = 1.                                            */
int32_t acq_valeur(acq_t *a)
{
    int32_t v = ((int32_t)acq_mv(a) * a->gain_num) / a->gain_den + a->decalage;
    a->derniere = v;
    a->initialisee = 1u;
    return v;
}
