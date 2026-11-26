/* ---------------------------------------------------------------------------
 * somme_test.h — banc de la routine assembleur trame_xor8
 *                (CEG 3536, laboratoire 4, E10, T9) — cible seulement
 * ------------------------------------------------------------------------- */
#ifndef SOMME_TEST_H
#define SOMME_TEST_H

#include <stdint.h>

#define SOMME_TEST_N_CAS 5u

uint32_t somme_autotest(void);      /* nombre de cas en échec (0 = succès)   */
extern uint32_t somme_test_obtenu[SOMME_TEST_N_CAS];    /* valeurs retournées */
extern const uint32_t somme_test_attendu[SOMME_TEST_N_CAS];

#endif /* SOMME_TEST_H */
