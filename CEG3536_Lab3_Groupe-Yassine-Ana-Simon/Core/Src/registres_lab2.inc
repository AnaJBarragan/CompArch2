/* ---------------------------------------------------------------------------
 * registres_lab2.inc — constantes du laboratoire 2 (CEG 3536, automne 2026)
 * Complète registres.inc (laboratoire 1) : TIM1 (PWM moteur), TIM2 (variante
 * PWM du buzzer), SysTick, EXTI0 (encodeur), brochage moteur / encodeur /
 * buzzer, temporisations et paliers de consigne.
 * Cible : STM32L552ZET6QU (Cortex-M33), carte NUCLEO-L552ZE-Q, TZEN = 0.
 * Adresses et noms d'après RM0438 (périphériques) et PM0264 (SysTick, SCB).
 * Inclus dans chaque fichier .s APRÈS registres.inc :
 *     #include "registres.inc"
 *     #include "registres_lab2.inc"
 * ------------------------------------------------------------------------- */

/* ---- Horloge (SystemInit vide : MSI 4 MHz, HCLK = PCLK1 = PCLK2) -------- */
    .equ HORLOGE_HZ,            4000000

/* ---- RCC (RM0438, section 9) : compléments ---------------------------- */
    .equ RCC_APB1ENR1,          0x58        /* TIM2EN (bit 0)                */
    .equ RCC_AHB2ENR_GPIOEEN,   (1 << 4)
    .equ RCC_APB2ENR_TIM1EN,    (1 << 11)
    .equ RCC_APB1ENR1_TIM2EN,   (1 << 0)

/* ---- GPIO (RM0438, section 12) : port E et fonctions alternatives ------ */
    .equ GPIOE_BASE,            0x42021000
    .equ GPIO_AFRL,             0x20        /* broches 0..7,  4 bits par broche */
    .equ GPIO_AFRH,             0x24        /* broches 8..15, 4 bits par broche */
    .equ GPIO_MODE_ENTREE,      0
    .equ GPIO_MODE_SORTIE,      1
    .equ GPIO_MODE_AF,          2
    .equ GPIO_AF1,              1           /* AF1 : TIM1_CH1 sur PE9, TIM2_CH1 sur PA0 */

/* ---- TIM1 (RM0438, section 29) et TIM2 (section 30) -------------------- */
    .equ TIM1_BASE,             0x40012C00  /* APB2 */
    .equ TIM2_BASE,             0x40000000  /* APB1 */
    .equ TIM_CR1,               0x00
    .equ TIM_CR2,               0x04
    .equ TIM_SMCR,              0x08
    .equ TIM_DIER,              0x0C
    .equ TIM_SR,                0x10
    .equ TIM_EGR,               0x14
    .equ TIM_CCMR1,             0x18
    .equ TIM_CCMR2,             0x1C
    .equ TIM_CCER,              0x20
    .equ TIM_CNT,               0x24
    .equ TIM_PSC,               0x28
    .equ TIM_ARR,               0x2C
    .equ TIM_RCR,               0x30        /* TIM1 seulement                */
    .equ TIM_CCR1,              0x34
    .equ TIM_CCR2,              0x38
    .equ TIM_BDTR,              0x44        /* TIM1 seulement (MOE)          */

    .equ TIM_CR1_CEN,           (1 << 0)    /* compteur activé               */
    .equ TIM_CR1_ARPE,          (1 << 7)    /* prérechargement de ARR        */
    .equ TIM_EGR_UG,            (1 << 0)    /* mise à jour : charge PSC/ARR  */
    .equ TIM_SR_UIF,            (1 << 0)
    .equ TIM_CCMR1_CC1S_MASK,   (3 << 0)    /* 00 : voie 1 en sortie         */
    .equ TIM_CCMR1_OC1PE,       (1 << 3)    /* prérechargement de CCR1       */
    .equ TIM_CCMR1_OC1M_MASK,   ((7 << 4) | (1 << 16))   /* OC1M[2:0] + OC1M[3] */
    .equ TIM_CCMR1_OC1M_PWM1,   (6 << 4)    /* 0110 : PWM mode 1             */
    .equ TIM_CCER_CC1E,         (1 << 0)    /* sortie OC1 activée            */
    .equ TIM_CCER_CC1P,         (1 << 1)    /* polarité (0 = actif haut)     */
    .equ TIM_BDTR_MOE,          (1 << 15)   /* Main Output Enable (TIM1)     */

