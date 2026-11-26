/* ---------------------------------------------------------------------------
 * surv.c — surveillances : sens, encodeur muet, protection thermique
 *          (CEG 3536, projet final, E18, E19) — SQUELETTE À COMPLÉTER
 * Module sans matériel, testé sur hôte : make surv dans hote/.
 *
 * Indications :
 *  - deux compteurs en millisecondes (sens opposé, encodeur muet), augmentés
 *    de dt_ms tant que la condition est vraie, remis à zéro sinon ;
 *  - « plus de 500 ms » : le défaut est levé quand le compteur DÉPASSE le
 *    délai, pas quand il l'atteint ;
 *  - deux comparateurs à hystérésis indépendants pour la température : chacun
 *    garde son état tant que la température est entre ses deux seuils.
 * ------------------------------------------------------------------------- */
#include "surv.h"

static uint32_t code;           /* DEFAUT_x mémorisé                         */
/* À COMPLÉTER : compteurs, états des deux comparateurs thermiques            */

void surv_init(void)
{
    code = DEFAUT_AUCUN;
    /* À COMPLÉTER */
}

void surv_step(const etat_vehicule_t *v, uint32_t sens_mesure, uint32_t fronts,
               int32_t temp_c10, uint32_t dt_ms)
{
    /* À COMPLÉTER */
    (void)v;
    (void)sens_mesure;
    (void)fronts;
    (void)temp_c10;
    (void)dt_ms;
}

uint32_t surv_code(void)
{
    return code;
}

void surv_acquitter(void)
{
    code = DEFAUT_AUCUN;
    /* À COMPLÉTER : compteurs à zéro */
}

uint32_t surv_thermique(void)
{
    /* À COMPLÉTER */
    return SURV_THERM_NORMAL;
}
