/* ---------------------------------------------------------------------------
 * acq.h — objet d'acquisition d'une voie analogique (CEG 3536, lab. 3, L3-C)
 *
 * Une instance par voie. L'état est privé : les champs de acq_t ne sont lus
 * et écrits que par les fonctions de acq.c (la structure est exposée ici
 * uniquement pour permettre l'allocation statique des instances).
 * La source des échantillons est le tampon circulaire rempli par le DMA :
 * src pointe sur le premier échantillon de la voie, pas = nombre de voies
 * entrelacées, n_moy = nombre d'échantillons moyennés (<= échantillons par
 * voie dans le tampon).
 * Version 2 (changement d'exigence, section 4.6 de l'énoncé) :
 * acq_config_filtre ajoute hystérésis et zone morte à acq_valeur, sans
 * changer les appelants ailleurs qu'à l'initialisation.
 * ------------------------------------------------------------------------- */
#ifndef ACQ_H
#define ACQ_H

#include <stdint.h>

typedef struct {
    /* --- privé : ne pas accéder hors de acq.c --- */
    const volatile uint16_t *src;   /* premier échantillon de la voie (tampon DMA) */
    uint32_t pas;                   /* nombre de voies entrelacées                */
    uint32_t n_moy;                 /* échantillons moyennés                      */
    int32_t  gain_num;              /* valeur = mv * gain_num / gain_den + decalage */
    int32_t  gain_den;
    int32_t  decalage;
    /* version 2 */
    int32_t  hysteresis;            /* 0 = désactivée                              */
    int32_t  zone_morte;            /* 0 = désactivée                              */
    int32_t  v_min, v_max;          /* bornes pour la zone morte                   */
    int32_t  derniere;              /* dernière valeur retournée (hystérésis)      */
    uint32_t initialisee;
} acq_t;

void     acq_init(acq_t *a, const volatile uint16_t *src, uint32_t pas, uint32_t n_moy,
                  int32_t gain_num, int32_t gain_den, int32_t decalage);
void     acq_config_filtre(acq_t *a, int32_t hysteresis, int32_t zone_morte,
                           int32_t v_min, int32_t v_max);           /* version 2 */
uint32_t acq_brut(const acq_t *a);      /* moyenne des n_moy échantillons (0..4095) */
uint32_t acq_mv(const acq_t *a);        /* tension en millivolts                     */
int32_t  acq_valeur(acq_t *a);          /* valeur étalonnée (%, dixièmes de °C, ...) */

#endif /* ACQ_H */
