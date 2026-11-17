/* ---------------------------------------------------------------------------
 * test_automate.c — trace du CODE GÉNÉRÉ sur une suite d'entrées
 *                   (CEG 3536, projet final, E15, T12, L5-B) — FOURNI
 * Usage : test_automate sequence.txt trace_code.txt
 * Lit une suite d'entrées (un nom par ligne, # pour les commentaires), exécute
 * automate_transition du code généré et écrit la trace dans le même format
 * que « automate.py simuler », qui exécute le MODÈLE :
 *     0 - -> ETAT_INITIAL sortie=valeur ...
 *     1 ENTREE -> ETAT sortie=valeur ...
 * « automate.py comparer » dit ensuite si les deux traces sont identiques
 * (make automate fait les trois étapes). Exécute aussi automate_autotest().
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "automate_gen.h"

static void ecrire(FILE *f, unsigned i, const char *entree, auto_etat_t e)
{
    auto_sorties_t s;
    automate_sorties(e, &s);
    fprintf(f, "%u %s -> %s", i, entree, automate_nom_etat(e));
    for (uint32_t k = 0u; k < AUTO_N_SORTIES; k++) {
        fprintf(f, " %s=%u", automate_nom_sortie(k), (unsigned)automate_valeur_sortie(&s, k));
    }
    fprintf(f, "\n");
}

int main(int argc, char **argv)
{
    char ligne[128];
    unsigned i = 0u, no = 0u;
    auto_etat_t e = AUTO_ETAT_INITIAL;

    if (argc != 3) {
        fprintf(stderr, "usage : %s sequence.txt trace_code.txt\n", argv[0]);
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");
    if (in == NULL || out == NULL) {
        fprintf(stderr, "ouverture impossible : %s ou %s\n", argv[1], argv[2]);
        return 2;
    }
    ecrire(out, 0u, "-", e);
    while (fgets(ligne, sizeof ligne, in) != NULL) {
        no++;
        char *diese = strchr(ligne, '#');
        if (diese != NULL) {
            *diese = '\0';
        }
        char *mot = strtok(ligne, " \t\r\n");
        if (mot == NULL) {
            continue;
        }
        uint32_t x;
        for (x = 0u; x < AUTO_N_ENTREES; x++) {
            if (strcmp(mot, automate_nom_entree((auto_entree_t)x)) == 0) {
                break;
            }
        }
        if (x == AUTO_N_ENTREES) {
            fprintf(stderr, "%s, ligne %u : entree inconnue \"%s\"\n", argv[1], no, mot);
            return 2;
        }
        e = automate_transition(e, (auto_entree_t)x, 0);
        ecrire(out, ++i, mot, e);
    }
    fclose(in);
    fclose(out);

    uint32_t ecarts = automate_autotest();
    printf("test_automate : somme de la table %08X, %u etats, %u entrees, %u pas executes\n",
           (unsigned)AUTO_SOMME, (unsigned)AUTO_N_ETATS, (unsigned)AUTO_N_ENTREES, i);
    printf("test_automate : autotest du code genere, %u ecart(s) : %s\n", (unsigned)ecarts,
           ecarts ? "ECHEC" : "OK");
    return ecarts ? 1 : 0;
}
