/* ---------------------------------------------------------------------------
 * surv.h — surveillances du véhicule : sens de rotation, encodeur muet,
 *          protection thermique à deux seuils
 *          (CEG 3536, projet final, partie II, E18, E19)
 * Module SANS matériel, compilé sur la cible et sur hôte (hote/test_surv.c).
 *
 * surv_step est appelé une fois par fenêtre de mesure (REGUL_PERIODE_MS), avec
 * les valeurs de cette fenêtre. Il ne pousse aucun événement lui-même : la
 * boucle principale compare surv_code() et surv_thermique() à leur valeur
 * précédente et pousse EV_DEFAUT ou EV_THERMIQUE quand elles changent.
 * ------------------------------------------------------------------------- */
#ifndef SURV_H
#define SURV_H

#include <stdint.h>
#include "projet.h"

#define SURV_THERM_NORMAL       0u
#define SURV_THERM_LIMITATION   1u      /* >= 40,0 °C, levée à 37,0 °C        */
#define SURV_THERM_ARRET        2u      /* >= 50,0 °C, sortie à 45,0 °C       */

void     surv_init(void);
/* v            état exposé du véhicule (etat et pwm_pct sont utilisés) ;
 * sens_mesure  SENS_x de la fenêtre (éventuellement injecté, mode essai) ;
 * fronts       fronts d'encodeur de la fenêtre (éventuellement injectés) ;
 * temp_c10     température analogique en 0,1 °C (éventuellement injectée) ;
 * dt_ms        durée de la fenêtre, en millisecondes.
 * Défaut de sens : en MARCHE_AVANT ou MARCHE_ARRIÈRE, sens mesuré OPPOSÉ au
 * sens commandé pendant plus de SURV_SENS_MS ; SENS_IMMOBILE n'est pas opposé.
 * Encodeur muet : en marche, pwm_pct > SURV_MUET_PWM_PCT et aucun front
 * pendant plus de SURV_MUET_MS. Les compteurs reviennent à zéro dès que la
 * condition disparaît et hors de la marche.                                 */
void     surv_step(const etat_vehicule_t *v, uint32_t sens_mesure, uint32_t fronts,
                   int32_t temp_c10, uint32_t dt_ms);
/* DEFAUT_AUCUN, DEFAUT_SENS ou DEFAUT_ENCODEUR. Le code est MÉMORISÉ : il
 * reste jusqu'à surv_acquitter(), car le défaut arrête le moteur et la
 * condition disparaît d'elle-même.                                          */
uint32_t surv_code(void);
void     surv_acquitter(void);
/* SURV_THERM_x : niveau thermique courant, avec hystérésis.                 */
uint32_t surv_thermique(void);

#endif /* SURV_H */
