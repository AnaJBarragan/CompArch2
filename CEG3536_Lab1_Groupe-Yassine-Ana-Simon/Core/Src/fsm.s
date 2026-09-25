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
prochain_sens			.space 	4   /*Variable inclut pour savoir quelle est prochain. 0 = par avant, 1 = par arrière*/
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
    ldr 	r0, =prochain_sens
    str		r1, [r0]
    bl      fsm_maj_del
    pop     {r4, pc}
    .size   fsm_init, .-fsm_init

/* void fsm_step(void)
 * Un pas de la machine à états, appelé toutes les PERIODE_SCRUTATION_MS.
 * Lit les événements validés (button_pressed) et le drapeau estop_flag,
 * applique les transitions E2 à E7, puis met à jour les DEL (fsm_maj_del).
 *
 * À COMPLÉTER — ordre recommandé :
 *   A. E4/E5 : si estop_flag == 1 : estop_flag = 0 ; etat = ETAT_ARRET_URGENCE ;
 *      clignote_compteur = 0 ; clignote_phase = 1.
 *   B. si etat == ETAT_ARRET_URGENCE :
 *        - E5 : les appuis sur User sont ignorés (appeler quand même
 *          button_pressed(BTN_USER) pour entretenir l'anti-rebond) ;
 *        - E6 : si button_pressed(BTN_TOUCH) == 1 ET button_raw(BTN_ESTOP) == 0
 *          (E-Stop relâché) : etat = ETAT_ARRET (acquittement). Sinon rester.
 *      sinon :
 *        - E2 : si button_pressed(BTN_USER) == 1 :
 *              ARRET -> MARCHE_AVANT, MARCHE_AVANT -> ARRET (puis ARRET -> MARCHE_ARRIERE
 *              au prochain appui, MARCHE_ARRIERE -> ARRET). Une variable "prochain sens"
 *              en .bss permet d'alterner avant/arrière à partir d'ARRÊT.
 *              compteur_transitions++ à chaque transition.
 *        - E7 : si button_pressed(BTN_TOUCH) == 1 : touch_enabled ^= 1 ;
 *              touch_signal_compteur = TOUCH_SIGNAL_MS / PERIODE_SCRUTATION_MS.
 *   C. bl fsm_maj_del.
 * Invariants (E9) : jamais deux DEL allumées (garanti par led_set) ; jamais de
 * sortie d'ARRÊT_URGENCE sans acquittement.
 * AAPCS : appelle d'autres routines -> push {r4, lr}.                         */
    .global fsm_step
    .type   fsm_step, %function
fsm_step:
    push    {r4, r5, r6, lr}

    /* ----- À COMPLÉTER : étapes A et B ----- */

    /*E2 Below*/
    movs	r0, #BTN_USER
    bl		button_pressed
    cmp		r0,	#1
	bne		fsm_step_fin
   	ldr		r0, =etat
   	ldr		r1, [r0]
   	cmp 	r1, #ETAT_ARRET
   	bne		fsm_vers_arret
	ldr     r2, =prochain_sens
    ldr     r3, [r2]
    cmp		r3, #0
    bne		fsm_marche_arriere
    movs	r0,	#ETAT_MARCHE_AVANT
    movs	r2,	#1
    b		fsm_depart

fsm_step_fin:
    bl      fsm_maj_del
    pop     {r4, r5, r6, pc}
    .size   fsm_step, .-fsm_step


fsm_marche_arriere:
	movs	r0, #ETAT_MARCHE_ARRIERE
	movs r2, #1


fsm_depart:
	str		r3, [r2]
	str		r0,	[r1]
	bl		fsm_compte_transition


fsm_vers_arret:
	movs 	r1, #ETAT_ARRET
	str 	r1, [r0]
	bl		fsm_compte_transition


fsm_compte_transition:
    ldr     r0, =compteur_transitions
    ldr     r1, [r0]
    adds    r1, r1, #1
    str     r1, [r0]



/* static void fsm_maj_del(void)  — routine locale, seul point d'appel de led_set
 * ARRÊT, MARCHE_AVANT, MARCHE_ARRIÈRE : DEL fixe d'après etat_vers_del.
 * ARRÊT_URGENCE : À COMPLÉTER (E5) — alterner rouge / aucune toutes les
 *   CLIGNOTEMENT_DEMI_MS / PERIODE_SCRUTATION_MS appels (clignote_compteur,
 *   clignote_phase).
 * Hors urgence : À COMPLÉTER (E7) — tant que touch_signal_compteur > 0,
 *   décrémenter et afficher LED_AUCUNE au lieu de la DEL de l'état.          */
    .type   fsm_maj_del, %function
fsm_maj_del:
    push    {r4, lr}
    ldr     r0, =etat
    ldr     r4, [r0]
    cmp     r4, #ETAT_ARRET_URGENCE
    bhi     fsm_maj_del_fin             /* état invalide : ne rien changer */

    /* ----- À COMPLÉTER : clignotement (E5) et extinction brève (E7) ----- */

    ldr     r1, =etat_vers_del
    ldrb    r0, [r1, r4]                /* r0 = DEL associée à l'état */
    bl      led_set
fsm_maj_del_fin:
    pop     {r4, pc}
    .size   fsm_maj_del, .-fsm_maj_del
