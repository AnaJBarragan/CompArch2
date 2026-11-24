/* ---------------------------------------------------------------------------
 * tableau_test.h — autotest du formatage du tableau de bord
 *                  (CEG 3536, laboratoire 4, E11)
 * Même source sur hôte (hote/test_tableau.c) et sur cible (commande série f).
 * ------------------------------------------------------------------------- */
#ifndef TABLEAU_TEST_H
#define TABLEAU_TEST_H

#include <stdint.h>

#define TABLEAU_TEST_N_CAS 13u      /* 6 fournis + 7 de l'équipe           */

uint32_t tableau_autotest(void);    /* nombre de cas en échec (0 = succès)  */
extern uint32_t tableau_test_resultats[TABLEAU_TEST_N_CAS];   /* 1 = réussi */

#endif /* TABLEAU_TEST_H */
