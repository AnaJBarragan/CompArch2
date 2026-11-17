/* ---------------------------------------------------------------------------
 * essai.h — mode essai : injection de valeurs de capteurs
 *           (CEG 3536, projet final, partie II, E20)
 * Module SANS matériel, compilé sur la cible et sur hôte (hote/test_surv.c).
 *
 * C'est le SEUL endroit du programme où une mesure peut être remplacée. Chaque
 * fonction essai_x(mesure) retourne la mesure réelle, ou la valeur injectée
 * lorsque le mode essai est actif ET qu'une injection de ce type est en cours.
 *
 * SÉCURITÉ : essai_sens et essai_fronts ne s'appliquent qu'aux valeurs
 * remises à surv_step. Le régulateur de vitesse et le trajet reçoivent
 * TOUJOURS la mesure réelle : un encodeur « muet » injecté dans le régulateur
 * le ferait accélérer jusqu'à la commande maximale.
 *
 * Commandes (un caractère) :
 *   m  active ou désactive le mode essai ; la désactivation annule toutes les
 *      injections (SW7 appelle essai_basculer) ;
 *   1  température injectée 42,0 °C     2  52,0 °C     3  25,0 °C ;
 *   h  humidité injectée : 80 % au premier appui, 40 % au suivant, etc. ;
 *   s  sens mesuré inversé (bascule) ;
 *   n  encodeur muet (bascule).
 * Hors du mode essai, les commandes 1, 2, 3, h, s et n sont refusées.
 * ------------------------------------------------------------------------- */
#ifndef ESSAI_H
#define ESSAI_H

#include <stdint.h>
#include <stdbool.h>
#include "projet.h"

void     essai_init(void);                  /* mode inactif, aucune injection  */
bool     essai_actif(void);
void     essai_basculer(void);              /* équivalent de la commande m     */
/* Traite un caractère reçu. Retourne true si la commande a été acceptée,
 * false si elle n'appartient pas au mode essai ou si elle est refusée.      */
bool     essai_commande(char c);

int32_t  essai_temp_c10(int32_t mesure);    /* température analogique, 0,1 °C  */
uint32_t essai_hum_x10(uint32_t mesure);    /* humidité relative, 0,1 %        */
uint32_t essai_sens(uint32_t mesure);       /* SENS_x ; AVANT <-> ARRIÈRE si s */
uint32_t essai_fronts(uint32_t mesure);     /* fronts de la fenêtre ; 0 si n   */

#endif /* ESSAI_H */
