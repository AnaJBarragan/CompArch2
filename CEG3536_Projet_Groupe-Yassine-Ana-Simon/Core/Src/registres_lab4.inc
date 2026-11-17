/* ---------------------------------------------------------------------------
 * registres_lab4.inc — constantes du laboratoire 4 (CEG 3536, automne 2026)
 * Complète registres.inc et registres_lab2.inc : lignes EXTI 6 (THI, PB6) et
 * 9 (PIR, PB9). Adresses et noms d'après le RM0438 (et stm32l552xx.h).
 * ------------------------------------------------------------------------- */

/* ---- EXTI : sélection de la source, 8 bits par ligne ------------------- */
    .equ EXTI_EXTICR2,          0x64        /* lignes 4..7                   */
    .equ EXTI_EXTICR3,          0x68        /* lignes 8..11                  */
    .equ EXTICR2_EXTI6_POS,     16          /* champ EXTI6 : bits 23:16      */
    .equ EXTICR3_EXTI9_POS,     8           /* champ EXTI9 : bits 15:8       */
    .equ EXTI_LIGNE6,           (1 << 6)
    .equ EXTI_LIGNE9,           (1 << 9)

/* ---- NVIC : un vecteur par ligne sur le STM32L552 ---------------------- */
    .equ EXTI6_IRQn,            17
    .equ EXTI9_IRQn,            20
/* Priorités (0 = la plus élevée ; PRIO_ESTOP = 0, PRIO_ENCODEUR = 1,
 * PRIO_SYSTICK = 2 dans registres_lab2.inc). Valeurs de départ, à justifier
 * dans le rapport : sous E-Stop et l'encodeur, au niveau de SysTick.        */
    .equ PRIO_PIR,              2
    .equ PRIO_THI,              3

/* ---- Brochage (guide, p. 44 et 51-52) ---------------------------------- */
    .equ THI_PIN,               6           /* PB6, interruption du TouchPad */
    .equ PIR_PIN,               9           /* PB9, capteur de mouvement     */

/* Front actif de THI : À RELEVER au débogueur (guide, p. 44 : la ligne
 * signale chaque toucher ou relâchement). 1 = front montant, 0 = descendant. */
    .equ THI_FRONT_MONTANT,     0

/* ---- Filtrage du PIR (E6) ---------------------------------------------- */
    .equ PIR_FILTRE_MS,         50          /* identique à lab4.h            */
