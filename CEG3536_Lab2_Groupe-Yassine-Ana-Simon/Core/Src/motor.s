/* ---------------------------------------------------------------------------
 * motor.s — commande du moteur : PWM TIM1_CH1 et direction AI1/AI2/STB
 *           (CEG 3536, laboratoire 2) — SQUELETTE À COMPLÉTER
 *
 * Routines exportées (AAPCS) : motor_init, motor_set_pwm, motor_set_dir,
 *                              motor_stop_now
 * Aucune variable : l'état de la propulsion se lit dans les registres
 * (TIM1_CCR1, GPIOB_ODR, GPIOE_ODR).
 *
 * Table du pilote (guide, table 12) : AI1 = 1, AI2 = 0 + PWM -> un sens ;
 * AI1 = 0, AI2 = 1 + PWM -> l'autre sens ; AI1 = AI2 = 0 -> arrêt (haute
 * impédance) ; STB = 0 -> veille. AI1 = AI2 = 1 (freinage court) jamais produit.
 * Règle de sécurité (E3) : AI1/AI2/STB ne changent que lorsque CCR1 = 0.
 *
 * Fourni : horloges, GPIO (PB10, PB11, PE15 en sortie, PE9 en AF1),
 *          motor_set_pwm, motor_stop_now.
 * À écrire : configuration de TIM1 (étape 4 de motor_init, E2) et la règle
 *            de sens de motor_set_dir (E3).
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .text
    .align  2

/* void motor_init(void)
 *   1. horloges GPIOB, GPIOE (RCC_AHB2ENR) et TIM1 (RCC_APB2ENR) ;
 *   2. PB10, PB11, PE15 en sortie push-pull, niveau 0 (BSRR) ;
 *   3. PE9 en fonction alternative AF1 (MODER = 10, AFRH champ 1 = 0001) ;
 *   4. TIM1 : PSC, ARR, CCR1 = 0, CCMR1 (CC1S = 00, OC1M = PWM 1, OC1PE),
 *      CCER (CC1E, polarité haute), CR1 (ARPE), BDTR (MOE), EGR (UG) puis
 *      CR1 (CEN). UG charge PSC et ARR dans les registres actifs.
 * État final : PWM à 0 % (PE9 bas), STB = 0, AI1 = AI2 = 0 (E1).
 * Registres modifiés : r0-r2 (routine feuille).                             */
    .global motor_init
    .type   motor_init, %function
