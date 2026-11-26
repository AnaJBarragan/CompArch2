/* ---------------------------------------------------------------------------
 * fsm.h — machine à états du véhicule (CEG 3536, laboratoire 3, E8 ;
 *         projet final, partie II, E15)
 * Consomme les événements de la file (evq), les traduit en entrées de
 * l'automate généré (automate_gen.c), et pilote les sorties par les routines
 * assembleur (led_set, motor_set_dir, motor_set_pwm, buzzer_bip).
 * ------------------------------------------------------------------------- */
#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include "lab3.h"
#include "evq.h"

void fsm_init(void);                             /* INIT, sorties sûres (E22)   */
void fsm_traiter(const evq_event_t *e);          /* traduction + transition     */
void fsm_sorties(void);                          /* chaque tick : rampe, DEL, buzzer */
const etat_vehicule_t *fsm_etat(void);           /* état exposé (télémétrie)    */

/* ---- partie II ---- */
/* Fin de la phase INIT (main.c, après INIT_DUREE_MS) : code = DEFAUT_AUCUN
 * (ressources conformes -> PRÊT) ou DEFAUT_INIT (-> DÉFAUT, motif affiché). */
void fsm_fin_init(uint32_t code);
/* Résultat du régulateur et du trajet (main.c, toutes les REGUL_PERIODE_MS) :
 * commande PWM visée en mode AUTONOME (rejointe par la rampe), consigne en
 * tr/min, segment et distance (0,1 tour) pour la télémétrie et le LCD.      */
void fsm_auto(uint32_t pwm_pct, uint32_t consigne_rpm, uint32_t segment, uint32_t dist_x10);
/* Plafond effectif en pour cent : vitesse maximale du TouchPad, abaissée à
 * LIMITATION_PCT quand une limitation est active (SW3, humidité, 40 °C).    */
uint32_t fsm_plafond_pct(void);
/* Vrai en MARCHE_AVANT ou MARCHE_ARRIÈRE, dans les deux modes.             */
bool fsm_en_marche(void);

#endif /* FSM_H */
