/* ---------------------------------------------------------------------------
 * regul.c — régulateur de vitesse PI en virgule fixe
 *           (CEG 3536, projet final, E17) — SQUELETTE À COMPLÉTER
 * Module sans matériel, testé sur hôte : make regul dans hote/ (le programme
 * d'essai simule le moteur par un modèle du premier ordre).
 *
 * Indications :
 *  - tous les calculs en int32_t ; la commande interne u_q8 vaut la commande
 *    en pour cent multipliée par 256 ; le résultat retourné est u_q8 / 256
 *    arrondi au plus proche ;
 *  - borner u_q8 APRÈS chaque mise à jour, entre 0 et plafond_pct x 256 :
 *    c'est ce qui empêche l'intégrale de s'emballer quand la consigne est
 *    hors d'atteinte (plafond abaissé, moteur chargé) ;
 *  - aucun type flottant.
 * ------------------------------------------------------------------------- */
#include "regul.h"

void regul_init(regul_t *r, int32_t kp_q8, int32_t ki_q8)
{
    r->kp_q8 = kp_q8;
    r->ki_q8 = ki_q8;
    regul_reset(r);
}

void regul_reset(regul_t *r)
{
    r->u_q8 = 0;
    r->e_prec = 0;
    r->actif = 0u;
}

uint32_t regul_step(regul_t *r, uint32_t consigne_rpm, uint32_t mesure_rpm, uint32_t plafond_pct)
{
    /* À COMPLÉTER */
    (void)r;
    (void)consigne_rpm;
    (void)mesure_rpm;
    (void)plafond_pct;
    return 0u;
}
