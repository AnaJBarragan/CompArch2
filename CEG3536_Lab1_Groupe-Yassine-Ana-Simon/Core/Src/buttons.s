/* ---------------------------------------------------------------------------
 * buttons.s — lecture des boutons, anti-rebond, temporisation
 *             (CEG 3536, laboratoire 1)
 *
 * Routines exportées (AAPCS) : button_raw (fournie), button_pressed (À FAIRE),
 *                              delay_ms (fournie, boucle calibrée)
 * Variables (.bss)            : btn_valide[3], btn_compteur[3]
 * ------------------------------------------------------------------------- */
#include "registres.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

/* ---- Variables d'état des boutons (un mot par bouton : User, E-Stop, Touch) */
    .bss
    .align  2
    .global btn_valide
btn_valide:     .space  3 * 4       /* dernier niveau validé (0 relâché, 1 appuyé) */
    .global btn_compteur
btn_compteur:   .space  3 * 4       /* échantillons consécutifs différents de btn_valide */

/* ---- Table de description des boutons : port, broche, niveau actif ------ */
    .section .rodata
    .align  2
btn_table:
    .word   GPIOC_BASE, BTN_USER_PIN,  BTN_USER_ACTIF_HAUT     /* BTN_USER  = 0 */
    .word   GPIOB_BASE, BTN_ESTOP_PIN, BTN_ESTOP_ACTIF_HAUT    /* BTN_ESTOP = 1 */
    .word   GPIOB_BASE, BTN_TOUCH_PIN, BTN_TOUCH_ACTIF_HAUT    /* BTN_TOUCH = 2 */

    .text
    .align  2

/* uint32_t button_raw(uint32_t id)
 * r0 = BTN_USER (0), BTN_ESTOP (1), BTN_TOUCH (2)
 * Retour r0 = 1 si le bouton est appuyé, 0 sinon (niveau IDR normalisé
 * d'après BTN_x_ACTIF_HAUT). Identifiant invalide : retourne 0.
 * Registres modifiés : r0-r3 (routine feuille).                             */
    .global button_raw
    .type   button_raw, %function
