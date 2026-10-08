/* ---------------------------------------------------------------------------
 * fsm.s — machine à états du panneau de commande (CEG 3536, laboratoire 1)
 *
 * Routines exportées : fsm_init (fournie), fsm_step (À COMPLÉTER)
 * Variables (.bss)   : etat, touch_enabled, compteur_transitions,
 *                      clignote_compteur, clignote_phase, touch_signal_compteur
 *
 * États (section 3) : ETAT_ARRET (rouge fixe), ETAT_MARCHE_AVANT (verte),
 *                     ETAT_MARCHE_ARRIERE (bleue), ETAT_ARRET_URGENCE (rouge 2 Hz)
 * Un seul point de mise à jour des DEL : fsm_maj_del (critère B3).
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
compteur_transitions:   .space  4   /* nombre de transitions validées (T3, watch) */
clignote_compteur:      .space  4   /* pas de scrutation écoulés dans la demi-période */
clignote_phase:         .space  4   /* 0 rouge éteinte, 1 rouge allumée (E5)     */
touch_signal_compteur:  .space  4   /* pas restants d'extinction brève (E7)      */
    .global prochain_sens
prochain_sens:          .space  4   /* 0 = prochaine marche avant, 1 = arrière   */

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
    ldr     r0, =clignote_compteur
    str     r1, [r0]
    ldr     r0, =clignote_phase
    str     r1, [r0]
    ldr     r0, =touch_signal_compteur
    str     r1, [r0]
    bl      fsm_maj_del
    pop     {r4, pc}
    .size   fsm_init, .-fsm_init

/* void fsm_step(void)
 * Un pas de la machine à états, appelé toutes les PERIODE_SCRUTATION_MS.
 * Lit les événements validés (button_pressed) et le drapeau estop_flag,
 * applique les transitions E2 à E7, puis met à jour les DEL (fsm_maj_del).
 *
 * AAPCS : appelle d'autres routines -> push {r4, lr}.                         */
    .global fsm_step
    .type   fsm_step, %function
fsm_step:
    push    {r4, lr}

    /* A. E4/E5 : si estop_flag == 1 */
    ldr     r0, =estop_flag
    ldr     r1, [r0]
    cmp     r1, #1
    bne     fsm_check_urgence

    /* estop_flag = 0 ; etat = ETAT_ARRET_URGENCE ; reset clignote */
    movs    r1, #0
    str     r1, [r0]                    /* estop_flag = 0 */
    ldr     r0, =etat
    movs    r1, #ETAT_ARRET_URGENCE
    str     r1, [r0]                    /* etat = ETAT_ARRET_URGENCE */
    ldr     r0, =clignote_compteur
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, =clignote_phase
    movs    r1, #1
    str     r1, [r0]
    b       fsm_step_fin

fsm_check_urgence:
    /* B. si etat == ETAT_ARRET_URGENCE */
    ldr     r0, =etat
    ldr     r1, [r0]
    ldr     r2, =ETAT_ARRET_URGENCE
    cmp     r1, r2
    bne     fsm_normal_state

    /* En urgence : entretenir l'anti-rebond User (ignoré) */
    movs    r0, #BTN_USER
    bl      button_pressed

    /* E6 : acquittement si Touch pressé ET E-Stop relâché (button_raw == 0) */
    movs    r0, #BTN_TOUCH
    bl      button_pressed
    cmp     r0, #1
    bne     fsm_step_fin

    movs    r0, #BTN_ESTOP
    bl      button_raw
    cmp     r0, #0
    bne     fsm_step_fin

    /* Acquittement valide -> retour à ETAT_ARRET */
    ldr     r0, =etat
    ldr     r2, =ETAT_ARRET
    str     r2, [r0]
    b       fsm_step_fin

fsm_normal_state:
    /* E2 : vérifier bouton User */
    movs    r0, #BTN_USER
    bl      button_pressed
    cmp     r0, #1
    bne     fsm_check_touch

    /* Transition User gérée */
    ldr     r0, =etat
    ldr     r1, [r0]                    /* r1 = état courant */

    ldr     r2, =ETAT_ARRET
    cmp     r1, r2
    bne     fsm_transition_vers_arret

    /* De ARRÊT vers MARCHE_AVANT ou MARCHE_ARRIERE selon prochain_sens */
    ldr     r2, =prochain_sens
    ldr     r3, [r2]
    cmp     r3, #1
    beq     fsm_vers_arriere

    /* Aller vers MARCHE_AVANT */
    ldr     r3, =ETAT_MARCHE_AVANT
    str     r3, [r0]
    movs    r3, #1
    str     r3, [r2]                    /* prochain_sens = 1 pour la prochaine fois */
    b       fsm_inc_trans

