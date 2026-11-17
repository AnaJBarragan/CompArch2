/* ---------------------------------------------------------------------------
 * evq_test.h — autotest de la file d'événements (CEG 3536, lab. 3, E11)
 * Même source sur hôte (hote/test_evq.c l'appelle) et sur cible (commande q).
 * ------------------------------------------------------------------------- */
#ifndef EVQ_TEST_H
#define EVQ_TEST_H

#include <stdint.h>

#define EVQ_TEST_N_CAS 9u

uint32_t evq_autotest(void);               /* nombre de cas en échec (0 = succès) */
extern uint32_t evq_test_resultats[EVQ_TEST_N_CAS];   /* 1 = réussi, 0 = échec */

#endif /* EVQ_TEST_H */
