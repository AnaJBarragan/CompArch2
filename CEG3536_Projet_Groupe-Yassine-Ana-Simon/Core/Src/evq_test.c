/* ---------------------------------------------------------------------------
 * evq_test.c — autotest de la file d'événements (CEG 3536, lab. 3, E11)
 *              — FOURNI (cas minimaux ; l'équipe en ajoute)
 * Aucune dépendance matérielle ni printf : compilable sur hôte et sur cible.
 * Cas : 1 vide ; 2 un élément ; 3 ordre FIFO ; 4 plein ; 5 débordement compté ;
 *       6 enroulement de l'index ; 7 vidage complet ; 8 compteur total ;
 *       9 pop sur pointeur nul.
 * Partie II (politique « écraser le plus ancien », E14) : les cas 5 et 8 sont
 * adaptés ; 11 E-Stop et défaut jamais perdus, ordre conservé ; 12 occupation
 * maximale (evq_max) et remise à zéro.
 * ------------------------------------------------------------------------- */
#include "evq.h"
#include "evq_test.h"

uint32_t evq_test_resultats[EVQ_TEST_N_CAS];

static uint32_t cas_ok(uint32_t i, int condition)
{
    evq_test_resultats[i] = condition ? 1u : 0u;
    return condition ? 0u : 1u;
}

uint32_t evq_autotest(void)
{
    uint32_t echecs = 0u;
    evq_event_t e;

    /* 1. vide */
    evq_init();
    echecs += cas_ok(0u, evq_count() == 0u && !evq_pop(&e) && evq_debordements() == 0u);

    /* 2. un élément */
    echecs += cas_ok(1u, evq_push(EV_USER, 42u, 7u) && evq_count() == 1u
                         && evq_pop(&e) && e.type == EV_USER && e.valeur == 42u
                         && e.t_ms == 7u && evq_count() == 0u);

    /* 3. ordre FIFO */
    {
        int ok = 1;
        for (uint32_t i = 0u; i < 5u; i++) ok = ok && evq_push(EV_TEST, i, i);
        for (uint32_t i = 0u; i < 5u; i++) ok = ok && evq_pop(&e) && e.valeur == i;
        echecs += cas_ok(2u, ok && evq_count() == 0u);
    }

    /* 4. plein : EVQ_CAPACITE push acceptés */
    {
        int ok = 1;
        for (uint32_t i = 0u; i < EVQ_CAPACITE; i++) ok = ok && evq_push(EV_TEST, i, 0u);
        echecs += cas_ok(3u, ok && evq_count() == EVQ_CAPACITE);
    }

    /* 5. débordement (partie II) : les deux plus anciens (0 et 1) sont écrasés,
     *    le débordement est signalé (false) et compté, la file reste pleine  */
    {
        int signale = !evq_push(EV_TEST, 999u, 0u) && !evq_push(EV_TEST, 998u, 0u);
        echecs += cas_ok(4u, signale && evq_debordements() == 2u
                             && evq_count() == EVQ_CAPACITE
                             && evq_pop(&e) && e.valeur == 2u);
    }

    /* 6. enroulement : après un pop, un push se place en fin et sort en dernier */
    {
        int ok = evq_push(EV_TEST, 777u, 0u);
        uint32_t derniere = 0u;
        while (evq_pop(&e)) derniere = e.valeur;
        echecs += cas_ok(5u, ok && derniere == 777u);
    }

    /* 7. vidage complet puis réutilisation */
    echecs += cas_ok(6u, evq_count() == 0u && evq_push(EV_ESTOP, 0u, 1u)
                         && evq_pop(&e) && e.type == EV_ESTOP && !evq_pop(&e));

    /* 8. compteur total des push acceptés (1 + 5 + 32 + 2 + 1 + 1 = 42) :
     *    à la partie II, les deux push du cas 5 sont acceptés (écrasement)   */
    echecs += cas_ok(7u, evq_total() == 42u);

    /* 9. pop avec pointeur nul : refusé sans effet */
    evq_push(EV_TEST, 5u, 0u);
    echecs += cas_ok(8u, !evq_pop((evq_event_t *)0) && evq_count() == 1u);

    /* 10. (équipe, E11) nouveaux types de la partie I : type, valeur et
     *     horodatage conservés, ordre d'arrivée respecté                    */
    {
        int ok = 1;
        evq_init();
        ok = ok && evq_push(EV_TOUCHE, 10u, 100u) && evq_push(EV_PIR, 1234u, 101u)
                && evq_push(EV_COMMUT, 8u, 102u) && evq_push(EV_ENV, 452u, 103u)
                && evq_push(EV_DEFAUT, 0x3Eu, 104u);
        ok = ok && evq_pop(&e) && e.type == EV_TOUCHE && e.valeur == 10u && e.t_ms == 100u;
        ok = ok && evq_pop(&e) && e.type == EV_PIR && e.valeur == 1234u;
        ok = ok && evq_pop(&e) && e.type == EV_COMMUT && e.valeur == 8u;
        ok = ok && evq_pop(&e) && e.type == EV_ENV && e.valeur == 452u;
        ok = ok && evq_pop(&e) && e.type == EV_DEFAUT && e.valeur == 0x3Eu && e.t_ms == 104u;
        echecs += cas_ok(9u, ok && evq_count() == 0u && (uint32_t)EV_TOUCHE == 8u
                             && (uint32_t)EV_DEFAUT == 12u);
    }

    /* 11. (partie II, L5-A) un E-Stop et un défaut en tête d'une file pleine
     *     ne sont jamais perdus : sauvés dans la réserve, lus en premier, dans
     *     leur ordre d'arrivée, puis les ordinaires restants dans le leur     */
    {
        int ok = 1;
        uint32_t attendu;
        evq_init();
        ok = ok && evq_push(EV_ESTOP, 0u, 1u) && evq_push(EV_DEFAUT, 0x103u, 2u);
        for (uint32_t i = 0u; i < EVQ_CAPACITE - 2u; i++) {
            ok = ok && evq_push(EV_TEST, i, 3u);            /* file pleine (32) */
        }
        for (uint32_t i = EVQ_CAPACITE - 2u; i < EVQ_CAPACITE + 8u; i++) {
            (void)evq_push(EV_TEST, i, 4u);                 /* 10 de trop       */
        }
        ok = ok && evq_debordements() == 8u;        /* 2 sauvés, 8 ordinaires perdus */
        ok = ok && evq_pop(&e) && e.type == EV_ESTOP && e.t_ms == 1u;
        ok = ok && evq_pop(&e) && e.type == EV_DEFAUT && e.valeur == 0x103u;
        attendu = 8u;                               /* EV_TEST 0 à 7 écrasés    */
        while (evq_pop(&e)) {
            ok = ok && e.type == EV_TEST && e.valeur == attendu;
            attendu++;
        }
        echecs += cas_ok(10u, ok && attendu == EVQ_CAPACITE + 8u);
    }

    /* 12. occupation maximale : 32 dans la file + 2 dans la réserve au pire du
     *     cas 11 ; evq_init remet tout à zéro                                */
    {
        int ok = evq_max() == EVQ_CAPACITE + 2u && evq_count() == 0u;
        evq_init();
        echecs += cas_ok(11u, ok && evq_max() == 0u && evq_debordements() == 0u
                              && evq_total() == 0u);
    }

    evq_init();
    return echecs;
}
