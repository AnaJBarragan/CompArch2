/* ---------------------------------------------------------------------------
 * buzzer.s — avertisseur sonore sur PA0, commande GPIO (CEG 3536, lab. 2)
 *            — CORRIGÉ
 *
 * Routines exportées : buzzer_init, buzzer_set, buzzer_bip, buzzer_step
 * Variables (.bss)   : buzzer_continu, buzzer_phase, buzzer_restant, buzzer_bips
 *
 * Le module du guide (p. 36) produit un son constant quand PA0 = 1. Les bips
 * (E9) sont une petite machine à états cadencée par buzzer_step, appelée
 * toutes les 1 ms par fsm_step : rien ne bloque.
 *   phase 0 : silence (repos)
 *   phase 1 : son pendant buzzer_restant ms, puis phase 2 s'il reste des bips
 *   phase 2 : silence BIP_SILENCE_MS, puis phase 1 (bips--)
 * buzzer_continu = 1 force le son quelle que soit la phase (ARRÊT_URGENCE) ;
 * il est positionné par EXTI2_IRQHandler et remis à 0 à l'acquittement.
 * Variante PWM (TIM2_CH1, AF1) admise si documentée : non retenue ici.
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global buzzer_continu
buzzer_continu: .space  4   /* 1 = son continu forcé                           */
buzzer_phase:   .space  4   /* 0 repos, 1 son, 2 silence entre deux bips       */
buzzer_restant: .space  4   /* ms restantes dans la phase courante             */
buzzer_bips:    .space  4   /* bips restant à produire après le bip courant    */

    .text
    .align  2

/* void buzzer_init(void)
 * Horloge GPIOA (déjà active), PA0 en sortie push-pull, niveau 0, variables à 0.
 * Registres modifiés : r0, r1 (routine feuille).                            */
    .global buzzer_init
    .type   buzzer_init, %function
buzzer_init:
    ldr     r0, =RCC_BASE
    ldr     r1, [r0, #RCC_AHB2ENR]
    orr     r1, r1, #RCC_AHB2ENR_GPIOAEN
    str     r1, [r0, #RCC_AHB2ENR]
    ldr     r1, [r0, #RCC_AHB2ENR]
    ldr     r0, =GPIOA_BASE
    mov     r1, #(1 << (BUZZER_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]        /* silence avant le passage en sortie */
    ldr     r1, [r0, #GPIO_MODER]
    bic     r1, r1, #(3 << (2 * BUZZER_PIN))
    orr     r1, r1, #(GPIO_MODE_SORTIE << (2 * BUZZER_PIN))
    str     r1, [r0, #GPIO_MODER]
    ldr     r1, [r0, #GPIO_OTYPER]
    bic     r1, r1, #(1 << BUZZER_PIN)
    str     r1, [r0, #GPIO_OTYPER]
    ldr     r1, [r0, #GPIO_PUPDR]
    bic     r1, r1, #(3 << (2 * BUZZER_PIN))
    str     r1, [r0, #GPIO_PUPDR]
    movs    r1, #0
    ldr     r0, =buzzer_continu
    str     r1, [r0]
    ldr     r0, =buzzer_phase
    str     r1, [r0]
    ldr     r0, =buzzer_restant
    str     r1, [r0]
    ldr     r0, =buzzer_bips
    str     r1, [r0]
    bx      lr
    .size   buzzer_init, .-buzzer_init

/* void buzzer_set(uint32_t on)
 * r0 = 0 silence, autre : son. Écriture BSRR atomique ; r0-r1 seulement :
 * appelable depuis EXTI2_IRQHandler.
 * Registres modifiés : r1 (r0 conservé).                                    */
    .global buzzer_set
    .type   buzzer_set, %function
buzzer_set:
    ldr     r1, =GPIOA_BASE
    cmp     r0, #0
    beq     buzzer_set_off
    mov     r0, #(1 << BUZZER_PIN)
    str     r0, [r1, #GPIO_BSRR]
    bx      lr
buzzer_set_off:
    mov     r0, #(1 << (BUZZER_PIN + 16))
    str     r0, [r1, #GPIO_BSRR]
    bx      lr
    .size   buzzer_set, .-buzzer_set

/* void buzzer_bip(uint32_t n)
 * Lance n bips de BIP_MS séparés de BIP_SILENCE_MS (n = 1 : bip court,
 * n = 2 : double bip). Une séquence en cours est remplacée. n = 0 : rien.
 * Le son commence au prochain buzzer_step (au plus 1 ms plus tard).
 * Registres modifiés : r0-r2 (routine feuille).                             */
    .global buzzer_bip
    .type   buzzer_bip, %function
buzzer_bip:
    cmp     r0, #0
    beq     buzzer_bip_fin
    subs    r0, r0, #1
    ldr     r1, =buzzer_bips
    str     r0, [r1]                    /* bips restant après celui-ci       */
    ldr     r1, =buzzer_restant
    movs    r2, #BIP_MS
    str     r2, [r1]
    ldr     r1, =buzzer_phase
    movs    r2, #1
    str     r2, [r1]
buzzer_bip_fin:
    bx      lr
    .size   buzzer_bip, .-buzzer_bip

/* void buzzer_step(void)
 * À appeler toutes les 1 ms. Calcule la sortie : son continu forcé, sinon
 * phase 1 = son, phases 0 et 2 = silence ; décrémente buzzer_restant et
 * enchaîne les phases. Appelle buzzer_set : push {r4, lr} (alignement 8).
 * r4 = &buzzer_phase.                                                       */
    .global buzzer_step
    .type   buzzer_step, %function
buzzer_step:
    push    {r4, lr}
    ldr     r0, =buzzer_continu
    ldr     r0, [r0]
    cmp     r0, #0
    bne     buzzer_step_son             /* son continu : ignore les bips     */
    ldr     r4, =buzzer_phase
    ldr     r0, [r4]
    cmp     r0, #0
    beq     buzzer_step_silence         /* repos                             */
    /* phase 1 ou 2 : décompte */
    ldr     r1, =buzzer_restant
    ldr     r2, [r1]
    subs    r2, r2, #1
    str     r2, [r1]
    bne     buzzer_step_sortie          /* phase inchangée                   */
    /* fin de la phase courante */
    cmp     r0, #1
    bne     buzzer_step_fin_silence
    ldr     r1, =buzzer_bips            /* fin d'un bip : reste-t-il des bips ? */
    ldr     r2, [r1]
    cmp     r2, #0
    beq     buzzer_step_repos
    subs    r2, r2, #1
    str     r2, [r1]
    movs    r0, #2                      /* -> silence entre bips             */
    str     r0, [r4]
    ldr     r1, =buzzer_restant
    movs    r2, #BIP_SILENCE_MS
    str     r2, [r1]
    b       buzzer_step_silence
buzzer_step_fin_silence:
    movs    r0, #1                      /* -> bip suivant                    */
    str     r0, [r4]
    ldr     r1, =buzzer_restant
    movs    r2, #BIP_MS
    str     r2, [r1]
    b       buzzer_step_son
buzzer_step_repos:
    movs    r0, #0
    str     r0, [r4]
    b       buzzer_step_silence
buzzer_step_sortie:
    cmp     r0, #1
    bne     buzzer_step_silence
buzzer_step_son:
    movs    r0, #1
    bl      buzzer_set
    pop     {r4, pc}
buzzer_step_silence:
    movs    r0, #0
    bl      buzzer_set
    pop     {r4, pc}
    .size   buzzer_step, .-buzzer_step
