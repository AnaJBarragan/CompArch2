/* ---------------------------------------------------------------------------
 * fsm.h — machine à états du véhicule en C (CEG 3536, laboratoire 3, E8)
 * Consomme les événements de la file (evq) et pilote les sorties par les
 * routines assembleur (led_set, motor_set_dir, motor_set_pwm, buzzer_bip).
 * ------------------------------------------------------------------------- */
#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include "lab3.h"
#include "evq.h"

void fsm_init(void);                             /* ARRÊT, sorties sûres (E1)   */
void fsm_traiter(const evq_event_t *e);          /* une transition par événement */
void fsm_sorties(void);                          /* chaque tick : rampe, DEL, buzzer */
const etat_vehicule_t *fsm_etat(void);           /* état exposé (télémétrie)    */

#endif /* FSM_H */
