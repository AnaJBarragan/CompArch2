/* ---------------------------------------------------------------------------
 * crit_test.h — banc des sections critiques et rafale d'événements
 *               (CEG 3536, projet final, E13, E14, T11) — cible seulement
 * ------------------------------------------------------------------------- */
#ifndef CRIT_TEST_H
#define CRIT_TEST_H

#include <stdint.h>

#define CRIT_TEST_N_CAS 5u

/* Banc de crit_entrer / crit_sortir : nombre de cas en échec (0 = succès).
 * À appeler depuis la commande série c, hors de toute section critique.
 * Rétablit toujours PRIMASK = 0 avant de retourner.                         */
uint32_t crit_autotest(void);
extern uint32_t crit_test_resultats[CRIT_TEST_N_CAS];   /* 1 = cas réussi     */

/* Rafale de n événements EV_TEST poussés DEPUIS UN GESTIONNAIRE D'INTERRUPTION
 * (ligne EXTI15, déclenchée par logiciel), pendant que la boucle principale
 * consomme la file : commande série e (E14). crit_rafale_init() est à appeler
 * une fois, après evq_init().                                               */
void     crit_rafale_init(void);
void     crit_rafale(uint32_t n);
extern volatile uint32_t crit_rafale_refus;     /* evq_push refusés (file pleine) */

#endif /* CRIT_TEST_H */
