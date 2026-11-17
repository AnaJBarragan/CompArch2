/* ---------------------------------------------------------------------------
 * essai.c — mode essai : injection de valeurs de capteurs
 *           (CEG 3536, projet final, E20) — SQUELETTE À COMPLÉTER
 * Module sans matériel, testé sur hôte : make surv dans hote/.
 * La bascule du mode (commande m) est fournie ; les injections sont à écrire.
 * ------------------------------------------------------------------------- */
#include "essai.h"

static bool actif;              /* mode essai actif                          */
/* À COMPLÉTER : une variable par injection (température, humidité, sens,
 * encodeur muet), chacune avec son indicateur « injection en cours »         */

static void annuler_injections(void)
{
    /* À COMPLÉTER */
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
    /* À COMPLÉTER : 1, 2, 3, h, s, n */
    return false;
}

int32_t essai_temp_c10(int32_t mesure)
{
    /* À COMPLÉTER */
    return mesure;
}

uint32_t essai_hum_x10(uint32_t mesure)
{
    /* À COMPLÉTER */
    return mesure;
}

uint32_t essai_sens(uint32_t mesure)
{
    /* À COMPLÉTER */
    return mesure;
}

uint32_t essai_fronts(uint32_t mesure)
{
    /* À COMPLÉTER */
    return mesure;
}
