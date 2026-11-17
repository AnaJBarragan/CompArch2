/* ---------------------------------------------------------------------------
 * acq.c — objet d'acquisition d'une voie analogique (CEG 3536, lab. 3, L3-C)
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

/* Moyenne arrondie des n_moy échantillons de la voie : src[0], src[pas],
 * src[2 x pas], ... Le DMA écrit le tampon pendant la lecture ; chaque
 * lecture de 16 bits est atomique, donc chaque échantillon lu est valide. */
uint32_t acq_brut(const acq_t *a)
{
    uint32_t somme = 0u;
    for (uint32_t i = 0u; i < a->n_moy; i++) {
        somme += a->src[i * a->pas];
    }
    return (somme + a->n_moy / 2u) / a->n_moy;   /* arrondi au plus proche */
}

/* Division entière arrondie au plus proche (et non tronquée vers zéro),
 * comme pct = (brut x 100 + 2047) / 4095 dans l'énoncé (4.8) : sans
 * arrondi, 97,7 % deviendrait 97 et la zone morte haute ne serait jamais
 * atteinte. Valable pour les deux signes de n et d.                        */
static int32_t div_arrondie(int32_t n, int32_t d)
{
    return ((n < 0) != (d < 0)) ? (n - d / 2) / d : (n + d / 2) / d;
}

uint32_t acq_mv(const acq_t *a)
{
    return (acq_brut(a) * ACQ_VREF_MV + ACQ_PLEINE_ECHELLE / 2) / ACQ_PLEINE_ECHELLE;
}

/* Version 2 : relation linéaire, puis filtres configurés par
 * acq_config_filtre (sans effet si les paramètres sont nuls) :
 *  1. zone morte : v < v_min + zone_morte -> v_min ;
 *                  v > v_max - zone_morte -> v_max ;
 *  2. hystérésis : la valeur retournée ne change que si l'écart avec la
 *     dernière valeur retournée dépasse hysteresis.
 * La zone morte passe avant l'hystérésis : les extrémités (0 et 100 %)
 * restent atteignables, car le saut vers v_min ou v_max dépasse alors
 * l'hystérésis.                                                         */
int32_t acq_valeur(acq_t *a)
{
    int32_t v = div_arrondie((int32_t)acq_mv(a) * a->gain_num, a->gain_den) + a->decalage;

    if (a->zone_morte > 0) {
        if (v < a->v_min + a->zone_morte) {
            v = a->v_min;
        } else if (v > a->v_max - a->zone_morte) {
            v = a->v_max;
        }
    }
    if (a->hysteresis > 0 && a->initialisee) {
        int32_t ecart = v - a->derniere;
        if (ecart < 0) {
            ecart = -ecart;
        }
        if (ecart <= a->hysteresis) {
            return a->derniere;            /* variation due au bruit : ignorée */
        }
    }
    a->derniere = v;
    a->initialisee = 1u;
    return v;
}
