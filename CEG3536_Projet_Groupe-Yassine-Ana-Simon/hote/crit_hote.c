/* ---------------------------------------------------------------------------
 * crit_hote.c — équivalents C, pour l'hôte, des routines assembleur crit_entrer
 *               et crit_sortir (CEG 3536, projet final, L5-C) — FOURNI
 * Couche d'adaptation du portage : sur Windows et Linux, il n'y a ni PRIMASK
 * ni interruption. Les deux fonctions gardent la même interface (projet.h) et
 * simulent le registre par une variable, ce qui permet de vérifier sur hôte
 * que chaque crit_entrer est suivi de son crit_sortir (crit_hote_primask()
 * vaut 0 hors de toute section). Elles ne protègent RIEN : la concurrence
 * réelle ne s'essaie que sur la cible (commandes c et e).
 * ------------------------------------------------------------------------- */
#include "projet.h"

volatile uint32_t crit_max_cycles;      /* toujours 0 sur hôte (CRIT_CYCLES)   */
static uint32_t primask_simule;         /* 1 = « interruptions masquées »      */

uint32_t crit_entrer(void)
{
    uint32_t avant = primask_simule;
    primask_simule = 1u;
    return avant;
}

void crit_sortir(uint32_t etat)
{
    primask_simule = etat;
}

uint32_t crit_hote_primask(void)
{
    return primask_simule;
}