motor_init:
    /* 1. horloges */
    ldr     r0, =RCC_BASE
    ldr     r1, [r0, #RCC_AHB2ENR]
    orr     r1, r1, #(RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOEEN)
    str     r1, [r0, #RCC_AHB2ENR]
    ldr     r1, [r0, #RCC_APB2ENR]
    orr     r1, r1, #RCC_APB2ENR_TIM1EN
    str     r1, [r0, #RCC_APB2ENR]
    ldr     r1, [r0, #RCC_APB2ENR]      /* relecture : délai après activation */

    /* 2a. GPIOB : PB10, PB11 en sortie, niveau 0 d'abord */
    ldr     r0, =GPIOB_BASE
    mov     r1, #((1 << (MOTOR_AI1_PIN + 16)) | (1 << (MOTOR_AI2_PIN + 16)))
    str     r1, [r0, #GPIO_BSRR]
    ldr     r1, [r0, #GPIO_MODER]
    bic     r1, r1, #(3 << (2 * MOTOR_AI1_PIN))
    bic     r1, r1, #(3 << (2 * MOTOR_AI2_PIN))
    orr     r1, r1, #(GPIO_MODE_SORTIE << (2 * MOTOR_AI1_PIN))
    orr     r1, r1, #(GPIO_MODE_SORTIE << (2 * MOTOR_AI2_PIN))
    str     r1, [r0, #GPIO_MODER]
    ldr     r1, [r0, #GPIO_OTYPER]
    bic     r1, r1, #((1 << MOTOR_AI1_PIN) | (1 << MOTOR_AI2_PIN))  /* push-pull */
    str     r1, [r0, #GPIO_OTYPER]
    ldr     r1, [r0, #GPIO_PUPDR]
    bic     r1, r1, #(3 << (2 * MOTOR_AI1_PIN))
    bic     r1, r1, #(3 << (2 * MOTOR_AI2_PIN))
    str     r1, [r0, #GPIO_PUPDR]

    /* 2b. GPIOE : PE15 en sortie (STB = 0), PE9 en AF1 */
    ldr     r0, =GPIOE_BASE
    mov     r1, #(1 << (MOTOR_STB_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]        /* STB = 0 avant le passage en sortie */
    ldr     r1, [r0, #GPIO_AFRH]
    bic     r1, r1, #(0xF << (4 * (MOTOR_PWM_PIN - 8)))
    orr     r1, r1, #(GPIO_AF1 << (4 * (MOTOR_PWM_PIN - 8)))
    str     r1, [r0, #GPIO_AFRH]        /* AF choisie avant MODER = AF (pas de glitch) */
    ldr     r1, [r0, #GPIO_MODER]
    bic     r1, r1, #(3 << (2 * MOTOR_STB_PIN))
    orr     r1, r1, #(GPIO_MODE_SORTIE << (2 * MOTOR_STB_PIN))
    bic     r1, r1, #(3 << (2 * MOTOR_PWM_PIN))
    orr     r1, r1, #(GPIO_MODE_AF << (2 * MOTOR_PWM_PIN))
    str     r1, [r0, #GPIO_MODER]
    ldr     r1, [r0, #GPIO_OTYPER]
    bic     r1, r1, #((1 << MOTOR_STB_PIN) | (1 << MOTOR_PWM_PIN))
    str     r1, [r0, #GPIO_OTYPER]
    ldr     r1, [r0, #GPIO_OSPEEDR]
    bic     r1, r1, #(3 << (2 * MOTOR_PWM_PIN))
    orr     r1, r1, #(1 << (2 * MOTOR_PWM_PIN))   /* vitesse moyenne pour 20 kHz */
    bic     r1, r1, #(3 << (2 * MOTOR_STB_PIN))
    str     r1, [r0, #GPIO_OSPEEDR]
    ldr     r1, [r0, #GPIO_PUPDR]
    bic     r1, r1, #(3 << (2 * MOTOR_PWM_PIN))
    bic     r1, r1, #(3 << (2 * MOTOR_STB_PIN))
    str     r1, [r0, #GPIO_PUPDR]

    /* 4. TIM1 en PWM mode 1 sur la voie 1 — À COMPLÉTER (E2)
     *   ldr r0, =TIM1_BASE
     *   a) PSC = PWM_PSC ; ARR = PWM_ARR (ldr r1, =PWM_ARR) ; CCR1 = 0
     *   b) CCMR1 : effacer CC1S (TIM_CCMR1_CC1S_MASK) et OC1M
     *      (ldr r2, =TIM_CCMR1_OC1M_MASK ; bic), puis orr
     *      (TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE)
     *   c) CCER : bic TIM_CCER_CC1P (actif haut), orr TIM_CCER_CC1E
     *   d) CR1 |= TIM_CR1_ARPE
     *   e) BDTR |= TIM_BDTR_MOE          (indispensable sur un temporisateur avancé)
     *   f) EGR = TIM_EGR_UG              (charge PSC, ARR, CCR1), puis SR = 0
     *   g) CR1 |= TIM_CR1_CEN            (le compteur tourne à 20 kHz)
     * Vérifier au débogueur (SFRs > TIM1) : CR1 = 0x81, CCMR1 = 0x68,
     * CCER = 0x1, ARR = 0xC7, BDTR = 0x8000 ; puis à l'oscilloscope (T3).   */

    bx      lr
    .size   motor_init, .-motor_init

/* uint32_t motor_set_pwm(uint32_t pct)
 * r0 = rapport cyclique demandé, 0 à 100 (%) ; valeurs > 100 saturées à 100.
 * CCR1 = pct * PWM_PLEINE / 100 (PWM_PLEINE = ARR + 1 = 200 : 100 % -> 200,
 * sortie haute en permanence ; 0 -> sortie basse). Le prérechargement
 * (OC1PE) applique la nouvelle valeur au prochain débordement : pas
 * d'impulsion tronquée. Retour r0 = valeur écrite dans CCR1.
 * Registres modifiés : r0-r2 (routine feuille). FOURNIE.                    */
    .global motor_set_pwm
    .type   motor_set_pwm, %function
motor_set_pwm:
    cmp     r0, #100
    it      hi
    movhi   r0, #100
    movs    r1, #PWM_PLEINE
    mul     r0, r0, r1                  /* r0 = pct * (ARR + 1)              */
    movs    r1, #100
    udiv    r0, r0, r1                  /* r0 = CCR1                         */
    ldr     r2, =TIM1_BASE
    str     r0, [r2, #TIM_CCR1]
    bx      lr
    .size   motor_set_pwm, .-motor_set_pwm

/* uint32_t motor_set_dir(uint32_t dir)
 * r0 = MOTOR_ARRET (0), MOTOR_AVANT (1), MOTOR_ARRIERE (2).
 * Retour r0 = 0 si accepté, 1 si refusé (sorties inchangées).
 *   - MOTOR_ARRET : toujours accepté ; CCR1 = 0, AI1 = AI2 = 0, STB = 0
 *     (même séquence que motor_stop_now, sans appel : routine feuille).
 *   - MOTOR_AVANT / MOTOR_ARRIERE : refusé si CCR1 != 0 (règle E3 : jamais
 *     de changement de sens ni de départ sous PWM non nulle) ; sinon AI1/AI2
 *     par UNE SEULE écriture BSRR (mise à 1 d'une broche, mise à 0 de l'autre :
 *     jamais AI1 = AI2 = 1), puis STB = 1. Les masques combinés ne tiennent
 *     pas dans un immédiat : utiliser ldr r2, =(...).
 *   - autre valeur : refusé.
 * Registres modifiés : r0-r3 (routine feuille).
 * À COMPLÉTER (E3). Le squelette refuse tout : le moteur ne peut pas tourner
 * tant que la règle n'est pas écrite.                                       */
    .global motor_set_dir
    .type   motor_set_dir, %function
motor_set_dir:
    /* À COMPLÉTER : voir les étapes ci-dessus. Étiquettes suggérées :
     * motor_set_dir_arret, motor_set_dir_arriere, motor_set_dir_ecrire,
     * motor_set_dir_refus.                                                   */
    movs    r0, #1                      /* refusé tant que non écrit         */
    bx      lr
    .size   motor_set_dir, .-motor_set_dir

/* void motor_stop_now(void)
 * Mise en sécurité minimale de la propulsion : CCR1 = 0, STB = 0, AI1 = AI2 = 0,
 * trois écritures atomiques. N'utilise que r0-r1 : appelable depuis
 * EXTI2_IRQHandler (E7) sans sauvegarde supplémentaire.
 * Registres modifiés : r0, r1. FOURNIE.                                     */
    .global motor_stop_now
    .type   motor_stop_now, %function
motor_stop_now:
    ldr     r0, =TIM1_BASE
    movs    r1, #0
    str     r1, [r0, #TIM_CCR1]         /* PWM nulle (prise au prochain débordement, < 50 µs) */
    ldr     r0, =GPIOE_BASE
    mov     r1, #(1 << (MOTOR_STB_PIN + 16))
    str     r1, [r0, #GPIO_BSRR]        /* STB = 0 : sorties en haute impédance immédiatement */
    ldr     r0, =GPIOB_BASE
    mov     r1, #((1 << (MOTOR_AI1_PIN + 16)) | (1 << (MOTOR_AI2_PIN + 16)))
    str     r1, [r0, #GPIO_BSRR]        /* AI1 = AI2 = 0                     */
    bx      lr
    .size   motor_stop_now, .-motor_stop_now
