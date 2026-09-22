/* ---------------------------------------------------------------------------
 * registres.inc — constantes du laboratoire 1 (CEG 3536, automne 2026)
 * Cible : STM32L552ZET6QU (Cortex-M33), carte NUCLEO-L552ZE-Q, TZEN = 0.
 * Adresses et noms d'après le manuel de référence RM0438 (et stm32l552xx.h).
 * Inclus dans chaque fichier .s par  #include "registres.inc"
 * (le préprocesseur C est actif : option -x assembler-with-cpp).
 * ------------------------------------------------------------------------- */

/* ---- RCC (RM0438, section 9) ------------------------------------------ */
    .equ RCC_BASE,              0x40021000
    .equ RCC_AHB2ENR,           0x4C        /* horloges des GPIO            */
    .equ RCC_APB2ENR,           0x60        /* SYSCFGEN (bit 0)             */
    .equ RCC_AHB2ENR_GPIOAEN,   (1 << 0)
    .equ RCC_AHB2ENR_GPIOBEN,   (1 << 1)
    .equ RCC_AHB2ENR_GPIOCEN,   (1 << 2)
    .equ RCC_APB2ENR_SYSCFGEN,  (1 << 0)

/* ---- GPIO (RM0438, section 12) ---------------------------------------- */
    .equ GPIOA_BASE,            0x42020000
    .equ GPIOB_BASE,            0x42020400
    .equ GPIOC_BASE,            0x42020800
    .equ GPIO_MODER,            0x00        /* 00 entrée, 01 sortie          */
    .equ GPIO_OTYPER,           0x04        /* 0 push-pull, 1 drain ouvert   */
    .equ GPIO_OSPEEDR,          0x08        /* 00 basse vitesse              */
    .equ GPIO_PUPDR,            0x0C        /* 00 aucune, 01 tirage haut, 10 tirage bas */
    .equ GPIO_IDR,              0x10
    .equ GPIO_ODR,              0x14
    .equ GPIO_BSRR,             0x18        /* bits 15:0 = mise à 1, bits 31:16 = mise à 0 */
    .equ GPIO_BRR,              0x28

/* ---- EXTI (RM0438, section 16) — sur la L5, EXTICR est dans EXTI ------ */
    .equ EXTI_BASE,             0x4002F400
    .equ EXTI_RTSR1,            0x00        /* front montant                 */
    .equ EXTI_FTSR1,            0x04        /* front descendant              */
    .equ EXTI_SWIER1,           0x08
    .equ EXTI_RPR1,             0x0C        /* requête front montant (w1c)   */
    .equ EXTI_FPR1,             0x10        /* requête front descendant (w1c)*/
    .equ EXTI_EXTICR1,          0x60        /* lignes 0..3, 8 bits par ligne */
    .equ EXTI_IMR1,             0x80        /* masque d'interruption         */
    .equ EXTI_EMR1,             0x84
    .equ EXTI_LIGNE2,           (1 << 2)
    .equ EXTICR1_EXTI2_POS,     16          /* champ EXTI2 : bits 18:16      */
    .equ EXTICR_PORT_B,         0x01        /* 0 = A, 1 = B, 2 = C, ...      */

/* ---- NVIC et SysTick (Cortex-M33, PM0264) ----------------------------- */
    .equ NVIC_ISER0,            0xE000E100
    .equ NVIC_ICER0,            0xE000E180
    .equ NVIC_ICPR0,            0xE000E280
    .equ NVIC_IPR_BASE,         0xE000E400  /* un octet par IRQ, 3 bits utiles (7:5) */
    .equ EXTI2_IRQn,            13
    .equ NVIC_PRIO_MAX,         0x00        /* 0 = priorité la plus élevée   */
    .equ SYSTICK_CTRL,          0xE000E010
    .equ SYSTICK_LOAD,          0xE000E014
    .equ SYSTICK_VAL,           0xE000E018

/* ---- Brochage de la plateforme (guide, p. 25-29) ---------------------- */
    .equ LED_ROUGE_PIN,         9           /* LD3, PA9                      */
    .equ LED_VERTE_PIN,         7           /* LD1, PC7                      */
    .equ LED_BLEUE_PIN,         7           /* LD2, PB7                      */
    .equ BTN_USER_PIN,          13          /* PC13                          */
    .equ BTN_ESTOP_PIN,         2           /* PB2 (EXTI2)                   */
    .equ BTN_TOUCH_PIN,         5           /* PB5                           */

/* Niveau logique actif : 1 si « appuyé = 1 ». À CONFIRMER PAR MESURE (T4).
 * User (Nucleo) : tirage bas externe, appuyé = 1.
 * E-Stop et Touch En : dépend du câblage Leafy — mesurer au point de test. */
    .equ BTN_USER_ACTIF_HAUT,   1
    .equ BTN_ESTOP_ACTIF_HAUT,  1
    .equ BTN_TOUCH_ACTIF_HAUT,  1

/* Résistances de rappel internes (PUPDR) : 0 aucune, 1 tirage haut, 2 tirage bas.
 * User : 0 (tirage externe sur la Nucleo). E-Stop / Touch En : selon la mesure. */
    .equ BTN_USER_PUPDR,        0
    .equ BTN_ESTOP_PUPDR,       0
    .equ BTN_TOUCH_PUPDR,       0

/* ---- Identifiants d'interface (section 4.2 de l'énoncé) --------------- */
    .equ LED_AUCUNE,            0
    .equ LED_ROUGE,             1
    .equ LED_VERTE,             2
    .equ LED_BLEUE,             3
    .equ BTN_USER,              0
    .equ BTN_ESTOP,             1
    .equ BTN_TOUCH,             2

/* ---- États de la machine (section 3) ---------------------------------- */
    .equ ETAT_ARRET,            0
    .equ ETAT_MARCHE_AVANT,     1
    .equ ETAT_MARCHE_ARRIERE,   2
    .equ ETAT_ARRET_URGENCE,    3

/* ---- Temporisations (E3, E5, E7, E8) ---------------------------------- */
    .equ PERIODE_SCRUTATION_MS, 1           /* période d'appel de fsm_step   */
    .equ ANTIREBOND_MS,         30          /* fenêtre exigée : 20 à 50 ms   */
    .equ CLIGNOTEMENT_DEMI_MS,  250         /* 2 Hz : 250 ms allumée, 250 ms éteinte */
    .equ TOUCH_SIGNAL_MS,       100         /* extinction brève (E7)         */
/* delay_ms : boucle calibrée. Horloge après réinitialisation : MSI 4 MHz.
 * Valeur de départ ~1000 itérations/ms ; À CALIBRER à l'oscilloscope (E8). */
    .equ DELAY_BOUCLES_PAR_MS,  1000