fsm_vers_arriere:
    /* Aller vers MARCHE_ARRIERE */
    ldr     r3, =ETAT_MARCHE_ARRIERE
    str     r3, [r0]
    movs    r3, #0
    str     r3, [r2]                    /* prochain_sens = 0 pour la prochaine fois */
    b       fsm_inc_trans

fsm_transition_vers_arret:
    /* De MARCHE_AVANT / MARCHE_ARRIERE vers ARRÊT */
    ldr     r2, =ETAT_ARRET
    str     r2, [r0]

fsm_inc_trans:
    /* compteur_transitions++ */
    ldr     r0, =compteur_transitions
    ldr     r1, [r0]
    adds    r1, r1, #1
    str     r1, [r0]

fsm_check_touch:
    /* E7 : vérifier bouton Touch */
    movs    r0, #BTN_TOUCH
    bl      button_pressed
    cmp     r0, #1
    bne     fsm_step_fin

    /* touch_enabled ^= 1 */
    ldr     r0, =touch_enabled
    ldr     r1, [r0]
    eors    r1, r1, #1
    str     r1, [r0]

    /* touch_signal_compteur = TOUCH_SIGNAL_MS / PERIODE_SCRUTATION_MS */
    ldr     r0, =touch_signal_compteur
    ldr     r1, =(TOUCH_SIGNAL_MS / PERIODE_SCRUTATION_MS)
    str     r1, [r0]

fsm_step_fin:
    bl      fsm_maj_del
    pop     {r4, pc}
    .size   fsm_step, .-fsm_step

/* static void fsm_maj_del(void)  — routine locale, seul point d'appel de led_set
 * ARRÊT, MARCHE_AVANT, MARCHE_ARRIÈRE : DEL fixe d'après etat_vers_del.
 * ARRÊT_URGENCE : clignotement rouge / aucune (E5).
 * Hors urgence : extinction brève si touch_signal_compteur > 0 (E7).         */
    .type   fsm_maj_del, %function
fsm_maj_del:
    push    {r4, lr}
    ldr     r0, =etat
    ldr     r4, [r0]
    cmp     r4, #ETAT_ARRET_URGENCE
    bhi     fsm_maj_del_fin             /* état invalide : ne rien changer */

    /* E5 : Si ARRÊT_URGENCE, clignotement 2 Hz (rouge / aucune) */
    ldr     r1, =ETAT_ARRET_URGENCE
    cmp     r4, r1
    bne     fsm_maj_del_touch_check

    ldr     r0, =clignote_compteur
    ldr     r1, [r0]
    adds    r1, r1, #1
    ldr     r2, =(CLIGNOTEMENT_DEMI_MS / PERIODE_SCRUTATION_MS)
    cmp     r1, r2
    blo     fsm_maj_del_phase_set

    movs    r1, #0                      /* reset compteur */
    str     r1, [r0]
    ldr     r0, =clignote_phase
    ldr     r1, [r0]
    eors    r1, r1, #1                  /* toggle phase */
    str     r1, [r0]
    b       fsm_maj_del_phase_apply

fsm_maj_del_phase_set:
    str     r1, [r0]                    /* stocker compteur incrémenté */

fsm_maj_del_phase_apply:
    ldr     r0, =clignote_phase
    ldr     r1, [r0]
    cmp     r1, #1
    beq     fsm_maj_del_rouge           /* phase 1 -> rouge allumée */
    movs    r0, #LED_AUCUNE
    b       fsm_maj_del_apply

fsm_maj_del_rouge:
    movs    r0, #LED_ROUGE
    b       fsm_maj_del_apply

fsm_maj_del_touch_check:
    /* E7 : Hors urgence, vérifier touch_signal_compteur */
    ldr     r0, =touch_signal_compteur
    ldr     r1, [r0]
    cmp     r1, #0
    ble     fsm_maj_del_normal_led

    /* Décrémenter le compteur d'extinction brève et afficher LED_AUCUNE */
    subs    r1, r1, #1
    str     r1, [r0]
    movs    r0, #LED_AUCUNE
    b       fsm_maj_del_apply

fsm_maj_del_normal_led:
    ldr     r1, =etat_vers_del
    ldrb    r0, [r1, r4]                /* r0 = DEL associée à l'état normal */

fsm_maj_del_apply:
    bl      led_set

fsm_maj_del_fin:
    pop     {r4, pc}
    .size   fsm_maj_del, .-fsm_maj_del
