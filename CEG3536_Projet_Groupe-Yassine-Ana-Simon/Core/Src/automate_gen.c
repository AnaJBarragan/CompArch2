/* ---------------------------------------------------------------------------
 * automate_gen.c — GENERE par outils/automate.py 1.0 a partir de automate.json
 * Automate « vehicule », somme de la table : F860101E
 * NE PAS MODIFIER A LA MAIN : corriger la table JSON, verifier, puis regenerer.
 * ------------------------------------------------------------------------- */
#include "automate_gen.h"

/* table[etat][entree] = etat suivant */
static const uint8_t table[AUTO_N_ETATS][AUTO_N_ENTREES] = {
    /* INIT               */ {   5,   0,   6,   0,   0,   0,   0,   0,   0,   1 },
    /* PRET               */ {   5,   1,   6,   4,   1,   1,   1,   2,   3,   1 },
    /* MANUEL             */ {   5,   2,   6,   4,   4,   4,   2,   1,   1,   2 },
    /* AUTONOME           */ {   5,   3,   6,   4,   4,   4,   1,   1,   1,   3 },
    /* ARRET_EVENEMENT    */ {   5,   1,   6,   4,   4,   4,   4,   4,   4,   4 },
    /* ARRET_URGENCE      */ {   5,   1,   5,   5,   5,   5,   5,   5,   5,   5 },
    /* DEFAUT             */ {   5,   1,   6,   6,   6,   6,   6,   6,   6,   6 },
};

static const auto_sorties_t sorties[AUTO_N_ETATS] = {
    /* INIT               */ { 0, 0, 0, 0 },
    /* PRET               */ { 0, 0, 0, 0 },
    /* MANUEL             */ { 1, 0, 0, 1 },
    /* AUTONOME           */ { 1, 1, 0, 1 },
    /* ARRET_EVENEMENT    */ { 0, 0, 1, 0 },
    /* ARRET_URGENCE      */ { 0, 0, 2, 2 },
    /* DEFAUT             */ { 0, 0, 1, 0 },
};

static const char *const noms_etats[AUTO_N_ETATS] = {
    "INIT", "PRET", "MANUEL", "AUTONOME", "ARRET_EVENEMENT", "ARRET_URGENCE", "DEFAUT"
};
static const char *const noms_entrees[AUTO_N_ENTREES] = {
    "ESTOP", "ACQUIT", "DEFAUT", "THERMIQUE", "OBSTACLE", "FEU", "FIN", "USER_MAN", "USER_AUT", "INIT_OK"
};
static const char *const noms_sorties[AUTO_N_SORTIES] = {
    "moteur", "regulation", "alarme", "voyant"
};

void automate_sorties(auto_etat_t e, auto_sorties_t *s)
{
    if (s != 0 && (uint32_t)e < AUTO_N_ETATS) {
        *s = sorties[e];
    }
}

auto_etat_t automate_transition(auto_etat_t e, auto_entree_t x, auto_sorties_t *s)
{
    if ((uint32_t)e < AUTO_N_ETATS && (uint32_t)x < AUTO_N_ENTREES) {
        e = (auto_etat_t)table[e][x];
    }
    automate_sorties(e, s);
    return e;
}

auto_entree_t automate_choisir(uint32_t masque)
{
    for (uint32_t i = 0u; i < AUTO_N_ENTREES; i++) {
        if (masque & (1u << i)) {
            return (auto_entree_t)i;
        }
    }
    return AUTO_N_ENTREES;
}

const char *automate_nom_etat(auto_etat_t e)
{
    return ((uint32_t)e < AUTO_N_ETATS) ? noms_etats[e] : "?";
}

const char *automate_nom_entree(auto_entree_t x)
{
    return ((uint32_t)x < AUTO_N_ENTREES) ? noms_entrees[x] : "-";
}

const char *automate_nom_sortie(uint32_t i)
{
    return (i < AUTO_N_SORTIES) ? noms_sorties[i] : 0;
}

uint32_t automate_valeur_sortie(const auto_sorties_t *s, uint32_t i)
{
    switch (i) {
    case 0u: return s->moteur;
    case 1u: return s->regulation;
    case 2u: return s->alarme;
    case 3u: return s->voyant;
    default: return 0u;
    }
}

/* Seconde copie de la table, sous une autre forme : triplets (etat, entree,
 * etat suivant) et somme des sorties de l'etat suivant. */
typedef struct { uint8_t e, x, suivant, somme_sorties; } auto_ref_t;
static const auto_ref_t reference[] = {
    {0,0,5,4}, {0,1,0,0}, {0,2,6,1}, {0,3,0,0}, {0,4,0,0}, {0,5,0,0},
    {0,6,0,0}, {0,7,0,0}, {0,8,0,0}, {0,9,1,0}, {1,0,5,4}, {1,1,1,0},
    {1,2,6,1}, {1,3,4,1}, {1,4,1,0}, {1,5,1,0}, {1,6,1,0}, {1,7,2,2},
    {1,8,3,3}, {1,9,1,0}, {2,0,5,4}, {2,1,2,2}, {2,2,6,1}, {2,3,4,1},
    {2,4,4,1}, {2,5,4,1}, {2,6,2,2}, {2,7,1,0}, {2,8,1,0}, {2,9,2,2},
    {3,0,5,4}, {3,1,3,3}, {3,2,6,1}, {3,3,4,1}, {3,4,4,1}, {3,5,4,1},
    {3,6,1,0}, {3,7,1,0}, {3,8,1,0}, {3,9,3,3}, {4,0,5,4}, {4,1,1,0},
    {4,2,6,1}, {4,3,4,1}, {4,4,4,1}, {4,5,4,1}, {4,6,4,1}, {4,7,4,1},
    {4,8,4,1}, {4,9,4,1}, {5,0,5,4}, {5,1,1,0}, {5,2,5,4}, {5,3,5,4},
    {5,4,5,4}, {5,5,5,4}, {5,6,5,4}, {5,7,5,4}, {5,8,5,4}, {5,9,5,4},
    {6,0,5,4}, {6,1,1,0}, {6,2,6,1}, {6,3,6,1}, {6,4,6,1}, {6,5,6,1},
    {6,6,6,1}, {6,7,6,1}, {6,8,6,1}, {6,9,6,1},
};

uint32_t automate_autotest(void)
{
    uint32_t ecarts = 0u;
    for (uint32_t i = 0u; i < sizeof reference / sizeof reference[0]; i++) {
        auto_sorties_t s;
        uint32_t somme = 0u;
        auto_etat_t suivant = automate_transition((auto_etat_t)reference[i].e,
                                                  (auto_entree_t)reference[i].x, &s);
        for (uint32_t k = 0u; k < AUTO_N_SORTIES; k++) {
            somme += automate_valeur_sortie(&s, k);
        }
        if ((uint32_t)suivant != reference[i].suivant || (somme & 0xFFu) != reference[i].somme_sorties) {
            ecarts++;
        }
    }
    return ecarts;
}
