/* ---------------------------------------------------------------------------
 * surv.c — surveillances : sens, encodeur muet, protection thermique
 *          (CEG 3536, projet final, E18, E19) — complété par l'équipe
 * Module sans matériel, testé sur hôte : make surv dans hote/.
 *
 * Deux compteurs en millisecondes (sens opposé, encodeur muet), augmentés de
 * dt_ms tant que leur condition est vraie, remis à zéro dès qu'elle disparaît
 * ou hors de la marche. Le défaut est levé quand un compteur DÉPASSE son
 * délai (> 500 ms, > 1000 ms). Le premier défaut levé est MÉMORISÉ jusqu'à
 * surv_acquitter() : quand le moteur s'arrête, la condition disparaît
 * d'elle-même, mais le motif doit rester affiché.
 * Thermique : deux comparateurs à hystérésis INDÉPENDANTS (énoncé 4.13) ;
 * chacun garde son état tant que la température est entre ses deux seuils.
 * ------------------------------------------------------------------------- */
#include "surv.h"

#define COMPTEUR_MAX    0x7FFFFFFFu     /* saturation : jamais de retour à 0 */

static uint32_t code;           /* DEFAUT_x mémorisé                         */
static uint32_t t_sens;         /* durée du sens opposé, ms                  */
static uint32_t t_muet;         /* durée sans front au-delà de 30 %, ms      */
static uint32_t limitation;     /* comparateur 40,0 / 37,0 °C                */
static uint32_t arret;          /* comparateur 50,0 / 45,0 °C                */

static uint32_t cumuler(uint32_t t, uint32_t dt)
{
    return (t < COMPTEUR_MAX - dt) ? t + dt : COMPTEUR_MAX;
}

void surv_init(void)
{
    code = DEFAUT_AUCUN;
    t_sens = t_muet = 0u;
    limitation = arret = 0u;
}

void surv_step(const etat_vehicule_t *v, uint32_t sens_mesure, uint32_t fronts,
               int32_t temp_c10, uint32_t dt_ms)
{
    bool marche = (v->etat == ETAT_MARCHE_AVANT || v->etat == ETAT_MARCHE_ARRIERE);
    uint32_t oppose = (v->etat == ETAT_MARCHE_AVANT) ? SENS_ARRIERE : SENS_AVANT;

    /* sens mesuré opposé au sens commandé (SENS_IMMOBILE n'est pas opposé) */
    if (marche && sens_mesure == oppose) {
        t_sens = cumuler(t_sens, dt_ms);
        if (t_sens > SURV_SENS_MS && code == DEFAUT_AUCUN) {
            code = DEFAUT_SENS;
        }
    } else {
        t_sens = 0u;
    }

    /* encodeur muet : aucun front alors que la commande dépasse 30 % */
    if (marche && v->pwm_pct > SURV_MUET_PWM_PCT && fronts == 0u) {
        t_muet = cumuler(t_muet, dt_ms);
        if (t_muet > SURV_MUET_MS && code == DEFAUT_AUCUN) {
            code = DEFAUT_ENCODEUR;
        }
    } else {
        t_muet = 0u;
    }

    /* protection thermique : deux comparateurs à hystérésis */
    if (!limitation && temp_c10 >= TEMP_LIMITE_C10) {
        limitation = 1u;
    } else if (limitation && temp_c10 <= TEMP_LIMITE_FIN_C10) {
        limitation = 0u;
    }
    if (!arret && temp_c10 >= TEMP_ARRET_C10) {
        arret = 1u;
    } else if (arret && temp_c10 <= TEMP_ARRET_FIN_C10) {
        arret = 0u;
    }
}

uint32_t surv_code(void)
{
    return code;
}

void surv_acquitter(void)
{
    code = DEFAUT_AUCUN;
    t_sens = t_muet = 0u;
}

uint32_t surv_thermique(void)
{
    if (arret) {
        return SURV_THERM_ARRET;
    }
    return limitation ? SURV_THERM_LIMITATION : SURV_THERM_NORMAL;
}
