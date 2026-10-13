/* ---------------------------------------------------------------------------
 * estop.s — arrêt d'urgence par interruption externe (CEG 3536, laboratoire 1)
 *           — CORRIGÉ
 *
 * Routines exportées : estop_init, EXTI2_IRQHandler
 * Variable (.bss)     : estop_flag
 *
 * Règle des interruptions (section 4.1) : l'ISR est courte. Exception exigée
 * pour E-Stop : l'ISR met d'abord les sorties en état sûr (verte et bleue
 * éteintes, rouge allumée), positionne estop_flag, efface la requête EXTI et
 * se termine. Le changement complet d'état vers ARRÊT_URGENCE est fait par
 * fsm_step dans la boucle principale.
 *
 * Particularité STM32L5 (RM0438, section 16) : les registres de sélection de
 * port EXTICR sont dans EXTI (EXTI_EXTICR1, offset 0x60), et non dans SYSCFG
 * comme sur les STM32F4/L4. La requête se lit et s'efface dans EXTI_RPR1
 * (front montant) et EXTI_FPR1 (front descendant), par écriture de 1.
 * ------------------------------------------------------------------------- */
#include "registres.inc"

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
    /* 1. ligne 2 <- port B */
    ldr     r1, [r0, #EXTI_EXTICR1]
    bic     r1, r1, #(0xFF << EXTICR1_EXTI2_POS)
    orr     r1, r1, #(EXTICR_PORT_B << EXTICR1_EXTI2_POS)
    str     r1, [r0, #EXTI_EXTICR1]
    /* 2. front d'appui */
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
    /* 3. démasquer la ligne 2 */
    ldr     r1, [r0, #EXTI_IMR1]
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_IMR1]
    /* 4. NVIC : priorité la plus élevée, puis activation */
    ldr     r0, =NVIC_IPR_BASE
    movs    r1, #NVIC_PRIO_MAX
    strb    r1, [r0, #EXTI2_IRQn]
    ldr     r0, =NVIC_ISER0
    mov     r1, #(1 << EXTI2_IRQn)
    str     r1, [r0]
    bx      lr
    .size   estop_init, .-estop_init

/* void EXTI2_IRQHandler(void)   — vecteur d'interruption EXTI ligne 2
 * Remplace le gestionnaire faible (.weak) du fichier de démarrage.
 * Cause : ce vecteur est dédié à la ligne 2 (PB2, E-Stop) ; la cause est donc
 * connue sans lire RPR1/FPR1, et seule la requête de la ligne 2 est effacée.
 * Le matériel empile r0-r3, r12, lr, pc, xPSR : l'ISR utilise r0-r1 sans
 * sauvegarde et retourne par bx lr (EXC_RETURN).
 *   1. état sûr immédiat par BSRR : verte et bleue éteintes, rouge allumée ;
 *   2. estop_flag = 1 ;
 *   3. effacement de la requête (RPR1 et FPR1, w1c) puis dsb pour que
 *      l'écriture soit effective avant le retour (évite une seconde entrée). */
    .global EXTI2_IRQHandler
    .type   EXTI2_IRQHandler, %function
EXTI2_IRQHandler:
    ldr     r0, =GPIOC_BASE
    mov     r1, #(1 << (LED_VERTE_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]        /* verte éteinte */
    ldr     r0, =GPIOB_BASE
    mov     r1, #(1 << (LED_BLEUE_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]        /* bleue éteinte */
    ldr     r0, =GPIOA_BASE
    mov     r1, #(1 << LED_ROUGE_PIN)
    str     r1, [r0, #GPIO_BSRR]        /* rouge allumée */
    ldr     r0, =estop_flag
    movs    r1, #1
    str     r1, [r0]
    ldr     r0, =EXTI_BASE
    mov     r1, #EXTI_LIGNE2
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    bx      lr
    .size   EXTI2_IRQHandler, .-EXTI2_IRQHandler
