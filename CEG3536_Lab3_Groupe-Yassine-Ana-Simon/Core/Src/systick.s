/* ---------------------------------------------------------------------------
 * systick.s — base de temps de 1 ms par SysTick (CEG 3536, laboratoire 2)
 *             — SQUELETTE À COMPLÉTER
 *
 * Routines exportées : systick_init, SysTick_Handler, delay_ms
 * Variables (.bss)   : ticks_ms, tick_1ms
 *
 * delay_ms remplace la boucle calibrée du laboratoire 1 : SUPPRIMER l'ancienne
 * version de buttons.s (sinon : symbole défini deux fois à l'édition de liens).
 * Même interface : r0 = durée en ms. Elle n'est plus appelée dans la boucle
 * principale, cadencée par tick_1ms (E10).
 *
 * Fourni : SysTick_Handler. À écrire : systick_init et delay_ms.
 * Tant que systick_init n'est pas écrite, SysTick n'interrompt jamais et la
 * boucle principale attend tick_1ms indéfiniment : c'est la première tâche
 * de la séance 1 (la DEL rouge de fsm_init s'allume quand même : E1).
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global ticks_ms
ticks_ms:       .space  4   /* millisecondes écoulées (déborde après 49,7 jours) */
    .global tick_1ms
tick_1ms:       .space  4   /* 1 à chaque interruption ; consommé par main      */

    .text
    .align  2

/* void systick_init(uint32_t periode_us)
 * r0 = période en microsecondes (1000 pour 1 ms).
 *   LOAD = periode_us * (HORLOGE_HZ / 1 000 000) - 1 = 4 * r0 - 1 (3999 à 1 ms)
 *   VAL  = 0 (le compteur repart de LOAD)
 *   ticks_ms = tick_1ms = 0
 *   priorité SysTick = PRIO_SYSTICK << PRIO_POS dans l'octet SCB_SHPR3_SYSTICK
 *   (strb : un seul octet, ne pas écraser PendSV)
 *   CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE
 * Vérifier au débogueur : SYSTICK_LOAD = 0xF9F, SYSTICK_CTRL = 0x7 (bit 16
 * COUNTFLAG peut apparaître), SCB_SHPR3 = 0x40000000, ticks_ms qui avance.
 * Registres modifiés : r0-r2 (routine feuille). À COMPLÉTER (E10).          */
    .global systick_init
    .type   systick_init, %function
systick_init:
    /* À COMPLÉTER */

    /* code complete */
    movs    r1, #(HORLOGE_HZ / 1000000)     /* 4 cycles par microseconde      */
    mul     r0, r0, r1                      /* r0 = periode_us * 4            */
    subs    r0, r0, #1                      /* LOAD = N - 1 (3999 à 1 ms)     */
    ldr     r1, =SYSTICK_LOAD
    str     r0, [r1]
    movs    r0, #0
    ldr     r1, =SYSTICK_VAL
    str     r0, [r1]                        /* le compteur repart de LOAD     */
    ldr     r1, =ticks_ms
    str     r0, [r1]
    ldr     r1, =tick_1ms
    str     r0, [r1]
    movs    r2, #(PRIO_SYSTICK << PRIO_POS) /* 2 << 5 = 0x40                  */
    ldr     r1, =SCB_SHPR3_SYSTICK
    strb    r2, [r1]                        /* un octet : PendSV intact       */
    movs    r2, #(SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE)
    ldr     r1, =SYSTICK_CTRL
    str     r2, [r1]                        /* démarre : en dernier           */
    /* Fin de section complete */

    bx      lr
    .size   systick_init, .-systick_init

/* void SysTick_Handler(void)   — exception SysTick (toutes les 1 ms)
 * ticks_ms++ ; tick_1ms = 1 ; rien d'autre (E10). r0-r1 empilés par le
 * matériel. FOURNIE.                                                        */
    .global SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    ldr     r0, =ticks_ms
    ldr     r1, [r0]
    adds    r1, r1, #1
    str     r1, [r0]
    ldr     r0, =tick_1ms
    movs    r1, #1
    str     r1, [r0]
    bx      lr
    .size   SysTick_Handler, .-SysTick_Handler

/* void delay_ms(uint32_t ms)
 * Attend que ticks_ms ait avancé de r0 : lire ticks_ms au départ (r1), puis
 * boucler tant que (ticks_ms - r1) < r0 en NON SIGNÉ (subs puis cmp/bhs) :
 * correct même si ticks_ms déborde. Précision : -1/+0 ms. Requiert
 * systick_init et interruptions activées (jamais depuis une ISR).
 * Registres modifiés : r1, r2 (r0 conservé ; routine feuille).
 * À COMPLÉTER (E10). Le squelette retourne immédiatement.                   */
    .global delay_ms
    .type   delay_ms, %function
delay_ms:
    /* À COMPLÉTER */
    
    /* code complete */
    ldr     r2, =ticks_ms
    ldr     r1, [r2]                        /* r1 = ticks_ms au départ        */
delay_ms_attente:
    ldr     r2, =ticks_ms
    ldr     r2, [r2]                        /* r2 = ticks_ms courant          */
    subs    r2, r2, r1                      /* écoulé, modulo 2^32            */
    cmp     r2, r0
    blo     delay_ms_attente                /* non signé : tant que écoulé < ms */
    /* fin de section a completer */

    bx      lr
    .size   delay_ms, .-delay_ms
