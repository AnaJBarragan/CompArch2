/* ---------------------------------------------------------------------------
 * tableau.h — tableau de bord (CEG 3536, laboratoire 4, E3, E8, E11)
 * Deux fichiers :
 *   tableau.c       formatage des deux lignes et image des huit DEL, SANS
 *                   matériel : compilé sur la cible et sur hôte (test_tableau) ;
 *   tableau_pres.c  présentation : choisit la page, la couleur et le
 *                   clignotement, puis remet le texte à lcd et l'image des DEL
 *                   au répartiteur du bus. Aucune transaction I2C ici.
 *
 * FORMATS (16 caractères par ligne, complétés par des espaces ; l1 et l2
 * reçoivent 16 caractères puis '\0') :
 *
 * PAGE_CONDUITE
 *   ligne 1 : "MMM SS EEEEEEEEE"
 *     MMM = "MAN" ou "AUT" (v->mode)
 *     SS  = "AV" en MARCHE_AVANT, "AR" en MARCHE_ARRIÈRE, "--" sinon
 *     E.. = "ARRET" (ou "FIN" si v->cause == CAUSE_FIN), "MARCHE", "URGENCE",
 *           et en ARRÊT_ÉVÉNEMENT, selon v->cause : "OBSTACLE", "FEU ROUGE",
 *           "DEFAUT"
 *     exemples : "AUT AV MARCHE   "   "AUT -- FEU ROUGE"   "MAN -- ARRET    "
 *   ligne 2 : "Cccc% Vvvvv.v Pp"
 *     ccc    = v->consigne_pct sur trois chiffres (000 à 100)
 *     vvvv.v = vitesse en tr/min, une décimale, arrondie au plus proche à
 *              partir de v->vitesse_rpm_q16, cadrée à droite sur 6 caractères
 *     p      = v->plafond_pct / 10 (1 à 9), ou 'X' pour 100 %
 *     exemples : "C060% V 245.3 P9"   "C000% V   0.0 PX"
 * PAGE_VERROU (pendant VERROU_MS après une touche refusée)
 *   ligne 1 : "VERROU          " ; ligne 2 : celle de PAGE_CONDUITE
 * PAGE_CAPTEURS
 *   ligne 1 : "Tttt.tC  Hhhh.h%"   exemple : "T 23.4C  H 45.2%"
 *     température m->t_c10 et humidité m->h_x10, chacune cadrée à droite sur
 *     5 caractères (signe compris) : colonnes 1 à 5 et 10 à 14
 *   ligne 2 : "Ppppp.phPa Aaa.a"   exemple : "P1003.2hPa A24.1"
 *     pression m->p_hpa_x10 sur 6 caractères, température analogique
 *     m->ta_c10 sur 4 caractères
 *   si m->valide == 0 : "T --.-C  H --.-%" et "P ---.-hPa A24.1" (la
 *     température analogique reste affichée)
 * ------------------------------------------------------------------------- */
#ifndef TABLEAU_H
#define TABLEAU_H

#include <stdint.h>
#include "lab4.h"

/* ---- tableau.c : sans matériel ----------------------------------------- */
void tableau_formater(const etat_vehicule_t *v, const env_mesure_t *m,
                      uint32_t page, char l1[17], char l2[17]);

/* image logique des huit DEL (DEL_x de lab4.h, bit à 1 = allumée) :
 *   Y1 = DRAPEAU_LIMITATION   Y2 = mode AUTONOME      Y3 = DRAPEAU_TOUCH
 *   Y4 = DRAPEAU_FIN          R1 = DRAPEAU_FEU        R2 = DRAPEAU_OBSTACLE
 *   R3 = DRAPEAU_DEFAUT       R4 = état ARRÊT_URGENCE                        */
uint8_t tableau_del(const etat_vehicule_t *v);

/* ---- tableau_pres.c : présentation -------------------------------------- */
void    tableau_init(void);
void    tableau_page_basculer(void);            /* SW8 : conduite <-> capteurs */
void    tableau_verrou(void);                   /* touche refusée : « VERROU » */
void    tableau_env(const env_mesure_t *m);     /* dernière mesure à afficher  */
/* à appeler chaque tick, après fsm_sorties : met à jour le texte et la couleur
 * demandés à lcd, et l'image des DEL. Aucune transaction sur le bus.          */
void    tableau_step(void);
uint8_t tableau_image_del(void);                /* image à écrire sur l'extenseur */

#endif /* TABLEAU_H */
