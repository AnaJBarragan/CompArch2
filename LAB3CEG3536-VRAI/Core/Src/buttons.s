/* ---------------------------------------------------------------------------
 * buttons.s — lecture des boutons, anti-rebond
 *             (CEG 3536, laboratoire 1) — CORRIGÉ
 *
 * Routines exportées (AAPCS) : button_raw, button_pressed
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
 * 0 sinon. Appelée toutes les PERIODE_SCRUTATION_MS.
 *
 * Anti-rebond par échantillons consécutifs (E3) :
 *   - si le niveau brut == btn_valide[id] : compteur = 0, retour 0 ;
 *   - sinon compteur++ ; tant que compteur < ANTIREBOND_MS / PERIODE_SCRUTATION_MS,
 *     retour 0 (un rebond qui revient au niveau validé remet le compteur à 0) ;
 *   - à la fenêtre atteinte : btn_valide[id] = niveau, compteur = 0,
 *     retour 1 si le nouveau niveau est « appuyé », 0 s'il est « relâché ».
 * Un appui maintenu ne produit donc qu'un seul événement.
 *
 * AAPCS : appelle button_raw -> LR sauvegardé ; push {r4, r5, r6, lr} =
 * 16 octets, pile alignée sur 8. r4 = id, r5 = niveau brut, r6 = &btn_valide.  */
    .global button_pressed
    .type   button_pressed, %function
button_pressed:
    push    {r4, r5, r6, lr}
    cmp     r0, #2
    bhi     button_pressed_non
    mov     r4, r0                      /* r4 = id */
    bl      button_raw
    mov     r5, r0                      /* r5 = niveau brut normalisé */
    ldr     r6, =btn_valide
    ldr     r1, [r6, r4, lsl #2]        /* r1 = niveau validé courant */
    ldr     r2, =btn_compteur
    cmp     r5, r1
    bne     button_pressed_different
    movs    r0, #0                      /* même niveau : rien de nouveau */
    str     r0, [r2, r4, lsl #2]        /* compteur = 0 */
    pop     {r4, r5, r6, pc}
button_pressed_different:
    ldr     r0, [r2, r4, lsl #2]
    adds    r0, r0, #1
    str     r0, [r2, r4, lsl #2]
    cmp     r0, #(ANTIREBOND_MS / PERIODE_SCRUTATION_MS)
    blo     button_pressed_non          /* fenêtre pas encore écoulée */
    str     r5, [r6, r4, lsl #2]        /* niveau stable : validé */
    movs    r0, #0
    str     r0, [r2, r4, lsl #2]        /* compteur = 0 */
    mov     r0, r5                      /* 1 = front d'appui, 0 = relâchement */
    pop     {r4, r5, r6, pc}
button_pressed_non:
    movs    r0, #0
    pop     {r4, r5, r6, pc}
    .size   button_pressed, .-button_pressed
