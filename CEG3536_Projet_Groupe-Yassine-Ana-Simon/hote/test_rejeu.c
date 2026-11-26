/* ---------------------------------------------------------------------------
 * test_rejeu.c — rejeu d'une trace de télémétrie sur hôte (CEG 3536, L5-C)
 *                — ajout de l'équipe
 * Usage : test_rejeu trace.csv decisions.txt
 *   trace.csv      produit par : python telemetrie.py s3.log --extraire s3.csv
 *   decisions.txt  une ligne de décision par trame (rejeu.h)
 * Affiche le bilan, dont l'empreinte à comparer avec Linux, Windows et la
 * cible (commande série j, après python csv_vers_c.py) :
 *   REJEU,trames=150,somme=1A2B3C4D,concordants=148,segment=4,dist_x10=1050,defauts=0
 * Compile :
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_rejeu test_rejeu.c \
 *       ../Core/Src/rejeu.c ../Core/Src/trajet.c ../Core/Src/regul.c ../Core/Src/surv.c
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rejeu.h"
#include "trajet.h"

#define MAX_TRAMES  20000u
#define MAX_COL     32

static rejeu_trame_t trames[MAX_TRAMES];
static FILE *sortie_fichier;

static void ecrire_ligne(const char *ligne)
{
    fputs(ligne, sortie_fichier);
}

static int colonne(char noms[][32], int n, const char *nom)
{
    for (int i = 0; i < n; i++) {
        if (strcmp(noms[i], nom) == 0) {
            return i;
        }
    }
    fprintf(stderr, "colonne absente du CSV : %s\n", nom);
    exit(2);
}

int main(int argc, char **argv)
{
    char ligne[512];
    char noms[MAX_COL][32];
    int ncol = 0;
    uint32_t n = 0u;
    rejeu_bilan_t b;

    if (argc != 3) {
        fprintf(stderr, "usage : %s trace.csv decisions.txt\n", argv[0]);
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    sortie_fichier = fopen(argv[2], "w");
    if (in == NULL || sortie_fichier == NULL) {
        fprintf(stderr, "ouverture impossible : %s ou %s\n", argv[1], argv[2]);
        return 2;
    }
    /* en-tête : noms des colonnes (telemetrie.py --extraire) */
    if (fgets(ligne, sizeof ligne, in) == NULL) {
        fprintf(stderr, "%s : fichier vide\n", argv[1]);
        return 2;
    }
    for (char *tok = strtok(ligne, ",\r\n"); tok != NULL && ncol < MAX_COL; tok = strtok(NULL, ",\r\n")) {
        strncpy(noms[ncol], tok, sizeof noms[0] - 1u);
        noms[ncol][sizeof noms[0] - 1u] = '\0';
        ncol++;
    }
    int c_t = colonne(noms, ncol, "ticks_ms"), c_e = colonne(noms, ncol, "etat");
    int c_s = colonne(noms, ncol, "sens"), c_p = colonne(noms, ncol, "pwm_pct");
    int c_v = colonne(noms, ncol, "vitesse_rpm_x100"), c_th = colonne(noms, ncol, "temp_c_x10");
    int c_m = colonne(noms, ncol, "mode"), c_pl = colonne(noms, ncol, "plafond_pct");
    int c_sg = colonne(noms, ncol, "segment");

    while (fgets(ligne, sizeof ligne, in) != NULL && n < MAX_TRAMES) {
        long v[MAX_COL] = { 0 };
        int i = 0;
        for (char *tok = strtok(ligne, ",\r\n"); tok != NULL && i < MAX_COL; tok = strtok(NULL, ",\r\n")) {
            v[i++] = strtol(tok, NULL, 10);
        }
        if (i < ncol) {
            continue;                           /* ligne incomplète ignorée   */
        }
        trames[n].ticks_ms = (uint32_t)v[c_t];
        trames[n].etat = (uint8_t)v[c_e];
        trames[n].sens = (uint8_t)v[c_s];
        trames[n].pwm_pct = (uint8_t)v[c_p];
        trames[n].vitesse_rpm_x100 = (uint16_t)v[c_v];
        trames[n].temp_c_x10 = (int16_t)v[c_th];
        trames[n].mode = (uint8_t)v[c_m];
        trames[n].plafond_pct = (uint8_t)v[c_pl];
        trames[n].segment = (uint8_t)v[c_sg];
        n++;
    }
    fclose(in);

    rejeu_executer(trames, n, TRAJET_REF, TRAJET_REF_N, ecrire_ligne, &b);
    fclose(sortie_fichier);
    printf("REJEU,trames=%u,somme=%08X,concordants=%u,segment=%u,dist_x10=%u,defauts=%u\n",
           (unsigned)b.n, (unsigned)b.somme, (unsigned)b.concordants, (unsigned)b.segment_final,
           (unsigned)b.dist_x10_final, (unsigned)b.defauts);
    printf("  %s : %u decisions ecrites ; int = %u bits, long = %u bits, pointeur = %u bits, "
           "sizeof(rejeu_trame_t) = %u\n", argv[2], (unsigned)b.n, (unsigned)(sizeof(int) * 8u),
           (unsigned)(sizeof(long) * 8u), (unsigned)(sizeof(void *) * 8u),
           (unsigned)sizeof(rejeu_trame_t));
    return 0;
}
