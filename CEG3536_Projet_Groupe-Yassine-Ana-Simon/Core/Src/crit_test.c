/* ---------------------------------------------------------------------------
 * crit_test.c — banc des routines assembleur crit_entrer / crit_sortir et
 *               rafale d'événements depuis une interruption
 *               (CEG 3536, projet final, E13, E14, T11) — FOURNI
 *
 * Cinq cas (crit_test_resultats, à relever aussi au débogueur) :
 *   1. crit_entrer, interruptions démasquées : retourne 0 et PRIMASK passe à 1 ;
 *   2. crit_sortir(0) : PRIMASK revient à 0 ;
 *   3. imbrication : le second crit_entrer retourne 1 et la sortie de la
 *      section intérieure laisse les interruptions masquées ;
 *   4. sortie de la section extérieure : PRIMASK revient à 0 ;
 *   5. appel dans un contexte déjà masqué (cas d'un gestionnaire qui a masqué) :
 *      crit_entrer retourne 1 et crit_sortir(1) ne démasque pas.
 * Le banc rétablit PRIMASK = 0 dans tous les cas, même si une routine est
 * fausse : il ne peut pas laisser le véhicule interruptions masquées.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "projet.h"
#include "evq.h"
#include "crit_test.h"

volatile uint32_t crit_max_cycles;          /* maximum relevé par CRIT_MESURER */
uint32_t crit_test_resultats[CRIT_TEST_N_CAS];

uint32_t crit_autotest(void)
{
    uint32_t a, b, echecs = 0u;

    __enable_irq();                                     /* état de départ connu */

    a = crit_entrer();
    crit_test_resultats[0] = (a == 0u && __get_PRIMASK() == 1u);
    crit_sortir(a);
    crit_test_resultats[1] = (__get_PRIMASK() == 0u);
    __enable_irq();

    a = crit_entrer();
    b = crit_entrer();
    crit_sortir(b);
    crit_test_resultats[2] = (b == 1u && __get_PRIMASK() == 1u);
    crit_sortir(a);
    crit_test_resultats[3] = (__get_PRIMASK() == 0u);
    __enable_irq();

    __disable_irq();
    a = crit_entrer();
    crit_sortir(a);
    crit_test_resultats[4] = (a == 1u && __get_PRIMASK() == 1u);
    __enable_irq();

    for (uint32_t i = 0u; i < CRIT_TEST_N_CAS; i++) {
        if (!crit_test_resultats[i]) {
            echecs++;
        }
    }
    return echecs;
}

/* ---- Rafale depuis un gestionnaire (E14) -------------------------------- */
/* La ligne EXTI15 n'est reliée à aucun signal du véhicule : son interruption
 * est déclenchée par logiciel (NVIC_SetPendingIRQ). Priorité 3 : sous E-Stop
 * (0), l'encodeur (1) et SysTick (2). Le gestionnaire pousse les événements
 * un à un : la boucle principale, interrompue au milieu d'un evq_pop, voit
 * alors la file modifiée par un autre contexte.                             */
#define CRIT_RAFALE_IRQn    EXTI15_IRQn
#define CRIT_RAFALE_PRIO    3u

static volatile uint32_t rafale_reste;
volatile uint32_t crit_rafale_refus;

void crit_rafale_init(void)
{
    NVIC_SetPriority(CRIT_RAFALE_IRQn, CRIT_RAFALE_PRIO);
    NVIC_EnableIRQ(CRIT_RAFALE_IRQn);
}

void crit_rafale(uint32_t n)
{
    rafale_reste = n;
    NVIC_SetPendingIRQ(CRIT_RAFALE_IRQn);
}

void EXTI15_IRQHandler(void)
{
    /* quatre événements par passage, puis nouvelle demande : la boucle
     * principale reprend la main entre deux passages et consomme la file    */
    for (uint32_t i = 0u; i < 4u && rafale_reste != 0u; i++) {
        rafale_reste--;
        if (!evq_push(EV_TEST, rafale_reste, ticks_ms)) {
            crit_rafale_refus++;
        }
    }
    if (rafale_reste != 0u) {
        NVIC_SetPendingIRQ(CRIT_RAFALE_IRQn);
    }
}
