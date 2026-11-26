/* ---------------------------------------------------------------------------
 * essai.c — mode essai : injection de valeurs de capteurs
 *           (CEG 3536, projet final, E20) — complété par l'équipe
 * Module sans matériel, testé sur hôte : make surv dans hote/.
 *
 * Chaque injection a son indicateur ; une fonction essai_x(mesure) rend la
 * valeur injectée seulement si le mode est actif ET l'injection en cours.
 * La sortie du mode annule toutes les injections : à la rentrée, aucune
 * valeur résiduelle. C'est le SEUL endroit du programme où une mesure peut
 * être remplacée (énoncé 4.2).
 * ------------------------------------------------------------------------- */
#include "essai.h"

#define HUM_AUCUNE  0u
#define HUM_HAUTE   1u
#define HUM_BASSE   2u

static bool     actif;          /* mode essai actif                          */
static bool     temp_inj;       /* température injectée (1, 2, 3)            */
static int32_t  temp_c10;
static uint32_t hum;            /* HUM_x : bascule 80 % / 40 % (h)           */
static bool     sens_inv;       /* sens mesuré inversé (s)                   */
static bool     muet;           /* encodeur muet (n)                         */

static void annuler_injections(void)
{
    temp_inj = false;
    temp_c10 = 0;
    hum = HUM_AUCUNE;
    sens_inv = false;
    muet = false;
}

void essai_init(void)
{
    actif = false;
    annuler_injections();
}

bool essai_actif(void)
{
    return actif;
}

void essai_basculer(void)
{
    actif = !actif;
    if (!actif) {
        annuler_injections();           /* retour aux mesures réelles        */
    }
}

bool essai_commande(char c)
{
    if (c == 'm') {
        essai_basculer();
        return true;
    }
    if (!actif) {
        return false;                   /* injections refusées hors du mode  */
    }
    switch (c) {
    case '1': temp_inj = true; temp_c10 = ESSAI_TEMP_1_C10; return true;
    case '2': temp_inj = true; temp_c10 = ESSAI_TEMP_2_C10; return true;
    case '3': temp_inj = true; temp_c10 = ESSAI_TEMP_3_C10; return true;
    case 'h': hum = (hum == HUM_HAUTE) ? HUM_BASSE : HUM_HAUTE; return true;
    case 's': sens_inv = !sens_inv; return true;
    case 'n': muet = !muet; return true;
    default:  return false;
    }
}

int32_t essai_temp_c10(int32_t mesure)
{
    return (actif && temp_inj) ? temp_c10 : mesure;
}

uint32_t essai_hum_x10(uint32_t mesure)
{
    if (!actif || hum == HUM_AUCUNE) {
        return mesure;
    }
    return (hum == HUM_HAUTE) ? ESSAI_HUM_HAUTE_X10 : ESSAI_HUM_BASSE_X10;
}

uint32_t essai_sens(uint32_t mesure)
{
    if (!actif || !sens_inv) {
        return mesure;
    }
    if (mesure == SENS_AVANT) {
        return SENS_ARRIERE;
    }
    return (mesure == SENS_ARRIERE) ? SENS_AVANT : mesure;  /* immobile inchangé */
}

uint32_t essai_fronts(uint32_t mesure)
{
    return (actif && muet) ? 0u : mesure;
}
