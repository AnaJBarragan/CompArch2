/* ---------------------------------------------------------------------------
 * tableau_test.c — autotest du formatage du tableau de bord
 *                  (CEG 3536, laboratoire 4, E11)
 *                  — FOURNI (six cas minimaux ; l'équipe en ajoute)
 * Aucune dépendance matérielle ni printf : compilable sur hôte et sur cible.
 * Cas : 1 ARRÊT en mode MANUEL ; 2 MARCHE_AVANT en mode AUTONOME ;
 *       3 ARRÊT_URGENCE ; 4 ARRÊT_ÉVÉNEMENT (OBSTACLE puis FEU ROUGE) ;
 *       5 page des capteurs ; 6 image des huit DEL.
 * Cas ajoutés par l'équipe : 7 MARCHE_ARRIÈRE ; 8 DEFAUT et DEL rouges ;
 * 9 FIN ; 10 PAGE_VERROU ; 11 mesure non valide ; 12 températures négatives ;
 * 13 vitesse à quatre chiffres.
 * ------------------------------------------------------------------------- */
#include "tableau.h"
#include "tableau_test.h"

uint32_t tableau_test_resultats[TABLEAU_TEST_N_CAS];

static uint32_t cas_ok(uint32_t i, int condition)
{
    tableau_test_resultats[i] = condition ? 1u : 0u;
    return condition ? 0u : 1u;
}

/* vrai si les 17 octets (16 caractères et '\0') sont identiques */
static int egal(const char *obtenu, const char *attendu)
{
    for (uint32_t i = 0u; i < 17u; i++) {
        if (obtenu[i] != attendu[i]) {
            return 0;
        }
        if (attendu[i] == '\0') {
            return i == 16u;
        }
    }
    return 0;
}

static etat_vehicule_t vehicule(uint32_t mode, uint32_t etat, uint32_t cause,
                                uint32_t consigne, uint32_t q16, uint32_t plafond)
{
    etat_vehicule_t v;

    v.etat = etat;
    v.sens = SENS_IMMOBILE;
    v.consigne_pct = consigne;
    v.pwm_pct = consigne;
    v.vitesse_rpm_q16 = q16;
    v.transitions = 0u;
    v.mode = mode;
    v.cause = cause;
    v.plafond_pct = plafond;
    v.drapeaux = 0u;
    return v;
}

