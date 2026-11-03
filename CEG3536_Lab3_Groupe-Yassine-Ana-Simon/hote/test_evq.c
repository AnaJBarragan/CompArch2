/* ---------------------------------------------------------------------------
 * test_evq.c — programme d'essai hôte de la file d'événements (jalon J2)
 *              CEG 3536, laboratoire 3 — FOURNI
 * Compile sur Windows (MinGW/MSYS2) et Linux : make dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_evq test_evq.c \
 *       ../Core/Src/evq.c ../Core/Src/evq_test.c
 * Le même evq.c et le même evq_test.c sont compilés dans le projet STM32
 * (commande série q) : aucune ligne n'est propre à l'hôte, sauf ce main.
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <limits.h>
#include "evq.h"
#include "evq_test.h"

int main(void)
{
    uint32_t echecs = evq_autotest();

    printf("evq : capacite %u, sizeof(evq_event_t) = %u, int = %u bits, long = %u bits\n",
           (unsigned)EVQ_CAPACITE, (unsigned)sizeof(evq_event_t),
           (unsigned)(sizeof(int) * CHAR_BIT), (unsigned)(sizeof(long) * CHAR_BIT));
    for (uint32_t i = 0u; i < EVQ_TEST_N_CAS; i++) {
        printf("  cas %u : %s\n", (unsigned)(i + 1u), evq_test_resultats[i] ? "reussi" : "ECHEC");
    }
    printf("%u cas, %u echec(s) : %s\n", (unsigned)EVQ_TEST_N_CAS, (unsigned)echecs,
           echecs ? "ECHEC" : "OK");
    return echecs ? 1 : 0;
}
