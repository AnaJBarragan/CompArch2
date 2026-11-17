/* ---------------------------------------------------------------------------
 * estop.s — arrêt d'urgence par interruption externe (CEG 3536, laboratoire 2)
 *           — CORRIGÉ (gestionnaire du laboratoire 1 étendu à la propulsion)
 *
 * Routines exportées : estop_init, EXTI2_IRQHandler
 * Variable (.bss)     : estop_flag
 *
 * Laboratoire 2 (E7) : l'ISR met elle-même la propulsion en sécurité
 * (motor_stop_now : CCR1 = 0, STB = 0, AI1 = AI2 = 0) et force le son continu
 * (buzzer_set, buzzer_continu = 1) avant de positionner estop_flag. Les deux
 * routines appelées n'utilisent que r0-r1 ; l'ISR sauvegarde LR (push {r4, lr},
 * 8 octets) et retourne par pop {r4, pc} (EXC_RETURN). Le changement complet
 * d'état vers ARRÊT_URGENCE reste dans fsm_step.
 * estop_init est inchangée par rapport au laboratoire 1.
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global estop_flag
estop_flag:     .space  4           /* 1 = E-Stop reçu, à consommer par fsm_step */

    .text
    .align  2

/* void estop_init(void)
 * Configure PB2 comme source de l'interruption EXTI2, priorité la plus élevée.
 *   1. EXTI_EXTICR1 : champ EXTI2 (bits 23:16, 8 bits) = EXTICR_PORT_B.
 *   2. Front correspondant à l'APPUI (BTN_ESTOP_ACTIF_HAUT) : RTSR1 ou FTSR1 ;
 *      l'autre front est désactivé ; requêtes en attente effacées.
 *   3. EXTI_IMR1 |= EXTI_LIGNE2.
 *   4. NVIC : priorité 0 (octet NVIC_IPR_BASE + 13), puis NVIC_ISER0 bit 13.
 * Registres modifiés : r0-r2 (routine feuille).                             */
    .global estop_init
    .type   estop_init, %function
estop_init:
    ldr     r0, =EXTI_BASE
    ldr     r1, [r0, #EXTI_EXTICR1]
    bic     r1, r1, #(0xFF << EXTICR1_EXTI2_POS)
    orr     r1, r1, #(EXTICR_PORT_B << EXTICR1_EXTI2_POS)
    str     r1, [r0, #EXTI_EXTICR1]
    mov     r2, #EXTI_LIGNE2
    .if BTN_ESTOP_ACTIF_HAUT
    ldr     r1, [r0, #EXTI_RTSR1]       /* appuyé = 1 : front montant */
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_RTSR1]
    ldr     r1, [r0, #EXTI_FTSR1]
    bic     r1, r1, r2
    str     r1, [r0, #EXTI_FTSR1]
    .else
    ldr     r1, [r0, #EXTI_FTSR1]       /* appuyé = 0 : front descendant */
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_FTSR1]
    ldr     r1, [r0, #EXTI_RTSR1]
    bic     r1, r1, r2
    str     r1, [r0, #EXTI_RTSR1]
    .endif
    str     r2, [r0, #EXTI_RPR1]        /* requêtes en attente effacées (w1c) */
    str     r2, [r0, #EXTI_FPR1]
    ldr     r1, [r0, #EXTI_IMR1]
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_IMR1]
    ldr     r0, =NVIC_IPR_BASE
    movs    r1, #(PRIO_ESTOP << PRIO_POS)
    strb    r1, [r0, #EXTI2_IRQn]
    ldr     r0, =NVIC_ISER0
    mov     r1, #(1 << EXTI2_IRQn)
    str     r1, [r0]
    bx      lr
    .size   estop_init, .-estop_init

/* void EXTI2_IRQHandler(void)   — vecteur d'interruption EXTI ligne 2
 * Remplace le gestionnaire faible (.weak) du fichier de démarrage.
 *   1. propulsion en sécurité : motor_stop_now (CCR1 = 0, STB = 0, AI1 = AI2 = 0) ;
 *   2. son continu : buzzer_set(1) et buzzer_continu = 1 ;
 *   3. DEL : verte et bleue éteintes, rouge allumée (BSRR) ;
 *   4. estop_flag = 1 ;
 *   5. effacement de la requête (RPR1 et FPR1, w1c) puis dsb.
 * Délai PB2 -> PE9 : quelques microsecondes (latence 12 cycles + ~15
 * instructions à 4 MHz), la PWM s'arrêtant au plus tard au débordement
 * suivant (50 µs) ; STB = 0 coupe le pilote immédiatement.                 */
    .global EXTI2_IRQHandler
    .type   EXTI2_IRQHandler, %function
EXTI2_IRQHandler:
    push    {r4, lr}
    bl      motor_stop_now              /* 1. propulsion coupée (r0-r1)      */
    movs    r0, #1
    bl      buzzer_set                  /* 2. son continu immédiat           */
    ldr     r0, =buzzer_continu
    movs    r1, #1
    str     r1, [r0]
    ldr     r0, =GPIOC_BASE             /* 3. DEL                            */
    mov     r1, #(1 << (LED_VERTE_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]
    ldr     r0, =GPIOB_BASE
    mov     r1, #(1 << (LED_BLEUE_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]
    ldr     r0, =GPIOA_BASE
    mov     r1, #(1 << LED_ROUGE_PIN)
    str     r1, [r0, #GPIO_BSRR]
    ldr     r0, =estop_flag             /* 4. drapeau                        */
    movs    r1, #1
    str     r1, [r0]
    ldr     r0, =EXTI_BASE              /* 5. requête effacée                */
    mov     r1, #EXTI_LIGNE2
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    pop     {r4, pc}
    .size   EXTI2_IRQHandler, .-EXTI2_IRQHandler
