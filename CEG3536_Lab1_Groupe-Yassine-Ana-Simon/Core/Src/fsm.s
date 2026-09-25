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
    ldr 	r0, =prochain_sens // Initialize prochain_sens
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
    push    {r4, r5, r6, lr} // Push registres r4, r5, r6 et lr au stack (Pour les sauvegarder) (r6 pour garder alignement).

    /* ----- À COMPLÉTER : étapes A et B ----- */

// E2 Dessous
    movs	r0, #BTN_USER	//BTN_USER dans registre r0
    bl		button_pressed 	//On branche à la fonction button_pressed
    cmp		r0,	#1			//Si button_pressed n'est pas pesé on branche à la fonction fsm_step_fin
	bne		fsm_step_fin

   	ldr		r4, =etat		//On set l'addresse memoire de etat au registre r4
   	ldr		r0, [r4]		//Load la valeur de l'état actif au registre r0
   	cmp 	r0, #ETAT_ARRET	//Si l'etat actif n'est pas ETAT_ARRET (Avant ou Arriere) -> Vers_Arret. Sinon Skip line et continue
   	bne		fsm_vers_arret

	ldr     r5, =prochain_sens //On set l'addresse memoire de prochain_sens au registre r5
    ldr     r1, [r5]		   //Load la valeur du prochain sens au registre r1
    cmp		r1, #0			   // Si le prochain sens n'est pas 0 (par avant) on branche à la fonction fsm_marche_arriere
    bne		fsm_marche_arriere
    // Sinon Skip line et continue. Vue qu'on est à la fin, on tombera dans la fonction fsm_marche_avant prochainement.

// Fonction qui load l'état à ETAT_MARCHE_AVANT et change le prochain sens au sens arrière, ensuite branche à fsm_depart.
fsm_marche_avant:
	movs	r0,	#ETAT_MARCHE_AVANT
    movs	r1,	#1
    b 		fsm_depart
// Fonction qui load l'état à ETAT_MARCHE_ARRIERE et change le prochain sens au sens par avant, ensuite branche à fsm_depart.
fsm_marche_arriere:
	movs	r0, #ETAT_MARCHE_ARRIERE
	movs 	r1, #0
	b		fsm_depart
//Pour fsm_depart on stocke la nouvelle valeur de l'état en mémoire ainsi que celle de prochain sens. (Adresse tenu par registre r4 et r5 respectivement)
// Ensuite on branche à compte transition pour effectuer une transition et on va à la fin pour mettre à jour la DEL.
fsm_depart:
	str		r0, [r4]
	str		r1,	[r5]
	bl		fsm_compte_transition
	b		fsm_step_fin
// fsm_vers_arret effectue la transition vers l'etat arret, compte la transition et mets à jour la DEL en branchant à fsm_step_fin.
fsm_vers_arret:
	movs 	r0, #ETAT_ARRET
	str 	r0, [r4]
	bl		fsm_compte_transition
	b		fsm_step_fin

// Appelle fsm_maj_del qui effectue une mise à jour de la DEL, ainsi que pop le stack.
// Cette fonction fsm_step_fin démarque la fin d'une transition complete dans la machine à état.
fsm_step_fin:
    bl      fsm_maj_del
    pop     {r4, r5, r6, pc}
    .size   fsm_step, .-fsm_step

// Effectue l'incrémentation/compte les transitions
fsm_compte_transition:
    ldr     r0, =compteur_transitions
    ldr     r1, [r0]
    adds    r1, r1, #1
    str     r1, [r0]
    bx		lr



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