/* ---- PWM moteur : 20 kHz à 4 MHz, 200 pas de rapport cyclique ---------- */
    .equ PWM_FREQ_HZ,           20000
    .equ PWM_PSC,               0
    .equ PWM_ARR,               (HORLOGE_HZ / (PWM_PSC + 1) / PWM_FREQ_HZ - 1)  /* 199 */
    .equ PWM_PLEINE,            (PWM_ARR + 1)                                   /* CCR1 à 100 % */

/* ---- SysTick (PM0264) : compléments ----------------------------------- */
    .equ SYSTICK_CTRL_ENABLE,   (1 << 0)
    .equ SYSTICK_CTRL_TICKINT,  (1 << 1)
    .equ SYSTICK_CTRL_CLKSOURCE,(1 << 2)    /* 1 = horloge du processeur     */
    .equ SYSTICK_PERIODE_US,    1000        /* paramètre de systick_init     */
    .equ SYSTICK_LOAD_1MS,      (HORLOGE_HZ / 1000 - 1)                         /* 3999 */
    .equ SCB_SHPR3,             0xE000ED20  /* priorités PendSV (23:16), SysTick (31:24) */
    .equ SCB_SHPR3_SYSTICK,     0xE000ED23  /* octet de priorité de SysTick  */

/* ---- Priorités (3 bits utiles, bits 7:5 de chaque octet) --------------- */
    .equ PRIO_ESTOP,            0           /* EXTI2 : la plus élevée (lab. 1) */
    .equ PRIO_ENCODEUR,         1           /* EXTI0                         */
    .equ PRIO_SYSTICK,          2
    .equ PRIO_POS,              5           /* valeur << PRIO_POS dans l'octet */

/* ---- EXTI (RM0438, section 16) : ligne 0 (encodeur A, PB0) ------------- */
    .equ EXTI_LIGNE0,           (1 << 0)
    .equ EXTICR1_EXTI0_POS,     0           /* champ EXTI0 : bits 7:0        */
    .equ EXTI0_IRQn,            11

/* ---- Brochage de la plateforme (guide, p. 36 et 38-41) ---------------- */
    .equ MOTOR_PWM_PIN,         9           /* PE9,  TIM1_CH1 (AF1)          */
    .equ MOTOR_AI1_PIN,         10          /* PB10, direction IN1           */
    .equ MOTOR_AI2_PIN,         11          /* PB11, direction IN2           */
    .equ MOTOR_STB_PIN,         15          /* PE15, veille (0 = veille)     */
    .equ ENC_A_PIN,             0           /* PB0, voie A (EXTI0)           */
    .equ ENC_B_PIN,             1           /* PB1, voie B (lecture IDR)     */
    .equ BUZZER_PIN,            0           /* PA0, GPIO ou TIM2_CH1 (AF1)   */

/* Résistances de rappel des entrées encodeur : 0 aucune, 1 haut, 2 bas.
 * Les sorties de l'encodeur sont actives : aucun tirage a priori. À confirmer
 * par mesure au point de test (formes d'onde propres, niveaux 0 / 3,3 V).   */
    .equ ENC_PUPDR,             0

/* Sens de comptage : 0 si « A = B au front » correspond à la marche avant
 * (AI1 = 1, AI2 = 0) ; 1 pour inverser. À déterminer à la séance 2 (T5) en
 * comparant sens_mesure au sens commandé.                                   */
    .equ ENC_SENS_INVERSE,      0

/* ---- Identifiants d'interface (section 4.2 de l'énoncé) --------------- */
    .equ MOTOR_ARRET,           0           /* motor_set_dir                 */
    .equ MOTOR_AVANT,           1
    .equ MOTOR_ARRIERE,         2
    .equ SENS_IMMOBILE,         0           /* sens_mesure                   */
    .equ SENS_AVANT,            1
    .equ SENS_ARRIERE,          2

/* ---- Temporisations et paliers (E4, E6, E9) ---------------------------- */
    .equ RAMPE_PAS_MS,          10          /* 1 point de rapport cyclique par 10 ms */
    .equ FENETRE_VITESSE_MS,    500         /* fenêtre de mesure de l'encodeur */
    .equ BIP_MS,                100         /* durée d'un bip                */
    .equ BIP_SILENCE_MS,        100         /* silence entre deux bips       */
    .equ CONSIGNE_INITIALE,     50          /* % après réinitialisation      */
    .equ CONSIGNE_PAS,          25
    .equ CONSIGNE_MIN,          25
    .equ CONSIGNE_MAX,          100
