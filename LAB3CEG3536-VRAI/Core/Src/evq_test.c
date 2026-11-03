/* ---------------------------------------------------------------------------
 * evq_test.c — autotest de la file d'événements (CEG 3536, lab. 3, E11)
 *              — FOURNI (cas minimaux ; l'équipe en ajoute)
 * Aucune dépendance matérielle ni printf : compilable sur hôte et sur cible.
 * Cas : 1 vide ; 2 un élément ; 3 ordre FIFO ; 4 plein ; 5 débordement compté ;
 *       6 enroulement de l'index ; 7 vidage complet ; 8 compteur total ;
 *       9 pop sur pointeur nul.
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

    /* 5. débordement : push refusé et compté, contenu intact */
    {
        int refus = !evq_push(EV_TEST, 999u, 0u) && !evq_push(EV_TEST, 998u, 0u);
        echecs += cas_ok(4u, refus && evq_debordements() == 2u
                             && evq_count() == EVQ_CAPACITE
                             && evq_pop(&e) && e.valeur == 0u);
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

    /* 8. compteur total des push acceptés (1 + 5 + 32 + 1 + 1 = 40) */
    echecs += cas_ok(7u, evq_total() == 40u);

    /* 9. pop avec pointeur nul : refusé sans effet */
    evq_push(EV_TEST, 5u, 0u);
    echecs += cas_ok(8u, !evq_pop((evq_event_t *)0) && evq_count() == 1u);

    evq_init();
    return echecs;
}
