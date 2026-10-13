/* ---------------------------------------------------------------------------
 * fsm.s — machine à états du panneau de commande (CEG 3536, laboratoire 1)
 *         — CORRIGÉ
 *
 * Routines exportées : fsm_init, fsm_step
 * Routine locale     : fsm_maj_del (seul point d'appel de led_set, critère B3)
 * Variables (.bss)   : etat, touch_enabled, compteur_transitions,
 *                      prochain_sens, clignote_compteur, clignote_phase,
 *                      touch_signal_compteur, blink_cycles (T6)
 *
 * États (section 3) : ETAT_ARRET (rouge fixe), ETAT_MARCHE_AVANT (verte),
 *                     ETAT_MARCHE_ARRIERE (bleue), ETAT_ARRET_URGENCE (rouge 2 Hz)
 * Transitions :
 *   E2  ARRÊT --User--> MARCHE_AVANT --User--> ARRÊT --User--> MARCHE_ARRIÈRE
 *       --User--> ARRÊT ... (prochain_sens alterne avant/arrière)
 *   E4  tout état --estop_flag--> ARRÊT_URGENCE
 *   E5  ARRÊT_URGENCE : rouge clignote à 2 Hz, User ignoré
 *   E6  ARRÊT_URGENCE --Touch En validé ET E-Stop relâché--> ARRÊT
 *   E7  hors urgence : Touch En validé -> touch_enabled ^= 1, extinction 100 ms
 * ------------------------------------------------------------------------- */
#include "registres.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .bss
    .align  2
    .global etat
etat:                   .space  4   /* état courant (ETAT_x)                     */
    .global touch_enabled
touch_enabled:          .space  4   /* autorisation TouchPad, 0/1 (E7)           */
    .global compteur_transitions
compteur_transitions:   .space  4   /* nombre de changements d'état (T3, watch)  */
prochain_sens:          .space  4   /* 0 : prochain départ en avant, 1 : en arrière */
clignote_compteur:      .space  4   /* pas de scrutation écoulés dans la demi-période */
clignote_phase:         .space  4   /* 0 rouge éteinte, 1 rouge allumée (E5)     */
touch_signal_compteur:  .space  4   /* pas restants d'extinction brève (E7)      */
    .global blink_cycles
blink_cycles:           .space  4   /* T6 : cycles DWT entre deux basculements de la
                                       rouge en urgence (= demi-période ; 2 Hz ->
                                       environ 1 000 000 cycles à 4 MHz)          */
blink_cyccnt_prec:      .space  4   /* DWT_CYCCNT au basculement précédent       */

/* ---- Table état -> DEL (un octet par état) ------------------------------ */
    .section .rodata
etat_vers_del:
    .byte   LED_ROUGE       /* ETAT_ARRET          */
    .byte   LED_VERTE       /* ETAT_MARCHE_AVANT   */
    .byte   LED_BLEUE       /* ETAT_MARCHE_ARRIERE */
    .byte   LED_ROUGE       /* ETAT_ARRET_URGENCE (clignotante, voir fsm_maj_del) */

    .text
    .align  2

/* void fsm_init(void)
 * État initial ARRÊT, variables à zéro, DEL rouge seule (E1).
 * Active aussi le compteur de cycles DWT (DEMCR.TRCENA, DWT_CTRL.CYCCNTENA)
 * qui sert à la mesure du clignotement (T6, variable blink_cycles).
 * Appelle fsm_maj_del : LR sauvegardé ; push {r4, lr} garde l'alignement 8. */
    .global fsm_init
    .type   fsm_init, %function
fsm_init:
    push    {r4, lr}
    movs    r1, #0
    ldr     r0, =etat
    movs    r2, #ETAT_ARRET
    str     r2, [r0]
    ldr     r0, =touch_enabled
    str     r1, [r0]
    ldr     r0, =compteur_transitions
    str     r1, [r0]
    ldr     r0, =prochain_sens
    str     r1, [r0]
    ldr     r0, =clignote_compteur
    str     r1, [r0]
    ldr     r0, =clignote_phase
    str     r1, [r0]
    ldr     r0, =touch_signal_compteur
    str     r1, [r0]
    ldr     r0, =blink_cycles
    str     r1, [r0]
    ldr     r0, =blink_cyccnt_prec
    str     r1, [r0]
    /* compteur de cycles DWT pour T6 */
    ldr     r0, =DEMCR
    ldr     r2, [r0]
    orr     r2, r2, #DEMCR_TRCENA
    str     r2, [r0]
    ldr     r0, =DWT_CTRL
    ldr     r2, [r0]
    orr     r2, r2, #DWT_CTRL_CYCCNTENA
    str     r2, [r0]
    bl      fsm_maj_del
    pop     {r4, pc}
    .size   fsm_init, .-fsm_init

