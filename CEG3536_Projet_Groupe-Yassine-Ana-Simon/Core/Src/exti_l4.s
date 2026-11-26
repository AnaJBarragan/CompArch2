/* ---------------------------------------------------------------------------
 * exti_l4.s — interruptions du PIR et du TouchPad
 *             (CEG 3536, laboratoire 4, E6 et E4) — complété par l'équipe
 *
 * Routines exportées : exti_l4_init, EXTI9_IRQHandler
 * Variables (.bss)    : pir_flag, pir_t_ms, thi_flag
 *
 * Règle du laboratoire : un gestionnaire lève un drapeau et mémorise un
 * horodatage. Aucun accès au bus I2C ni au moteur ici ; seul le gestionnaire
 * E-Stop (estop.s) agit sur la propulsion. Partie II (LISEZMOI, E14) : le
 * gestionnaire du PIR pousse en plus EV_PIR dans la file (bl evq_push) ;
 * celui de THI ne fait que lever thi_flag (la lecture du TouchPad, sur le
 * bus, reste dans la boucle principale).
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
pir_deja:       .space  4           /* 1 dès qu'un front a été retenu (filtrage)   */

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
    /* 1. PB6 et PB9 en entrée (MODER = 00 ; après réinitialisation, les
     *    broches du STM32L5 sont en mode analogique, 11), sans tirage      */
    ldr     r0, =GPIOB_BASE
    ldr     r1, [r0, #GPIO_MODER]
    bic     r1, r1, #((3 << (2 * THI_PIN)) | (3 << (2 * PIR_PIN)))
    str     r1, [r0, #GPIO_MODER]
    ldr     r1, [r0, #GPIO_PUPDR]
    bic     r1, r1, #((3 << (2 * THI_PIN)) | (3 << (2 * PIR_PIN)))
    str     r1, [r0, #GPIO_PUPDR]

    /* 2. sources : EXTI6 <- port B (EXTICR2, bits 23:16) ;
     *              EXTI9 <- port B (EXTICR3, bits 15:8)                    */
    ldr     r0, =EXTI_BASE
    ldr     r1, [r0, #EXTI_EXTICR2]
    bic     r1, r1, #(0xFF << EXTICR2_EXTI6_POS)
    orr     r1, r1, #(EXTICR_PORT_B << EXTICR2_EXTI6_POS)
    str     r1, [r0, #EXTI_EXTICR2]
    ldr     r1, [r0, #EXTI_EXTICR3]
    bic     r1, r1, #(0xFF << EXTICR3_EXTI9_POS)
    orr     r1, r1, #(EXTICR_PORT_B << EXTICR3_EXTI9_POS)
    str     r1, [r0, #EXTI_EXTICR3]

    /* 3. fronts. PIR : montant seulement. THI : selon THI_FRONT_MONTANT.   */
    ldr     r1, [r0, #EXTI_RTSR1]
    ldr     r2, [r0, #EXTI_FTSR1]
    orr     r1, r1, #EXTI_LIGNE9
    bic     r2, r2, #EXTI_LIGNE9
    .if THI_FRONT_MONTANT
    orr     r1, r1, #EXTI_LIGNE6
    bic     r2, r2, #EXTI_LIGNE6
    .else
    bic     r1, r1, #EXTI_LIGNE6
    orr     r2, r2, #EXTI_LIGNE6
    .endif
    str     r1, [r0, #EXTI_RTSR1]
    str     r2, [r0, #EXTI_FTSR1]

    /* 4. requêtes en attente effacées (écriture d'un 1) */
    mov     r1, #(EXTI_LIGNE6 | EXTI_LIGNE9)
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]

    /* 5. démasquage des deux lignes */
    ldr     r2, [r0, #EXTI_IMR1]
    orr     r2, r2, r1
    str     r2, [r0, #EXTI_IMR1]

    /* 6. NVIC : priorités (3 bits utiles, bits 7:5), puis activation */
    ldr     r0, =NVIC_IPR_BASE
    movs    r1, #(PRIO_PIR << PRIO_POS)
    strb    r1, [r0, #EXTI9_IRQn]
    movs    r1, #(PRIO_THI << PRIO_POS)
    strb    r1, [r0, #EXTI6_IRQn]
    ldr     r0, =NVIC_ISER0
    mov     r1, #((1 << EXTI9_IRQn) | (1 << EXTI6_IRQn))
    str     r1, [r0]                    /* ISER : écrire 1 active, 0 sans effet */
    bx      lr
    .size   exti_l4_init, .-exti_l4_init

/* void EXTI9_IRQHandler(void)   — vecteur d'interruption EXTI ligne 9 (PIR)
 * Remplace le gestionnaire faible (.weak) du fichier de démarrage.
 *   1. effacer la requête (RPR1, écriture d'un 1) puis dsb — fait EN PREMIER
 *      à la partie II : un nouveau front pendant l'appel de evq_push n'est
 *      ainsi pas effacé par erreur ;
 *   2. filtrage : si un front a déjà été retenu et que ticks_ms - pir_t_ms <
 *      PIR_FILTRE_MS, ignorer ce front (soustraction non signée, correcte au
 *      débordement de ticks_ms) ;
 *   3. sinon pir_t_ms = ticks_ms, pir_flag = 1 (gardé pour le débogueur et
 *      pour le suivi du retour à 0 dans main.c) ;
 *   4. partie II (E14) : evq_push(EV_PIR, pir_t_ms, ticks_ms) — l'événement
 *      entre dans la file à l'instant de l'interruption. evq_push protège la
 *      file par crit_entrer / crit_sortir : l'appel depuis un gestionnaire est
 *      sûr. La valeur 0 étant réservée au retour à 0 du signal, un
 *      horodatage nul est remplacé par 1.
 * Routine NON feuille depuis la partie II : push {r4, lr} (r4 garde
 * l'horodatage à travers l'appel ; 8 octets, pile alignée), retour par
 * pop {r4, pc} (EXC_RETURN). Aucun accès au bus, aucune action sur le moteur. */
    .global EXTI9_IRQHandler
    .type   EXTI9_IRQHandler, %function
EXTI9_IRQHandler:
    push    {r4, lr}
    ldr     r0, =EXTI_BASE              /* 1. requête effacée [FOURNI, déplacé] */
    mov     r1, #EXTI_LIGNE9
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    ldr     r0, =ticks_ms
    ldr     r4, [r0]                    /* r4 = maintenant                   */
    ldr     r1, =pir_deja
    ldr     r2, [r1]
    cbz     r2, EXTI9_retenir           /* premier front : toujours retenu   */
    ldr     r2, =pir_t_ms
    ldr     r2, [r2]
    subs    r2, r4, r2                  /* 2. écoulé = maintenant - pir_t_ms */
    cmp     r2, #PIR_FILTRE_MS          /* comparaison NON signée :          */
    blo     EXTI9_fin                   /* < 50 ms -> front ignoré           */
EXTI9_retenir:
    movs    r2, #1
    str     r2, [r1]                    /* 3. pir_deja = 1                   */
    ldr     r1, =pir_t_ms
    str     r4, [r1]                    /* pir_t_ms = maintenant             */
    ldr     r1, =pir_flag
    str     r2, [r1]                    /* pir_flag = 1                      */
    movs    r0, #EV_NUM_PIR             /* 4. evq_push(EV_PIR, valeur, t)    */
    mov     r1, r4                      /* valeur = pir_t_ms ...             */
    cbnz    r1, EXTI9_pousser
    movs    r1, #1                      /* ... jamais 0 (0 = signal retombé) */
EXTI9_pousser:
    mov     r2, r4                      /* t_ms = instant de l'interruption  */
    bl      evq_push
EXTI9_fin:
    pop     {r4, pc}
    .size   EXTI9_IRQHandler, .-EXTI9_IRQHandler

/* void EXTI6_IRQHandler(void)   — vecteur d'interruption EXTI ligne 6 (THI)
 * Le TouchPad signale un toucher ou un relâchement : on lève thi_flag, rien
 * d'autre. La boucle principale lit l'état des touches sur le bus (jamais
 * ici : aucun accès au bus dans une interruption), ce qui retire la demande
 * côté composant. Routine feuille : r0-r1 seulement.                       */
    .global EXTI6_IRQHandler
    .type   EXTI6_IRQHandler, %function
EXTI6_IRQHandler:
    ldr     r0, =thi_flag
    movs    r1, #1
    str     r1, [r0]                    /* thi_flag = 1                      */
    ldr     r0, =EXTI_BASE              /* requête effacée (les deux fronts) */
    mov     r1, #EXTI_LIGNE6
    str     r1, [r0, #EXTI_RPR1]
    str     r1, [r0, #EXTI_FPR1]
    dsb
    bx      lr
    .size   EXTI6_IRQHandler, .-EXTI6_IRQHandler
