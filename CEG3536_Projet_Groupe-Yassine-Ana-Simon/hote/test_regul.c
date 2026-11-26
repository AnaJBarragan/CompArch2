/* ---------------------------------------------------------------------------
 * test_regul.c — essai hôte du régulateur de vitesse (CEG 3536, projet final,
 *                E17) — FOURNI ; cas à ajouter par l'équipe
 * Compile sur Windows (MinGW/MSYS2) et Linux : make regul dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_regul test_regul.c \
 *       ../Core/Src/regul.c
 *
 * Le moteur est remplacé par un modèle du premier ordre, échantillonné toutes
 * les 100 ms :   v(k+1) = a x v(k) + (1 - a) x G x u(k)
 * avec G = 3,4 tr/min par point de PWM (340 tr/min à 100 %) et a = 0,51
 * (constante de temps d'environ 150 ms). Ce modèle est une approximation : il
 * sert à vérifier la logique du régulateur (départ, convergence, saturation,
 * emballement), pas à régler les gains, qui s'ajustent sur le banc.
 * Le modèle utilise le type double ; le régulateur, lui, reste en entiers.
 * ------------------------------------------------------------------------- */
#include "banc.h"
#include "regul.h"

#define PAS_2S  (2000u / REGUL_PERIODE_MS)      /* 20 pas = 2 secondes        */

static double vitesse;          /* vitesse simulée, tr/min                    */
static double gain = 3.4;       /* tr/min par point de PWM                    */
static unsigned u_max;          /* plus grande commande observée              */

static unsigned pas(regul_t *r, unsigned consigne, unsigned plafond)
{
    unsigned mesure = (unsigned)(vitesse + 0.5);
    unsigned u = regul_step(r, consigne, mesure, plafond);
    if (u > u_max) {
        u_max = u;
    }
    vitesse = 0.51 * vitesse + 0.49 * gain * (double)u;
    return u;
}

/* exécute n pas et retourne 1 si la vitesse finale est dans la tolérance */
static int converge(regul_t *r, unsigned consigne, unsigned plafond, unsigned n)
{
    for (unsigned i = 0u; i < n; i++) {
        pas(r, consigne, plafond);
    }
    double ecart = vitesse - (double)consigne;
    if (ecart < 0.0) {
        ecart = -ecart;
    }
    printf("      consigne %3u tr/min : vitesse %.1f apres %u pas (ecart %.1f %%)\n",
           consigne, vitesse, n, 100.0 * ecart / (double)consigne);
    return ecart * 100.0 <= (double)(REGUL_TOLERANCE_PCT * consigne);
}

