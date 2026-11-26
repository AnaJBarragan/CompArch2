/* ---------------------------------------------------------------------------
 * test_evq.c — programme d'essai hôte de la file d'événements (jalon J2)
 *              CEG 3536, laboratoire 3 — FOURNI ; étendu (partie II, L5-A)
 * Compile sur Windows (MinGW/MSYS2) et Linux : make evq dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_evq test_evq.c \
 *       ../Core/Src/evq.c ../Core/Src/evq_test.c crit_hote.c
 * Le même evq.c et le même evq_test.c sont compilés dans le projet STM32
 * (commande série q) : aucune ligne n'est propre à l'hôte, sauf ce main et
 * crit_hote.c, qui remplace crit.s (couche d'adaptation, L5-C).
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <limits.h>
#include "evq.h"
#include "evq_test.h"

uint32_t crit_hote_primask(void);   /* crit_hote.c : 0 hors de toute section */

int main(void)
{
    uint32_t echecs = evq_autotest();
    uint32_t equilibre = (crit_hote_primask() == 0u);

    printf("evq : capacite %u, sizeof(evq_event_t) = %u, int = %u bits, long = %u bits, "
           "pointeur = %u bits\n",
           (unsigned)EVQ_CAPACITE, (unsigned)sizeof(evq_event_t),
           (unsigned)(sizeof(int) * CHAR_BIT), (unsigned)(sizeof(long) * CHAR_BIT),
           (unsigned)(sizeof(void *) * CHAR_BIT));
    for (uint32_t i = 0u; i < EVQ_TEST_N_CAS; i++) {
        printf("  cas %u : %s\n", (unsigned)(i + 1u), evq_test_resultats[i] ? "reussi" : "ECHEC");
    }
    /* sur hôte, crit_entrer / crit_sortir ne protègent rien, mais ils
     * simulent PRIMASK : chaque entrée doit avoir sa sortie                   */
    printf("  sections critiques equilibrees (PRIMASK simule = 0) : %s\n",
           equilibre ? "reussi" : "ECHEC");
    if (!equilibre) {
        echecs++;
    }
    printf("%u cas, %u echec(s) : %s\n", (unsigned)(EVQ_TEST_N_CAS + 1u), (unsigned)echecs,
           echecs ? "ECHEC" : "OK");
    return echecs ? 1 : 0;
}
