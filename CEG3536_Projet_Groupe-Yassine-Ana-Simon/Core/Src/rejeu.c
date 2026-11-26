/* ---------------------------------------------------------------------------
 * rejeu.c — rejeu d'une trace de télémétrie (CEG 3536, projet final, L5-C)
 *           — ajout de l'équipe
 * Aucun registre, aucun printf, aucun type flottant : même source sur les
 * trois environnements. Les fronts de chaque intervalle entre deux trames
 * sont reconstitués à partir de la vitesse enregistrée :
 *     fronts = vitesse_rpm_x100 x 748 x dt_ms / (100 x 60 000)
 * en 64 bits, avec report du reste d'une trame à l'autre (aucune dérive).
 * Les décisions dépendent donc de la vitesse mesurée à 5 Hz, pas de chaque
 * front : le rejeu ne reproduit qu'approximativement le trajet réel (la
 * concordance des segments le mesure), mais il doit reproduire EXACTEMENT
 * les mêmes décisions sur Windows, Linux et STM32.
 * ------------------------------------------------------------------------- */
#include "rejeu.h"
#include "trajet.h"
#include "regul.h"
#include "surv.h"

#define DIVISEUR_FRONTS     6000000u        /* 100 x 60 000 ms             */
#define DT_MAX_MS           1000u           /* trame manquée : intervalle borné */
#define FNV_DEPART          2166136261u
#define FNV_PREMIER         16777619u

static char *fmt_u32(char *p, uint32_t v)
{
    char tmp[10];
    uint32_t n = 0u;

    do {
        tmp[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (n != 0u) {
        *p++ = tmp[--n];
    }
    return p;
}

void rejeu_executer(const rejeu_trame_t *t, uint32_t n, const segment_t *s, uint32_t n_seg,
                    rejeu_sortie_t sortie, rejeu_bilan_t *b)
{
    regul_t r;
    uint64_t reste = 0u;
    etat_vehicule_t v = { 0u };

    trajet_init(s, n_seg);
    surv_init();
    regul_init(&r, REGUL_KP_Q8, REGUL_KI_Q8);
    b->n = 0u;
    b->somme = FNV_DEPART;
    b->concordants = 0u;
    b->defauts = 0u;

    for (uint32_t k = 0u; k < n; k++) {
        uint32_t dt = (k == 0u) ? 0u : t[k].ticks_ms - t[k - 1u].ticks_ms;
        uint64_t produit;
        uint32_t fronts, consigne = 0u, commande = 0u;
        bool marche = (t[k].etat == ETAT_MARCHE_AVANT || t[k].etat == ETAT_MARCHE_ARRIERE);
        char ligne[64];
        char *p = ligne;

        if (dt > DT_MAX_MS) {
            dt = DT_MAX_MS;
        }
        produit = (uint64_t)t[k].vitesse_rpm_x100 * FRONTS_PAR_TOUR * dt + reste;
        fronts = (uint32_t)(produit / DIVISEUR_FRONTS);
        reste = produit % DIVISEUR_FRONTS;

        v.etat = t[k].etat;
        v.pwm_pct = t[k].pwm_pct;
        if (marche && t[k].mode == MODE_AUTONOME) {
            uint32_t plafond_rpm = (uint32_t)t[k].plafond_pct * VITESSE_MAX_RPM / 100u;
            trajet_avancer(fronts);
            consigne = trajet_consigne_rpm();
            if (consigne > plafond_rpm) {
                consigne = plafond_rpm;
            }
            commande = regul_step(&r, consigne, t[k].vitesse_rpm_x100 / 100u, t[k].plafond_pct);
        } else {
            regul_reset(&r);
        }
        surv_step(&v, t[k].sens, fronts, t[k].temp_c_x10, dt);

        if (t[k].mode == MODE_AUTONOME && trajet_segment() == t[k].segment) {
            b->concordants++;
        } else if (t[k].mode != MODE_AUTONOME && t[k].segment == 0u) {
            b->concordants++;
        }
        if (surv_code() != DEFAUT_AUCUN) {
            b->defauts++;
        }

        /* ligne de décision, identique sur les trois environnements */
        p = fmt_u32(p, k);                    *p++ = ',';
        p = fmt_u32(p, t[k].ticks_ms);        *p++ = ',';
        p = fmt_u32(p, (t[k].mode == MODE_AUTONOME) ? trajet_segment() : 0u); *p++ = ',';
        p = fmt_u32(p, consigne);             *p++ = ',';
        p = fmt_u32(p, commande);             *p++ = ',';
        p = fmt_u32(p, surv_code());          *p++ = ',';
        p = fmt_u32(p, surv_thermique());     *p++ = '\n';
        *p = '\0';
        for (const char *c = ligne; *c != '\0'; c++) {
            b->somme = (b->somme ^ (uint8_t)*c) * FNV_PREMIER;
        }
        if (sortie != 0) {
            sortie(ligne);
        }
        b->n++;
    }
    b->segment_final = trajet_segment();
    b->dist_x10_final = trajet_dist_x10();
}