int main(void)
{
    regul_t r, r2;
    unsigned u;

    printf("regul : Kp = %d/256, Ki = %d/256, pas de %u ms\n", REGUL_KP_Q8, REGUL_KI_Q8,
           (unsigned)REGUL_PERIODE_MS);
    regul_init(&r, REGUL_KP_Q8, REGUL_KI_Q8);

    vitesse = 0.0;
    u = regul_step(&r, 170u, 0u, 100u);
    VERIFIER(u == 50u, "premier pas : depart en boucle ouverte (170 tr/min -> 50 %)");

    u = regul_step(&r, 0u, 120u, 100u);
    VERIFIER(u == 0u, "consigne nulle : commande nulle");
    u = regul_step(&r, 170u, 0u, 100u);
    VERIFIER(u == 50u, "apres une consigne nulle : nouveau depart en boucle ouverte");

    /* trajet de référence : quatre consignes successives, 2 s chacune */
    regul_reset(&r);
    vitesse = 0.0;
    u_max = 0u;
    VERIFIER(converge(&r, 150u, 100u, PAS_2S), "150 tr/min : erreur <= 10 % apres 2 s");
    VERIFIER(converge(&r, 250u, 100u, PAS_2S), "250 tr/min : erreur <= 10 % apres 2 s");
    VERIFIER(converge(&r, 100u, 100u, PAS_2S), "100 tr/min : erreur <= 10 % apres 2 s");
    VERIFIER(converge(&r, 200u, 100u, PAS_2S), "200 tr/min : erreur <= 10 % apres 2 s");
    VERIFIER(u_max <= 100u, "la commande ne depasse jamais 100 %");

    /* plafond : consigne hors d'atteinte, puis retour à une consigne atteignable */
    regul_reset(&r);
    vitesse = 0.0;
    u_max = 0u;
    for (unsigned i = 0u; i < 50u; i++) {
        pas(&r, 250u, 30u);
    }
    VERIFIER(u_max <= 30u, "plafond de 30 % respecte pendant 5 s de saturation");
    u = pas(&r, 80u, 100u);
    VERIFIER(u < 30u, "apres la saturation : la commande redescend des le premier pas (pas d'emballement)");
    VERIFIER(converge(&r, 80u, 100u, PAS_2S), "80 tr/min apres saturation : erreur <= 10 % apres 2 s");

    /* moteur chargé : le gain réel est plus faible que prévu, l'intégrale compense */
    regul_reset(&r);
    vitesse = 0.0;
    gain = 2.8;
    VERIFIER(converge(&r, 200u, 100u, PAS_2S), "moteur charge (gain 2,8) : erreur <= 10 % apres 2 s");
    gain = 3.4;

    /* deux instances indépendantes (objet) */
    regul_init(&r2, REGUL_KP_Q8, REGUL_KI_Q8);
    regul_reset(&r);
    (void)regul_step(&r, 300u, 0u, 100u);
    (void)regul_step(&r, 300u, 0u, 100u);
    u = regul_step(&r2, 170u, 0u, 100u);
    VERIFIER(u == 50u, "deux instances : l'etat de l'une n'influence pas l'autre");

    /* ---- cas de l'équipe ---- */
    /* T14 : plafond abaissé au TouchPad pendant S2 (250 tr/min), puis relevé */
    regul_reset(&r);
    vitesse = 0.0;
    (void)converge(&r, 250u, 100u, PAS_2S);
    u_max = 0u;
    for (unsigned i = 0u; i < 20u; i++) {
        pas(&r, 250u, 50u);                     /* touche 5 : plafond 50 %    */
    }
    VERIFIER(u_max <= 50u && vitesse < 0.5 * 340.0 + 1.0,
             "plafond abaisse a 50 % en marche : commande et vitesse plafonnees");
    VERIFIER(converge(&r, 250u, 100u, PAS_2S), "plafond releve a 100 % : 250 tr/min rejoint en 2 s");

    /* bornes de la plage de consigne (80 et 300 tr/min) depuis l'arrêt */
    regul_reset(&r);
    vitesse = 0.0;
    VERIFIER(converge(&r, 300u, 100u, PAS_2S), "300 tr/min (borne haute) depuis l'arret : erreur <= 10 %");
    regul_reset(&r);
    vitesse = 0.0;
    VERIFIER(converge(&r, 80u, 100u, PAS_2S), "80 tr/min (borne basse) depuis l'arret : erreur <= 10 %");

    /* état interne borné : 50 pas saturés n'accumulent rien au-delà du plafond */
    regul_reset(&r);
    for (unsigned i = 0u; i < 50u; i++) {
        (void)regul_step(&r, 300u, 0u, 40u);    /* moteur bloqué, mesure nulle */
    }
    VERIFIER(r.u_q8 == 40 * 256, "moteur bloque : u_q8 reste exactement au plafond (anti-emballement)");

    /* intégration de main.c : la rampe du laboratoire 2 (1 point par 10 ms)
     * borne la commande appliquée, et main.c passe au régulateur le plafond
     * min(plafond, pwm appliquée + 10) : l'état interne ne devance jamais la
     * rampe de plus d'une fenêtre. Trajet de référence, 2 s par consigne.   */
    {
        static const unsigned c4[4] = { 150u, 250u, 100u, 200u };
        unsigned pwm_rampe = 0u, ok = 1u, depasse = 0u;
        regul_reset(&r);
        vitesse = 0.0;
        for (unsigned s = 0u; s < 4u; s++) {
            for (unsigned k = 0u; k < PAS_2S; k++) {
                unsigned borne = (pwm_rampe + 10u > 100u) ? 100u : pwm_rampe + 10u;
                unsigned cible = regul_step(&r, c4[s], (unsigned)(vitesse + 0.5), borne);
                for (unsigned m = 0u; m < 10u; m++) {
                    pwm_rampe += (pwm_rampe < cible) ? 1u : 0u;
                    pwm_rampe -= (pwm_rampe > cible) ? 1u : 0u;
                }
                vitesse = 0.51 * vitesse + 0.49 * gain * (double)pwm_rampe;
                if (s == 0u && vitesse > 1.10 * c4[0]) {
                    depasse = 1u;
                }
            }
            if ((vitesse - c4[s]) * (vitesse - c4[s]) > 0.01 * c4[s] * c4[s]) {
                ok = 0u;
            }
        }
        VERIFIER(ok && !depasse, "avec la rampe du lab. 2 et la borne pwm + 10 : quatre segments dans +/- 10 %, sans depassement au demarrage");
    }

    /* gains nuls : la commande reste celle de la boucle ouverte */
    regul_init(&r2, 0, 0);
    (void)regul_step(&r2, 204u, 0u, 100u);
    u = regul_step(&r2, 204u, 150u, 100u);
    VERIFIER(u == 60u, "Kp = Ki = 0 : commande figee a la boucle ouverte (204 tr/min -> 60 %)");

    return banc_bilan("test_regul");
}
