/* ---------------------------------------------------------------------------
 * test_surv.c — essai hôte des surveillances et du mode essai
 *               (CEG 3536, projet final, E18, E19, E20) — FOURNI ; cas à ajouter
 * Compile sur Windows (MinGW/MSYS2) et Linux : make surv dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_surv test_surv.c \
 *       ../Core/Src/surv.c ../Core/Src/essai.c
 * ------------------------------------------------------------------------- */
#include <string.h>
#include "banc.h"
#include "surv.h"
#include "essai.h"

#define DT REGUL_PERIODE_MS         /* une fenêtre de 100 ms par appel        */

static etat_vehicule_t v;

static void fenetres(unsigned n, uint32_t sens, uint32_t fronts, int32_t temp)
{
    for (unsigned i = 0u; i < n; i++) {
        surv_step(&v, sens, fronts, temp, DT);
    }
}

static uint32_t therm(int32_t temp)
{
    surv_step(&v, SENS_IMMOBILE, 0u, temp, DT);
    return surv_thermique();
}

int main(void)
{
    memset(&v, 0, sizeof v);

    printf("surv : sens de rotation\n");
    surv_init();
    v.etat = ETAT_MARCHE_AVANT;
    v.pwm_pct = 60u;
    fenetres(5u, SENS_ARRIERE, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "500 ms de sens oppose : pas encore de defaut");
    fenetres(1u, SENS_ARRIERE, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_SENS, "plus de 500 ms de sens oppose : DEFAUT_SENS");
    v.etat = ETAT_ARRET;
    v.pwm_pct = 0u;
    fenetres(20u, SENS_IMMOBILE, 0u, 250);
    VERIFIER(surv_code() == DEFAUT_SENS, "le code reste memorise apres l'arret");
    surv_acquitter();
    VERIFIER(surv_code() == DEFAUT_AUCUN, "acquittement : code efface");

    v.etat = ETAT_MARCHE_AVANT;
    v.pwm_pct = 60u;
    fenetres(4u, SENS_ARRIERE, 40u, 250);
    fenetres(1u, SENS_AVANT, 40u, 250);
    fenetres(4u, SENS_ARRIERE, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "sens oppose interrompu : le compteur repart de zero");
    surv_acquitter();
    fenetres(20u, SENS_IMMOBILE, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "SENS_IMMOBILE n'est pas un sens oppose");
    v.etat = ETAT_MARCHE_ARRIERE;
    fenetres(6u, SENS_AVANT, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_SENS, "marche arriere, sens mesure avant : DEFAUT_SENS");
    surv_acquitter();
    v.etat = ETAT_ARRET;
    v.pwm_pct = 0u;
    fenetres(20u, SENS_ARRIERE, 40u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "a l'arret : aucune surveillance du sens");

    printf("surv : encodeur muet\n");
    surv_init();
    v.etat = ETAT_MARCHE_AVANT;
    v.pwm_pct = 60u;
    fenetres(10u, SENS_IMMOBILE, 0u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "1 s sans front : pas encore de defaut");
    fenetres(1u, SENS_IMMOBILE, 0u, 250);
    VERIFIER(surv_code() == DEFAUT_ENCODEUR, "plus de 1 s sans front a 60 % : DEFAUT_ENCODEUR");
    surv_acquitter();
    v.pwm_pct = 30u;
    fenetres(30u, SENS_IMMOBILE, 0u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "commande de 30 % (pas plus de 30 %) : pas de defaut");
    v.pwm_pct = 60u;
    fenetres(9u, SENS_IMMOBILE, 0u, 250);
    fenetres(1u, SENS_AVANT, 12u, 250);
    fenetres(9u, SENS_IMMOBILE, 0u, 250);
    VERIFIER(surv_code() == DEFAUT_AUCUN, "un front suffit a remettre le compteur a zero");

    printf("surv : protection thermique\n");
    surv_init();
    v.etat = ETAT_ARRET;
    v.pwm_pct = 0u;
    VERIFIER(therm(399) == SURV_THERM_NORMAL, "39,9 C : normal");
    VERIFIER(therm(400) == SURV_THERM_LIMITATION, "40,0 C : limitation");
    VERIFIER(therm(380) == SURV_THERM_LIMITATION, "38,0 C : limitation maintenue (hysteresis)");
    VERIFIER(therm(370) == SURV_THERM_NORMAL, "37,0 C : limitation levee");
    VERIFIER(therm(500) == SURV_THERM_ARRET, "50,0 C : arret");
    VERIFIER(therm(460) == SURV_THERM_ARRET, "46,0 C : arret maintenu (hysteresis)");
    VERIFIER(therm(450) == SURV_THERM_LIMITATION, "45,0 C : arret leve, limitation encore active");
    VERIFIER(therm(250) == SURV_THERM_NORMAL, "25,0 C : normal");
    VERIFIER(therm(520) == SURV_THERM_ARRET && therm(250) == SURV_THERM_NORMAL,
             "saut de 25 a 52 C puis retour : arret, puis normal");

    printf("essai : mode essai\n");
    essai_init();
    VERIFIER(!essai_actif() && !essai_commande('1') && essai_temp_c10(231) == 231,
             "mode inactif : injection refusee, mesure reelle");
    VERIFIER(essai_commande('m') && essai_actif() && essai_temp_c10(231) == 231
             && essai_fronts(40u) == 40u, "m : mode actif, aucune injection encore");
    VERIFIER(essai_commande('1') && essai_temp_c10(231) == ESSAI_TEMP_1_C10, "1 : temperature injectee 42,0 C");
    VERIFIER(essai_commande('2') && essai_temp_c10(231) == ESSAI_TEMP_2_C10, "2 : temperature injectee 52,0 C");
    VERIFIER(essai_commande('3') && essai_temp_c10(231) == ESSAI_TEMP_3_C10, "3 : temperature injectee 25,0 C");
    VERIFIER(essai_hum_x10(452u) == 452u && essai_commande('h') && essai_hum_x10(452u) == ESSAI_HUM_HAUTE_X10
             && essai_commande('h') && essai_hum_x10(452u) == ESSAI_HUM_BASSE_X10,
             "h : humidite injectee 80 %, puis 40 %");
    VERIFIER(essai_commande('s') && essai_sens(SENS_AVANT) == SENS_ARRIERE
             && essai_sens(SENS_ARRIERE) == SENS_AVANT && essai_sens(SENS_IMMOBILE) == SENS_IMMOBILE,
             "s : sens mesure inverse, immobile inchange");
    VERIFIER(essai_commande('s') && essai_sens(SENS_AVANT) == SENS_AVANT, "s une seconde fois : sens reel");
    VERIFIER(essai_commande('n') && essai_fronts(40u) == 0u, "n : encodeur muet");
    VERIFIER(!essai_commande('?'), "caractere inconnu : refuse");
    VERIFIER(essai_commande('m') && !essai_actif() && essai_temp_c10(231) == 231
             && essai_hum_x10(452u) == 452u && essai_fronts(40u) == 40u,
             "m : sortie du mode, toutes les mesures reelles retablies");
    VERIFIER(essai_commande('m') && essai_temp_c10(231) == 231 && essai_fronts(40u) == 40u,
             "nouvelle entree dans le mode : aucune injection residuelle");

    /* À COMPLÉTER : au moins deux cas de l'équipe                            */

    return banc_bilan("test_surv");
}
