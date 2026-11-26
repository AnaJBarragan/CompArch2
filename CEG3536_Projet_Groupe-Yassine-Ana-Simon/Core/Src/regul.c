/* ---------------------------------------------------------------------------
 * regul.c — régulateur de vitesse PI en virgule fixe
 *           (CEG 3536, projet final, E17) — complété par l'équipe
 * Module sans matériel, testé sur hôte : make regul dans hote/ (le programme
 * d'essai simule le moteur par un modèle du premier ordre).
 *
 * Formats : u_q8 = commande en points de PWM x 256 (Q8) ; erreur en tr/min
 * (entier signé) ; gains en Q8 (256 = 1 point de PWM par tr/min).
 * Forme INCRÉMENTALE (énoncé 4.13), un pas toutes les 100 ms :
 *     e  = consigne - mesure
 *     u += Kp x (e - e_prec) + Ki x e
 *     u  borné entre 0 et plafond x 256
 * Le terme intégral n'existe pas comme variable séparée : il est contenu dans
 * u. En bornant u après chaque pas, on borne donc aussi l'intégrale : c'est
 * la protection contre l'emballement (anti-windup) exigée par E17.
 * Plages : |e| <= 340, |Kp x de| et |Ki x e| < 2^20 : aucun débordement en
 * int32_t. Aucun type flottant.
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
    int32_t e, u_max;

    if (plafond_pct > 100u) {
        plafond_pct = 100u;
    }
    if (consigne_rpm == 0u) {                   /* arrêt demandé            */
        regul_reset(r);
        return 0u;
    }
    u_max = (int32_t)plafond_pct * 256;
    e = (int32_t)consigne_rpm - (int32_t)mesure_rpm;

    if (!r->actif) {
        /* premier pas : commande attendue en boucle ouverte, sans P ni I    */
        r->u_q8 = (int32_t)((consigne_rpm * 100u * 256u) / VITESSE_MAX_RPM);
        r->actif = 1u;
    } else {
        r->u_q8 += r->kp_q8 * (e - r->e_prec) + r->ki_q8 * e;
    }
    r->e_prec = e;

    if (r->u_q8 > u_max) {                      /* saturation = anti-windup */
        r->u_q8 = u_max;
    } else if (r->u_q8 < 0) {
        r->u_q8 = 0;
    }
    return (uint32_t)((r->u_q8 + 128) >> 8);    /* arrondi au point le plus proche */
}
