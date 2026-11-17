/* ---------------------------------------------------------------------------
 * tableau.c — formatage du tableau de bord (CEG 3536, laboratoire 4, E3, E8)
 *             — SQUELETTE À COMPLÉTER
 * AUCUN accès matériel, aucun en-tête CMSIS, pas de printf : ce fichier est
 * compilé tel quel sur la cible et sur hôte (hote/test_tableau.c, E11).
 * Les formats sont définis dans tableau.h ; l'autotest (tableau_test.c) en
 * vérifie six cas — les faire passer sur hôte AVANT de brancher l'afficheur.
 * ------------------------------------------------------------------------- */
#include "tableau.h"

/* [FOURNI] remplit une ligne de 16 espaces, terminée par '\0' */
static void ligne_vide(char l[17])
{
    for (uint32_t i = 0u; i < 16u; i++) {
        l[i] = ' ';
    }
    l[16] = '\0';
}

/* [FOURNI] copie s dans l à partir de la colonne col, sans dépasser 16 ni
 * écrire le '\0' de s ; retourne la colonne qui suit                          */
static uint32_t placer(char l[17], uint32_t col, const char *s)
{
    while (*s != '\0' && col < 16u) {
        l[col++] = *s++;
    }
    return col;
}

/* Écrit x10 / 10 avec une décimale, cadré à droite sur largeur caractères, à
 * partir de la colonne col ; retourne la colonne qui suit.
 * Exemples (largeur 6) : 2453 -> " 245.3" ; 0 -> "   0.0" ; -52 -> "  -5.2".
 * À COMPLÉTER (chiffres produits de droite à gauche ; signe ; espaces).       */
static uint32_t placer_fixe1(char l[17], uint32_t col, int32_t x10, uint32_t largeur)
{
    (void)l; (void)x10;
    return col + largeur;
}

void tableau_formater(const etat_vehicule_t *v, const env_mesure_t *m,
                      uint32_t page, char l1[17], char l2[17])
{
    ligne_vide(l1);
    ligne_vide(l2);

    /* À COMPLÉTER : les trois pages de tableau.h.
     * Vitesse en dixièmes de tr/min, arrondie au plus proche :
     *     (uint32_t)(((uint64_t)v->vitesse_rpm_q16 * 10u + 32768u) >> 16)     */
    (void)v; (void)m; (void)page;
    (void)placer; (void)placer_fixe1;
}

uint8_t tableau_del(const etat_vehicule_t *v)
{
    /* À COMPLÉTER : image logique décrite dans tableau.h */
    (void)v;
    return 0u;
}
