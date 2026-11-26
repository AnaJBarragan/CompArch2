/* ---------------------------------------------------------------------------
 * trajet.h — suivi d'un trajet défini en distance
 *            (CEG 3536, projet final, partie II, E16)
 * Module SANS matériel : aucun registre, aucun en-tête CMSIS. Compilé sur la
 * cible et sur hôte (hote/test_trajet.c).
 *
 * Un trajet est une suite de segments « distance en tours de l'arbre de
 * sortie, consigne en tr/min ». La position avance au rythme des fronts de
 * l'encodeur (FRONTS_PAR_TOUR = 748 par tour), jamais au rythme du temps.
 * ------------------------------------------------------------------------- */
#ifndef TRAJET_H
#define TRAJET_H

#include <stdint.h>
#include <stdbool.h>
#include "projet.h"

/* Mémorise le trajet (le tableau doit rester valide) et le remet à zéro.    */
void     trajet_init(const segment_t *s, uint32_t n);
/* Retour au début du premier segment, distance nulle.                       */
void     trajet_reset(void);
/* Ajoute des fronts d'encodeur à la distance parcourue. Les fronts sont
 * cumulés sans perte : 748 appels d'un front valent un tour. Un appel peut
 * franchir une ou plusieurs limites de segment. Sans effet une fois le
 * trajet fini.                                                              */
void     trajet_avancer(uint32_t fronts);
/* Consigne du segment courant, en tr/min ; 0 quand le trajet est fini.      */
uint32_t trajet_consigne_rpm(void);
/* Numéro du segment courant, de 1 à n ; n quand le trajet est fini.         */
uint32_t trajet_segment(void);
uint32_t trajet_nb_segments(void);
/* Distance parcourue depuis le début, en dixièmes de tour (troncature),
 * plafonnée à la distance totale.                                           */
uint32_t trajet_dist_x10(void);
/* Distance totale du trajet, en tours.                                      */
uint32_t trajet_total_tours(void);
/* Vrai dès que la distance totale est atteinte.                             */
bool     trajet_fini(void);

/* Ajout de l'équipe : second trajet pour les essais (énoncé 4.8), 25 tours. */
#define TRAJET_EQUIPE_N         4u
extern const segment_t TRAJET_EQUIPE[TRAJET_EQUIPE_N];

#endif /* TRAJET_H */
