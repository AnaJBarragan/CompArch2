/* ---------------------------------------------------------------------------
 * encoder.s — lecture de l'encodeur en quadrature par interruption EXTI0
 *             (CEG 3536, laboratoire 2) — CORRIGÉ
 *
 * Routines exportées : encoder_init, EXTI0_IRQHandler, encoder_read
 * Variables (.bss)   : enc_count (signé), enc_repere, vitesse_fronts, sens_mesure
 *
 * Principe (E5) : les deux fronts de la voie A (PB0) déclenchent EXTI0. À
 * chaque front, le niveau de B (PB1) par rapport à A donne le sens :
 *   front montant de A  : B = 0 -> sens 1 ; B = 1 -> sens 2
 *   front descendant de A : B = 1 -> sens 1 ; B = 0 -> sens 2
 * soit, dans les deux cas : A != B -> sens 1, A = B -> sens 2. Le corrigé
 * compte +1 pour « A = B » et -1 pour « A != B », puis ENC_SENS_INVERSE
 * (registres_lab2.inc) aligne ce comptage sur la marche avant commandée.
 * 748 fronts par tour de l'arbre de sortie (2 fronts x 11 x 34).
 *
 * E6 : encoder_read, appelée toutes les FENETRE_VITESSE_MS par fsm_step,
 * calcule la variation de enc_count depuis la lecture précédente.
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global enc_count
enc_count:      .space  4   /* fronts comptés, signé (ISR : +1 / -1)          */
enc_repere:     .space  4   /* valeur de enc_count à la lecture précédente     */
    .global vitesse_fronts
vitesse_fronts: .space  4   /* |variation| sur la dernière fenêtre (T6, watch) */
    .global sens_mesure
sens_mesure:    .space  4   /* SENS_IMMOBILE / SENS_AVANT / SENS_ARRIERE       */

    .text
    .align  2

/* void encoder_init(void)
 *   1. horloge GPIOB (déjà active : gpio_init) ; PB0 et PB1 en entrée,
 *      PUPDR = ENC_PUPDR ;
 *   2. EXTI_EXTICR1 champ EXTI0 (bits 7:0) = port B ;
 *   3. RTSR1 et FTSR1 bit 0 (les deux fronts) ; requêtes en attente effacées ;
 *   4. IMR1 bit 0 ;
 *   5. NVIC : priorité PRIO_ENCODEUR (octet NVIC_IPR_BASE + 11), ISER0 bit 11 ;
 *   6. variables à zéro.
 * Registres modifiés : r0-r2 (routine feuille).                             */
    .global encoder_init
    .type   encoder_init, %function