/* void fsm_step(void)
 * Un pas de la machine à états, appelé toutes les PERIODE_SCRUTATION_MS.
 *   A. estop_flag -> ARRÊT_URGENCE (E4), initialisation du clignotement (E5)
 *   B. événements validés User et Touch En (toujours lus, pour entretenir
 *      l'anti-rebond même quand ils sont ignorés)
 *   C. en urgence : User ignoré (E5) ; acquittement Touch En si E-Stop relâché (E6)
 *      hors urgence : défilement User (E2) ; bascule touch_enabled (E7)
 *   D. mise à jour des DEL (fsm_maj_del)
 * Invariants (E9) : une seule DEL allumée (led_set, puis réaffirmation de
 * l'état sûr dans fsm_maj_del si un E-Stop est survenu pendant le pas) ;
 * aucune sortie d'ARRÊT_URGENCE sans acquittement (seul le chemin E6 écrit
 * ETAT_ARRET depuis l'urgence). Un second E-Stop pendant l'urgence, ou les
 * rebonds du relâchement d'E-Stop, consomment le drapeau sans rien changer :
 * le clignotement n'est pas relancé, la période mesurée (T6) reste stable.
 * AAPCS : push {r4, r5, r6, lr}. r4 = événement User, r5 = événement Touch,
 * r6 = &etat.                                                                */
    .global fsm_step
    .type   fsm_step, %function
fsm_step:
    push    {r4, r5, r6, lr}
    ldr     r6, =etat

    /* ---- A. drapeau E-Stop (positionné par l'ISR) ---- */
    ldr     r0, =estop_flag
    ldr     r1, [r0]
    cmp     r1, #0
    beq     fsm_step_lecture
    movs    r1, #0
    str     r1, [r0]                    /* drapeau consommé */
    ldr     r0, [r6]
    cmp     r0, #ETAT_ARRET_URGENCE
    beq     fsm_step_lecture            /* déjà en urgence : drapeau consommé, rien d'autre */
    movs    r0, #ETAT_ARRET_URGENCE
    str     r0, [r6]
    bl      fsm_compte_transition
    ldr     r0, =clignote_compteur      /* demi-période rouge allumée d'abord */
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, =touch_signal_compteur  /* annule une extinction brève en cours */
    str     r1, [r0]
    ldr     r0, =blink_cycles           /* T6 : mesure relancée à l'entrée en urgence */
    str     r1, [r0]
    ldr     r0, =DWT_CYCCNT
    ldr     r1, [r0]
    ldr     r0, =blink_cyccnt_prec
    str     r1, [r0]
    ldr     r0, =clignote_phase
    movs    r1, #1
    str     r1, [r0]

    /* ---- B. événements validés ---- */
fsm_step_lecture:
    movs    r0, #BTN_USER
    bl      button_pressed
    mov     r4, r0                      /* r4 = 1 si appui User validé */
    movs    r0, #BTN_TOUCH
    bl      button_pressed
    mov     r5, r0                      /* r5 = 1 si appui Touch En validé */

    /* ---- C. transitions ---- */
    ldr     r0, [r6]
    cmp     r0, #ETAT_ARRET_URGENCE
    bne     fsm_step_normal

    /* en urgence : E5 (User ignoré, r4 non utilisé) et E6 */
    cmp     r5, #0
    beq     fsm_step_maj
    movs    r0, #BTN_ESTOP
    bl      button_raw
    cmp     r0, #0
    bne     fsm_step_maj                /* E-Stop maintenu : acquittement refusé */
    movs    r0, #ETAT_ARRET
    str     r0, [r6]                    /* acquittement : retour en ARRÊT */
    bl      fsm_compte_transition
    b       fsm_step_maj

fsm_step_normal:
    /* E2 : défilement par User */
    cmp     r4, #0
    beq     fsm_step_touch
    ldr     r0, [r6]
    cmp     r0, #ETAT_ARRET
    bne     fsm_step_vers_arret
    ldr     r1, =prochain_sens
    ldr     r2, [r1]
    cmp     r2, #0
    bne     fsm_step_arriere
    movs    r0, #ETAT_MARCHE_AVANT      /* ARRÊT -> AVANT, prochain départ en arrière */
    movs    r2, #1
    b       fsm_step_depart
fsm_step_arriere:
    movs    r0, #ETAT_MARCHE_ARRIERE    /* ARRÊT -> ARRIÈRE, prochain départ en avant */
    movs    r2, #0
fsm_step_depart:
    str     r2, [r1]
    str     r0, [r6]
    bl      fsm_compte_transition
    b       fsm_step_touch
fsm_step_vers_arret:
    movs    r0, #ETAT_ARRET             /* AVANT ou ARRIÈRE -> ARRÊT */
    str     r0, [r6]
    bl      fsm_compte_transition

