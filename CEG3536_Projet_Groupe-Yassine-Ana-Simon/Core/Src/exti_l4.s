/* ---------------------------------------------------------------------------
 * exti_l4.s — interruptions du PIR et du TouchPad
 *             (CEG 3536, laboratoire 4, E6 et E4) — SQUELETTE À COMPLÉTER
 *
 * Routines exportées : exti_l4_init, EXTI9_IRQHandler
 * Variables (.bss)    : pir_flag, pir_t_ms, thi_flag
 *
 * Règle du laboratoire : un gestionnaire lève un drapeau et mémorise un
 * horodatage, rien d'autre. Aucun accès au bus I2C ni au moteur ici ; seul
 * le gestionnaire E-Stop (estop.s) agit sur la propulsion.
 *
 * Au moins un des deux gestionnaires est écrit en assembleur (E10) :
 *   - EXTI9_IRQHandler (PIR, PB9) : squelette ci-dessous ;
 *   - EXTI6_IRQHandler (THI, PB6) : à écrire ici, sur le même modèle, ou en C
 *     dans touchpad.c (void EXTI6_IRQHandler(void)). Ne pas le définir deux
 *     fois. Tant qu'il n'existe pas, laisser la ligne 6 masquée (IMR1).
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"
#include "registres_lab4.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global pir_flag
pir_flag:       .space  4           /* 1 = mouvement détecté, à consommer par main */
    .global pir_t_ms
pir_t_ms:       .space  4           /* ticks_ms du dernier front retenu            */
    .global thi_flag
thi_flag:       .space  4           /* 1 = demande du TouchPad, à consommer        */

    .text
    .align  2

/* void exti_l4_init(void)
 * Configure PB9 (PIR) comme source de EXTI9 et PB6 (THI) comme source de
 * EXTI6. À appeler après gpio_init (horloge de GPIOB déjà active) et avant
 * la boucle principale. Modèle : estop_init (estop.s).
 *   1. PB9 et PB6 en entrée : champs MODER9 et MODER6 = 00 (GPIOB_BASE).
 *   2. EXTI_EXTICR3 : champ EXTI9 = EXTICR_PORT_B ;
 *      EXTI_EXTICR2 : champ EXTI6 = EXTICR_PORT_B.
 *   3. PIR : front montant (RTSR1 |= EXTI_LIGNE9, FTSR1 &= ~EXTI_LIGNE9).
 *      THI : front selon THI_FRONT_MONTANT (registres_lab4.inc), à relever.
 *   4. Requêtes en attente effacées (RPR1 et FPR1, écriture d'un 1).
 *   5. EXTI_IMR1 |= EXTI_LIGNE9 (et EXTI_LIGNE6 quand son gestionnaire existe).
 *   6. NVIC : priorités PRIO_PIR et PRIO_THI (octets NVIC_IPR_BASE + IRQn),
 *      puis NVIC_ISER0, bits EXTI9_IRQn et EXTI6_IRQn.
 * Registres modifiés : r0-r3 (routine feuille).                             */
    .global exti_l4_init
    .type   exti_l4_init, %function
exti_l4_init:
    /* À COMPLÉTER */
    bx      lr
    .size   exti_l4_init, .-exti_l4_init

/* void EXTI9_IRQHandler(void)   — vecteur d'interruption EXTI ligne 9 (PIR)
 * Remplace le gestionnaire faible (.weak) du fichier de démarrage.
 *   1. filtrage : si ticks_ms - pir_t_ms < PIR_FILTRE_MS et qu'un front a déjà
 *      été retenu, ignorer ce front (même principe que l'anti-rebond du
 *      laboratoire 1 ; soustraction non signée, correcte au débordement) ;
 *   2. sinon pir_t_ms = ticks_ms et pir_flag = 1 ;
 *   3. dans tous les cas, effacer la requête (RPR1, écriture d'un 1) puis dsb.
 * Routine feuille : seuls r0-r3 sont utilisés (empilés par le matériel).
 * L'étape 3 est fournie : sans elle, le gestionnaire serait rappelé sans fin. */
    .global EXTI9_IRQHandler
    .type   EXTI9_IRQHandler, %function
EXTI9_IRQHandler:
    /* À COMPLÉTER : étapes 1 et 2 */

    ldr     r0, =EXTI_BASE              /* 3. requête effacée [FOURNI]       */
    mov     r1, #EXTI_LIGNE9
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    bx      lr
    .size   EXTI9_IRQHandler, .-EXTI9_IRQHandler