uint32_t tableau_autotest(void)
{
    uint32_t echecs = 0u;
    char l1[17], l2[17];
    etat_vehicule_t v;
    env_mesure_t m = { 234, 452u, 10032u, 241, 1u };

    /* 1. ARRÊT, mode MANUEL, plafond 100 % */
    v = vehicule(MODE_MANUEL, ETAT_ARRET, CAUSE_AUCUNE, 0u, 0u, 100u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(0u, egal(l1, "MAN -- ARRET    ") && egal(l2, "C000% V   0.0 PX"));

    /* 2. MARCHE_AVANT, mode AUTONOME : 245,3 tr/min = 16 075 981 en Q16.16 */
    v = vehicule(MODE_AUTONOME, ETAT_MARCHE_AVANT, CAUSE_AUCUNE, 60u, 16075981u, 90u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(1u, egal(l1, "AUT AV MARCHE   ") && egal(l2, "C060% V 245.3 P9"));

    /* 3. ARRÊT_URGENCE */
    v = vehicule(MODE_MANUEL, ETAT_ARRET_URGENCE, CAUSE_AUCUNE, 0u, 0u, 50u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(2u, egal(l1, "MAN -- URGENCE  ") && egal(l2, "C000% V   0.0 P5"));

    /* 4. ARRÊT_ÉVÉNEMENT : le message est celui de la cause */
    {
        int ok;
        v = vehicule(MODE_AUTONOME, ETAT_ARRET_EVENEMENT, CAUSE_OBSTACLE, 0u, 0u, 100u);
        tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
        ok = egal(l1, "AUT -- OBSTACLE ");
        v.cause = CAUSE_FEU;
        tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
        echecs += cas_ok(3u, ok && egal(l1, "AUT -- FEU ROUGE"));
    }

    /* 5. page des capteurs */
    tableau_formater(&v, &m, PAGE_CAPTEURS, l1, l2);
    echecs += cas_ok(4u, egal(l1, "T 23.4C  H 45.2%") && egal(l2, "P1003.2hPa A24.1"));

    /* 6. image des DEL : rien d'allumé, puis Y1, Y2, Y3 et R4 */
    {
        int ok;
        v = vehicule(MODE_MANUEL, ETAT_ARRET, CAUSE_AUCUNE, 0u, 0u, 100u);
        ok = tableau_del(&v) == 0x00u;
        v = vehicule(MODE_AUTONOME, ETAT_ARRET_URGENCE, CAUSE_AUCUNE, 0u, 0u, 100u);
        v.drapeaux = DRAPEAU_LIMITATION | DRAPEAU_TOUCH;
        echecs += cas_ok(5u, ok && tableau_del(&v) == (DEL_Y1 | DEL_Y2 | DEL_Y3 | DEL_R4));
    }

    /* ---- cas ajoutés par l'équipe (E11) ---- */

    /* 7. MARCHE_ARRIÈRE, mode MANUEL, consigne 45 %, plafond 70 % */
    v = vehicule(MODE_MANUEL, ETAT_MARCHE_ARRIERE, CAUSE_AUCUNE, 45u, 0u, 70u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(6u, egal(l1, "MAN AR MARCHE   ") && egal(l2, "C045% V   0.0 P7"));

    /* 8. ARRÊT_ÉVÉNEMENT cause DÉFAUT, et DEL Y4, R1, R2, R3 */
    v = vehicule(MODE_MANUEL, ETAT_ARRET_EVENEMENT, CAUSE_DEFAUT, 0u, 0u, 100u);
    v.drapeaux = DRAPEAU_FIN | DRAPEAU_FEU | DRAPEAU_OBSTACLE | DRAPEAU_DEFAUT;
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(7u, egal(l1, "MAN -- DEFAUT   ")
                         && tableau_del(&v) == (DEL_Y4 | DEL_R1 | DEL_R2 | DEL_R3));

    /* 9. fin de trajet : ARRÊT avec la cause FIN */
    v = vehicule(MODE_AUTONOME, ETAT_ARRET, CAUSE_FIN, 0u, 0u, 100u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(8u, egal(l1, "AUT -- FIN      "));

    /* 10. PAGE_VERROU : « VERROU », ligne 2 de la page de conduite */
    v = vehicule(MODE_AUTONOME, ETAT_MARCHE_AVANT, CAUSE_AUCUNE, 60u, 16075981u, 90u);
    tableau_formater(&v, &m, PAGE_VERROU, l1, l2);
    echecs += cas_ok(9u, egal(l1, "VERROU          ") && egal(l2, "C060% V 245.3 P9"));

    /* 11. mesure non valide : tirets, température analogique conservée */
    {
        env_mesure_t inv = { 0, 0u, 0u, 241, 0u };
        tableau_formater(&v, &inv, PAGE_CAPTEURS, l1, l2);
        echecs += cas_ok(10u, egal(l1, "T --.-C  H --.-%") && egal(l2, "P ---.-hPa A24.1"));
    }

    /* 12. températures négatives (capteur et analogique) */
    {
        env_mesure_t froid = { -52, 452u, 10032u, -15, 1u };
        tableau_formater(&v, &froid, PAGE_CAPTEURS, l1, l2);
        echecs += cas_ok(11u, egal(l1, "T -5.2C  H 45.2%") && egal(l2, "P1003.2hPa A-1.5"));
    }

    /* 13. vitesse à quatre chiffres : 1234,5 tr/min = 80 904 192 en Q16.16 */
    v = vehicule(MODE_MANUEL, ETAT_MARCHE_AVANT, CAUSE_AUCUNE, 100u, 80904192u, 100u);
    tableau_formater(&v, &m, PAGE_CONDUITE, l1, l2);
    echecs += cas_ok(12u, egal(l2, "C100% V1234.5 PX"));

    return echecs;
}