fsm_step_touch:
    /* E7 : Touch En hors urgence */
    cmp     r5, #0
    beq     fsm_step_maj
    ldr     r0, =touch_enabled
    ldr     r1, [r0]
    eor     r1, r1, #1
    str     r1, [r0]
    ldr     r0, =touch_signal_compteur
    movs    r1, #(TOUCH_SIGNAL_MS / PERIODE_SCRUTATION_MS)
    str     r1, [r0]

    /* ---- D. sorties ---- */
fsm_step_maj:
    bl      fsm_maj_del
    pop     {r4, r5, r6, pc}
    .size   fsm_step, .-fsm_step

/* static void fsm_compte_transition(void) — compteur_transitions++ (T3)
 * Routine feuille locale ; registres modifiés : r0, r1.                     */
    .type   fsm_compte_transition, %function
fsm_compte_transition:
    ldr     r0, =compteur_transitions
    ldr     r1, [r0]
    adds    r1, r1, #1
    str     r1, [r0]
    bx      lr
    .size   fsm_compte_transition, .-fsm_compte_transition

/* static void fsm_maj_del(void) — seul point d'appel de led_set
 * ARRÊT_URGENCE : rouge clignotante à 2 Hz (E5) — clignote_compteur compte
 *   les pas de scrutation, clignote_phase bascule toutes les
 *   CLIGNOTEMENT_DEMI_MS / PERIODE_SCRUTATION_MS. À chaque basculement,
 *   blink_cycles = DWT_CYCCNT - valeur au basculement précédent (T6).
 * Hors urgence : DEL de l'état (etat_vers_del), sauf pendant l'extinction
 *   brève de E7 (touch_signal_compteur > 0 : LED_AUCUNE, puis décrément).
 *   Puis, si estop_flag vaut 1 (ISR survenue pendant ce pas, éventuellement
 *   entre les deux écritures BSRR de led_set), l'état sûr est réaffirmé par
 *   led_set(LED_ROUGE) : quel que soit l'instant de l'ISR, la dernière
 *   écriture sur les DEL est l'état sûr (E4, E9 : jamais deux DEL allumées).
 * AAPCS : appelle led_set -> push {r4, lr}. r4 = état, r12 = temporaire.     */
    .type   fsm_maj_del, %function
fsm_maj_del:
    push    {r4, lr}
    ldr     r0, =etat
    ldr     r4, [r0]
    cmp     r4, #ETAT_ARRET_URGENCE
    bhi     fsm_maj_del_fin             /* état invalide : ne rien changer */
    beq     fsm_maj_del_urgence

    /* hors urgence : extinction brève (E7) puis DEL de l'état */
    ldr     r1, =touch_signal_compteur
    ldr     r2, [r1]
    cmp     r2, #0
    beq     fsm_maj_del_etat
    subs    r2, r2, #1
    str     r2, [r1]
    movs    r0, #LED_AUCUNE
    bl      led_set
    b       fsm_maj_del_verif_estop
fsm_maj_del_etat:
    ldr     r1, =etat_vers_del
    ldrb    r0, [r1, r4]                /* r0 = DEL associée à l'état */
    bl      led_set
fsm_maj_del_verif_estop:
    ldr     r1, =estop_flag             /* E-Stop pendant ce pas ? */
    ldr     r1, [r1]
    cmp     r1, #0
    beq     fsm_maj_del_fin
    movs    r0, #LED_ROUGE              /* oui : état sûr réaffirmé (E4, E9) */
    bl      led_set
    b       fsm_maj_del_fin

    /* urgence : clignotement à 2 Hz (E5) */
fsm_maj_del_urgence:
    ldr     r1, =clignote_compteur
    ldr     r2, [r1]
    adds    r2, r2, #1
    cmp     r2, #(CLIGNOTEMENT_DEMI_MS / PERIODE_SCRUTATION_MS)
    blo     fsm_maj_del_pas_bascule
    movs    r2, #0
    ldr     r3, =clignote_phase
    ldr     r0, [r3]
    eor     r0, r0, #1
    str     r0, [r3]
    /* T6 : écart de DWT_CYCCNT depuis le basculement précédent */
    ldr     r3, =DWT_CYCCNT
    ldr     r0, [r3]                    /* r0 = cycles courants */
    ldr     r3, =blink_cyccnt_prec
    ldr     r12, [r3]
    str     r0, [r3]
    sub     r12, r0, r12                /* écart (modulo 2^32) */
    ldr     r3, =blink_cycles
    str     r12, [r3]
fsm_maj_del_pas_bascule:
    str     r2, [r1]
    ldr     r3, =clignote_phase
    ldr     r0, [r3]
    cmp     r0, #0
    beq     fsm_maj_del_rouge_eteinte
    movs    r0, #LED_ROUGE
    bl      led_set
    b       fsm_maj_del_fin
fsm_maj_del_rouge_eteinte:
    movs    r0, #LED_AUCUNE
    bl      led_set
fsm_maj_del_fin:
    pop     {r4, pc}
    .size   fsm_maj_del, .-fsm_maj_del
