/* ---------------------------------------------------------------------------
 * tableau.c — formatage du tableau de bord (CEG 3536, laboratoire 4, E3, E8)
 *             — complété par l'équipe
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
 * Les chiffres sont produits de droite à gauche (unité de dixième d'abord).
 * Si le nombre ne tient pas, ses chiffres de poids fort sont perdus.        */
static uint32_t placer_fixe1(char l[17], uint32_t col, int32_t x10, uint32_t largeur)
{
    uint32_t neg = (x10 < 0) ? 1u : 0u;
    uint32_t v = neg ? (uint32_t)(-x10) : (uint32_t)x10;
    int32_t  pos = (int32_t)(col + largeur) - 1;    /* dernière colonne      */
    int32_t  debut = (int32_t)col;

    if (pos > 15) {                                 /* jamais hors des 16    */
        return col + largeur;
    }
    l[pos--] = (char)('0' + v % 10u);               /* dixième               */
    v /= 10u;
    l[pos--] = '.';
    do {                                            /* partie entière        */
        l[pos--] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v != 0u && pos >= debut);
    if (neg && pos >= debut) {
        l[pos--] = '-';
    }
    while (pos >= debut) {
        l[pos--] = ' ';
    }
    return col + largeur;
}

/* Écrit v (0 à 999) sur trois chiffres, avec les zéros de tête. */
static uint32_t placer_3chiffres(char l[17], uint32_t col, uint32_t v)
{
    if (v > 999u) {
        v = 999u;
    }
    l[col]      = (char)('0' + v / 100u);
    l[col + 1u] = (char)('0' + (v / 10u) % 10u);
    l[col + 2u] = (char)('0' + v % 10u);
    return col + 3u;
}

static const char *texte_etat(const etat_vehicule_t *v)
{
    switch (v->etat) {
    case ETAT_ARRET:
        return (v->cause == CAUSE_FIN) ? "FIN" : "ARRET";
    case ETAT_MARCHE_AVANT:
    case ETAT_MARCHE_ARRIERE:
        return "MARCHE";
    case ETAT_ARRET_URGENCE:
        return "URGENCE";
    case ETAT_ARRET_EVENEMENT:
        switch (v->cause) {
        case CAUSE_OBSTACLE: return "OBSTACLE";
        case CAUSE_FEU:      return "FEU ROUGE";
        case CAUSE_DEFAUT:   return "DEFAUT";
        default:             return "EVENEMENT";
        }
    default:
        return "?";
    }
}

/* ligne 2 de la page de conduite : "Cccc% Vvvvv.v Pp" */
static void ligne_conduite2(const etat_vehicule_t *v, char l[17])
{
    uint32_t rpm_x10 = (uint32_t)(((uint64_t)v->vitesse_rpm_q16 * 10u + 32768u) >> 16);

    l[0] = 'C';
    placer_3chiffres(l, 1u, v->consigne_pct);
    l[4] = '%';
    l[6] = 'V';
    placer_fixe1(l, 7u, (int32_t)rpm_x10, 6u);
    l[14] = 'P';
    l[15] = (v->plafond_pct >= 100u) ? 'X' : (char)('0' + v->plafond_pct / 10u);
}

void tableau_formater(const etat_vehicule_t *v, const env_mesure_t *m,
                      uint32_t page, char l1[17], char l2[17])
{
    ligne_vide(l1);
    ligne_vide(l2);

    switch (page) {
    case PAGE_CAPTEURS:
        /* "Tttt.tC  Hhhh.h%"  et  "Ppppp.phPa Aaa.a" */
        l1[0] = 'T';
        l1[6] = 'C';
        l1[9] = 'H';
        l1[15] = '%';
        l2[0] = 'P';
        placer(l2, 7u, "hPa");
        l2[11] = 'A';
        if (m->valide) {
            placer_fixe1(l1, 1u, m->t_c10, 5u);
            placer_fixe1(l1, 10u, (int32_t)m->h_x10, 5u);
            placer_fixe1(l2, 1u, (int32_t)m->p_hpa_x10, 6u);
        } else {
            placer(l1, 1u, " --.-");
            placer(l1, 10u, " --.-");
            placer(l2, 1u, " ---.-");
        }
        placer_fixe1(l2, 12u, m->ta_c10, 4u);       /* toujours affichée     */
        break;

    case PAGE_VERROU:
        placer(l1, 0u, "VERROU");
        ligne_conduite2(v, l2);
        break;

    default:                                        /* PAGE_CONDUITE         */
        /* "MMM SS EEEEEEEEE" */
        placer(l1, 0u, (v->mode == MODE_AUTONOME) ? "AUT" : "MAN");
        placer(l1, 4u, (v->etat == ETAT_MARCHE_AVANT)   ? "AV"
                     : (v->etat == ETAT_MARCHE_ARRIERE) ? "AR" : "--");
        placer(l1, 7u, texte_etat(v));
        ligne_conduite2(v, l2);
        break;
    }
}

uint8_t tableau_del(const etat_vehicule_t *v)
{
    uint8_t img = 0u;

    if (v->drapeaux & DRAPEAU_LIMITATION) img |= DEL_Y1;
    if (v->mode == MODE_AUTONOME)         img |= DEL_Y2;
    if (v->drapeaux & DRAPEAU_TOUCH)      img |= DEL_Y3;
    if (v->drapeaux & DRAPEAU_FIN)        img |= DEL_Y4;
    if (v->drapeaux & DRAPEAU_FEU)        img |= DEL_R1;
    if (v->drapeaux & DRAPEAU_OBSTACLE)   img |= DEL_R2;
    if (v->drapeaux & DRAPEAU_DEFAUT)     img |= DEL_R3;
    if (v->etat == ETAT_ARRET_URGENCE)    img |= DEL_R4;
    return img;
}
