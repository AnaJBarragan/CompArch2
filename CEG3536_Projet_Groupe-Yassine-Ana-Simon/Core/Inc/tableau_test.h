/* ---------------------------------------------------------------------------
 * tableau_test.h — autotest du formatage du tableau de bord
 *                  (CEG 3536, laboratoire 4, E11)
 * Même source sur hôte (hote/test_tableau.c) et sur cible (commande série f).
 * ------------------------------------------------------------------------- */
#ifndef TABLEAU_TEST_H
#define TABLEAU_TEST_H

#include <stdint.h>

#define TABLEAU_TEST_N_CAS 20u      /* 6 fournis + 7 (partie I) + 7 (partie II) */

uint32_t tableau_autotest(void);    /* nombre de cas en échec (0 = succès)  */
extern uint32_t tableau_test_resultats[TABLEAU_TEST_N_CAS];   /* 1 = réussi */

#endif /* TABLEAU_TEST_H */
