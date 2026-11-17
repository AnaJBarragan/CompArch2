/* ---------------------------------------------------------------------------
 * test_tableau.c — programme d'essai hôte du formatage du tableau de bord
 *                  CEG 3536, laboratoire 4 (E11) — FOURNI
 * Compile sur Windows (MinGW/MSYS2) et Linux : make dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_tableau test_tableau.c \
 *       ../Core/Src/tableau.c ../Core/Src/tableau_test.c
 * Le même tableau.c et le même tableau_test.c sont compilés dans le projet
 * STM32 (commande série f) : aucune ligne n'est propre à l'hôte, sauf ce main.
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include "tableau.h"
#include "tableau_test.h"

static void montrer(const char *titre, const etat_vehicule_t *v,
                    const env_mesure_t *m, uint32_t page)
{
    char l1[17], l2[17];

    tableau_formater(v, m, page, l1, l2);
    printf("  %-22s |%s|\n  %-22s |%s|\n", titre, l1, "", l2);
}

int main(void)
{
    uint32_t echecs = tableau_autotest();
    etat_vehicule_t v = { ETAT_MARCHE_AVANT, SENS_IMMOBILE, 60u, 60u, 16075981u, 3u,
                          MODE_AUTONOME, CAUSE_AUCUNE, 90u, DRAPEAU_TOUCH,
                          0u, 0u, 0u, 0u, 0u };     /* champs de la partie II */
    env_mesure_t m = { 234, 452u, 10032u, 241, 1u };

    printf("tableau : sizeof(etat_vehicule_t) = %u, sizeof(env_mesure_t) = %u\n",
           (unsigned)sizeof(etat_vehicule_t), (unsigned)sizeof(env_mesure_t));
    for (uint32_t i = 0u; i < TABLEAU_TEST_N_CAS; i++) {
        printf("  cas %u : %s\n", (unsigned)(i + 1u),
               tableau_test_resultats[i] ? "reussi" : "ECHEC");
    }
    printf("apercu (entre barres : les 16 colonnes de l'afficheur)\n");
    montrer("conduite", &v, &m, PAGE_CONDUITE);
    montrer("capteurs", &v, &m, PAGE_CAPTEURS);
    montrer("verrou", &v, &m, PAGE_VERROU);
    printf("  image des DEL : 0x%02X\n", (unsigned)tableau_del(&v));
    printf("%u cas, %u echec(s) : %s\n", (unsigned)TABLEAU_TEST_N_CAS, (unsigned)echecs,
           echecs ? "ECHEC" : "OK");
    return echecs ? 1 : 0;
}