button_raw:
    cmp     r0, #2
    bhi     button_raw_invalide
    ldr     r1, =btn_table
    add     r1, r1, r0, lsl #3          /* r1 += id * 8  */
    add     r1, r1, r0, lsl #2          /* r1 += id * 4  -> id * 12 */
    ldr     r2, [r1, #0]                /* base du port  */
    ldr     r3, [r1, #4]                /* numéro de broche */
    ldr     r1, [r1, #8]                /* 1 = actif haut */
    ldr     r2, [r2, #GPIO_IDR]
    lsr     r2, r2, r3
    and     r2, r2, #1                  /* niveau brut de la broche */
    eor     r0, r2, r1                  /* niveau XOR actif_haut ...  */
    eor     r0, r0, #1                  /* ... XOR 1 = appuyé (1) / relâché (0) */
    bx      lr
button_raw_invalide:
    movs    r0, #0
    bx      lr
    .size   button_raw, .-button_raw

/* uint32_t button_pressed(uint32_t id)
 * r0 = identifiant. Retour r0 = 1 si un appui vient d'être VALIDÉ (front),
 * 0 sinon. Doit être appelée périodiquement (toutes les PERIODE_SCRUTATION_MS).
 *
 * À FAIRE (E3) — anti-rebond par échantillons consécutifs :
 *   1. lire le niveau brut : bl button_raw (r0 = 0/1)
 *   2. si niveau == btn_valide[id] : btn_compteur[id] = 0 ; retourner 0
 *   3. sinon btn_compteur[id]++ ;
 *      si btn_compteur[id] < ANTIREBOND_MS / PERIODE_SCRUTATION_MS : retourner 0
 *   4. sinon (niveau stable depuis la fenêtre) : btn_valide[id] = niveau ;
 *      btn_compteur[id] = 0 ; retourner 1 si niveau == 1 (front d'appui), 0 sinon
 *   Un appui maintenu ne produit qu'un seul événement ; le relâchement est
 *   validé avec la même fenêtre mais ne retourne jamais 1.
 *
 * AAPCS : la routine appelle button_raw, donc LR est sauvegardé. On empile
 * {r4, r5, r6, lr} (16 octets) pour garder la pile alignée sur 8 octets à
 * l'appel de button_raw. r4 = id, r5 = niveau brut, r6 = adresse de tableau. */
    .global button_pressed
    .type   button_pressed, %function
button_pressed:
    push    {r4, r5, r6, lr}
    cmp     r0, #2                      // C = 1 if r0>=2; else C = 0. 
    bhi     button_pressed_non          // Branch conditional on C = 1 and Z = 0
    mov     r4, r0                      /* r4 = id */
    bl      button_raw
    mov     r5, r0                      /* r5 = niveau brut normalisé */

    /* ----- À COMPLÉTER : étapes 2 à 4 ci-dessus -----
     * Indices : ldr r6, =btn_valide ; ldr r0, [r6, r4, lsl #2]
     *           ldr r6, =btn_compteur ; ... ; str r0, [r6, r4, lsl #2]
     */
    @ ldr r6, =btn_valide 
    @ ldr r0, [r6, r4, lsl #2] // Multiply r4x4 () //Store last value of btn_valide for that button id. IE was it active before?
    @ ldr r6, =btn_compteur // Store value of 

    @ str r0, [r6, r4, lsl #2]

    /* ----- À COMPLÉTER : étapes 2 à 4 ci-dessus -----
     * Indices : ldr r6, =btn_valide ; ldr r0, [r6, r4, lsl #2]
     *           ldr r6, =btn_compteur ; ... ; str r0, [r6, r4, lsl #2]
     */


	/*E3- ANTI-REBOND*/


/* PSEUDOCODE
*Anti_rebond = 30
Ms = 1
r0 + appuie VALIDE =  1
r0+ appuie pas VALIDE = 0


if(id>2) {return 0)

niveau brut = button_raw(id)

if(niveau brut = niveau Valide) {no Signal change; return 0 }

if(niveau_brut = btn_valide(id)){btn_compteur(id) = 0; retunr 0}

if(niveau_brut != niveau valide) {
		if( btn_compteur < ANTIREBOND_MS / PERIODE_SCRUTATION_MS : retourner 0) {   }

 }


*/

@     ldr r6, =btn_valide                /* step 2: comparer avec la valeur validée */
@     ldr r0, [r6, r4, lsl #2]
@     cmp r0, r5
@     bne btn_step3

@     /* niveau inchangé: remettre le compteur à 0 et retourner 0 */
@     ldr r6, =btn_compteur
@     movs r0, #0
@     str r0, [r6, r4, lsl #2]
@     pop {r4, r5, r6, pc}


@ //STEP 3
@ btn_step3: /*step 3 ==> ELSE : compteur[id] ++  */
@     ldr r6, =btn_compteur
@     ldr r0, [r6, r4, lsl #2]
@     adds r0, r0, #1
@     str r0, [r6, r4, lsl #2]


@ 	/* STEP 4	*/
@     cmp r0, #30
@     bge btn_step4
@     movs r0, #0
@     pop {r4, r5, r6, pc}

@ //STEP4
@ btn_step4: /*4. sinon (niveau stable depuis la fenêtre) : btn_valide[id] = niveau ;
@  *      btn_compteur[id] = 0 ; retourner 1 si niveau == 1 (front d'appui), 0 sinon*/
@     ldr r6, =btn_valide
@     str r5, [r6, r4, lsl #2]


@     ldr r6, =btn_compteur
@     movs r0, #0
@     str r0, [r6, r4, lsl #2]

@     mov r0, r5
@     pop {r4, r5, r6, pc}

/* ----- Étape 2 à 4 : Logique d'anti-rebond ----- */
    ldr     r6, =btn_valide
    ldr     r2, [r6, r4, lsl #2]        /* r2 = btn_valide[id] */
    cmp     r5, r2                      /* niveau == btn_valide[id] ? */
    bne     btn_diff                    /* non -> incrémenter le compteur */

    /* Étape 2 : niveau inchangé -> réinitialiser le compteur et retourner 0 */
    ldr     r6, =btn_compteur
    movs    r0, #0
    str     r0, [r6, r4, lsl #2]        /* btn_compteur[id] = 0 */
    b       button_pressed_non          /* retourne 0 via le gestionnaire de sortie existant */

btn_diff:
    /* Étape 3 : niveau différent -> incrémenter le compteur */
    ldr     r6, =btn_compteur
    ldr     r2, [r6, r4, lsl #2]        /* r2 = btn_compteur[id] */
    adds    r2, r2, #1                  /* r2++ */
    str     r2, [r6, r4, lsl #2]        /* stocker btn_compteur[id] */

    /* Comparer avec le seuil (ANTIREBOND_MS / PERIODE_SCRUTATION_MS) */
    ldr     r3, =(ANTIREBOND_MS / PERIODE_SCRUTATION_MS)
    cmp     r2, r3
    blo     button_pressed_non          /* si < seuil, retourne 0 */

    /* Étape 4 : niveau stable -> mettre à jour btn_valide[id] et réinitialiser */
    ldr     r6, =btn_valide
    str     r5, [r6, r4, lsl #2]        /* btn_valide[id] = niveau (r5) */

    ldr     r6, =btn_compteur
    movs    r0, #0
    str     r0, [r6, r4, lsl #2]        /* btn_compteur[id] = 0 */

    mov     r0, r5                      /* retourne 1 si appui, 0 si relâchement */
    pop     {r4, r5, r6, pc}            /* sortie directe */

button_pressed_non:
    movs    r0, #0                      /* squelette : aucun événement */
    pop     {r4, r5, r6, pc}
    .size   button_pressed, .-button_pressed

/* void delay_ms(uint32_t ms)
 * Temporisation par boucle calibrée (admise au laboratoire 1, E8).
 * Hypothèse : horloge MSI de 4 MHz après réinitialisation, code en flash.
 * DELAY_BOUCLES_PAR_MS (registres.inc) doit être CALIBRÉE par mesure au point
 * de test et la valeur retenue rapportée. SysTick sera exigé au laboratoire 2.
 * Registres modifiés : r0, r1 (routine feuille).                            */
    .global delay_ms
    .type   delay_ms, %function
delay_ms:
    cbz     r0, delay_ms_fin
delay_ms_boucle_ms:
    ldr     r1, =DELAY_BOUCLES_PAR_MS
delay_ms_boucle_int:
    subs    r1, r1, #1
    bne     delay_ms_boucle_int
    subs    r0, r0, #1
    bne     delay_ms_boucle_ms
delay_ms_fin:
    bx      lr
    .size   delay_ms, .-delay_ms
