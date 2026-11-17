/* ---------------------------------------------------------------------------
 * regul.h — régulateur de vitesse proportionnel-intégral en virgule fixe
 *           (CEG 3536, projet final, partie II, E17)
 * Module SANS matériel, compilé sur la cible et sur hôte (hote/test_regul.c).
 *
 * Objet en C, dans la continuité de acq_t (laboratoire 3) : la structure est
 * visible pour que l'appelant réserve l'instance, mais ses champs ne sont
 * accédés que par les fonctions de ce module.
 *
 * Forme incrémentale, exécutée toutes les REGUL_PERIODE_MS = 100 ms :
 *     e = consigne - mesure                     (tr/min)
 *     u = u + Kp x (e - e_precedent) + Ki x e   (points de PWM, Q8)
 *     u borné entre 0 et le plafond             (l'intégrale est ainsi bornée)
 * Les gains sont en Q8 : 256 vaut 1 point de PWM par tr/min.
 * ------------------------------------------------------------------------- */
#ifndef REGUL_H
#define REGUL_H

#include <stdint.h>
#include "projet.h"

typedef struct {
    int32_t  kp_q8;         /* gain proportionnel, Q8                         */
    int32_t  ki_q8;         /* gain intégral par pas, Q8                      */
    int32_t  u_q8;          /* commande courante, Q8 (0 .. 100 x 256)         */
    int32_t  e_prec;        /* erreur du pas précédent, tr/min                */
    uint32_t actif;         /* 0 après regul_reset, 1 après le premier pas    */
} regul_t;

void     regul_init(regul_t *r, int32_t kp_q8, int32_t ki_q8);
/* À appeler à chaque arrêt : le prochain pas repart sans mémoire.           */
void     regul_reset(regul_t *r);
/* Un pas du régulateur. Retourne la commande PWM en pour cent, de 0 à
 * plafond_pct (plafond_pct <= 100).
 *  - premier pas après un reset : la commande part de la valeur attendue en
 *    boucle ouverte, consigne x 100 / VITESSE_MAX_RPM, sans terme
 *    proportionnel ni intégral ;
 *  - consigne nulle : retourne 0 et remet le régulateur à zéro ;
 *  - la commande ne dépasse jamais plafond_pct, et l'état interne non plus
 *    (protection contre l'emballement de l'intégrale).                      */
uint32_t regul_step(regul_t *r, uint32_t consigne_rpm, uint32_t mesure_rpm, uint32_t plafond_pct);

#endif /* REGUL_H */