encoder_init:
    ldr     r0, =RCC_BASE
    ldr     r1, [r0, #RCC_AHB2ENR]
    orr     r1, r1, #RCC_AHB2ENR_GPIOBEN
    str     r1, [r0, #RCC_AHB2ENR]
    ldr     r1, [r0, #RCC_AHB2ENR]
    /* 1. PB0, PB1 en entrée */
    ldr     r0, =GPIOB_BASE
    ldr     r1, [r0, #GPIO_MODER]
    bic     r1, r1, #(3 << (2 * ENC_A_PIN))
    bic     r1, r1, #(3 << (2 * ENC_B_PIN))
    str     r1, [r0, #GPIO_MODER]
    ldr     r1, [r0, #GPIO_PUPDR]
    bic     r1, r1, #(3 << (2 * ENC_A_PIN))
    bic     r1, r1, #(3 << (2 * ENC_B_PIN))
    orr     r1, r1, #(ENC_PUPDR << (2 * ENC_A_PIN))
    orr     r1, r1, #(ENC_PUPDR << (2 * ENC_B_PIN))
    str     r1, [r0, #GPIO_PUPDR]
    /* 2. ligne 0 <- port B */
    ldr     r0, =EXTI_BASE
    ldr     r1, [r0, #EXTI_EXTICR1]
    bic     r1, r1, #(0xFF << EXTICR1_EXTI0_POS)
    orr     r1, r1, #(EXTICR_PORT_B << EXTICR1_EXTI0_POS)
    str     r1, [r0, #EXTI_EXTICR1]
    /* 3. les deux fronts */
    mov     r2, #EXTI_LIGNE0
    ldr     r1, [r0, #EXTI_RTSR1]
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_RTSR1]
    ldr     r1, [r0, #EXTI_FTSR1]
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_FTSR1]
    str     r2, [r0, #EXTI_RPR1]        /* requêtes en attente effacées (w1c) */
    str     r2, [r0, #EXTI_FPR1]
    /* 4. démasquer */
    ldr     r1, [r0, #EXTI_IMR1]
    orr     r1, r1, r2
    str     r1, [r0, #EXTI_IMR1]
    /* 6. variables */
    movs    r1, #0
    ldr     r0, =enc_count
    str     r1, [r0]
    ldr     r0, =enc_repere
    str     r1, [r0]
    ldr     r0, =vitesse_fronts
    str     r1, [r0]
    ldr     r0, =sens_mesure
    str     r1, [r0]
    /* 5. NVIC : priorité 1 (octet), puis activation */
    ldr     r0, =NVIC_IPR_BASE
    movs    r1, #(PRIO_ENCODEUR << PRIO_POS)
    strb    r1, [r0, #EXTI0_IRQn]
    ldr     r0, =NVIC_ISER0
    mov     r1, #(1 << EXTI0_IRQn)
    str     r1, [r0]
    bx      lr
    .size   encoder_init, .-encoder_init

/* void EXTI0_IRQHandler(void)   — vecteur d'interruption EXTI ligne 0
 * Remplace le gestionnaire faible du fichier de démarrage. Le matériel
 * empile r0-r3, r12, lr, pc, xPSR : r0-r3 utilisables sans sauvegarde.
 *   1. une seule lecture de GPIOB_IDR ; A = bit 0, B = bit 1 ;
 *   2. A = B -> +1, A != B -> -1 (puis ENC_SENS_INVERSE) sur enc_count ;
 *   3. effacement des requêtes RPR1 et FPR1 bit 0, dsb, retour.
 * Durée : une vingtaine d'instructions ; peut être préemptée par E-Stop
 * (priorité 0) sans risque : enc_count n'est pas touché par E-Stop.        */
    .global EXTI0_IRQHandler
    .type   EXTI0_IRQHandler, %function
EXTI0_IRQHandler:
    ldr     r0, =GPIOB_BASE
    ldr     r1, [r0, #GPIO_IDR]
    lsr     r2, r1, #ENC_A_PIN
    lsr     r1, r1, #ENC_B_PIN
    eor     r1, r1, r2                  /* bit 0 : 0 si A = B, 1 si A != B   */
    and     r1, r1, #1
    .if ENC_SENS_INVERSE
    eor     r1, r1, #1
    .endif
    ldr     r0, =enc_count
    ldr     r2, [r0]
    cmp     r1, #0
    ite     eq
    addeq   r2, r2, #1                  /* A = B  : +1                       */
    subne   r2, r2, #1                  /* A != B : -1                       */
    str     r2, [r0]
    ldr     r0, =EXTI_BASE
    mov     r1, #EXTI_LIGNE0
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    bx      lr
    .size   EXTI0_IRQHandler, .-EXTI0_IRQHandler

/* uint32_t encoder_read(uint32_t *sens)
 * r0 = adresse d'un mot qui reçoit le sens (SENS_x), ou 0 si non désiré.
 * Retour r0 = nombre de fronts (valeur absolue) depuis la lecture précédente.
 * Met aussi à jour vitesse_fronts et sens_mesure (visibles au débogueur).
 * enc_count est lu par une seule instruction LDR : pas de section critique.
 * Registres modifiés : r0-r3 (routine feuille).                             */
    .global encoder_read
    .type   encoder_read, %function
encoder_read:
    ldr     r1, =enc_count
    ldr     r1, [r1]                    /* lecture atomique                  */
    ldr     r2, =enc_repere
    ldr     r3, [r2]
    str     r1, [r2]                    /* nouveau repère                    */
    subs    r3, r1, r3                  /* variation signée                  */
    beq     encoder_read_immobile
    bmi     encoder_read_arriere
    movs    r2, #SENS_AVANT
    b       encoder_read_fin
encoder_read_arriere:
    rsbs    r3, r3, #0                  /* valeur absolue                    */
    movs    r2, #SENS_ARRIERE
    b       encoder_read_fin
encoder_read_immobile:
    movs    r2, #SENS_IMMOBILE
encoder_read_fin:
    cmp     r0, #0
    it      ne
    strne   r2, [r0]                    /* *sens = r2 si adresse fournie     */
    ldr     r0, =sens_mesure
    str     r2, [r0]
    ldr     r0, =vitesse_fronts
    str     r3, [r0]
    mov     r0, r3
    bx      lr
    .size   encoder_read, .-encoder_read
