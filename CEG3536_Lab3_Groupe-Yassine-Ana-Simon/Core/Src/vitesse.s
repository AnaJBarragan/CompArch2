/* ---------------------------------------------------------------------------
 * vitesse.s — conversion des fronts d'encodeur en vitesse, virgule fixe
 *             (CEG 3536, laboratoire 3, E3) — SQUELETTE À COMPLÉTER
 *
 * Routine exportée (AAPCS) : enc_fronts_vers_rpm_q16 (prototype dans lab3.h)
 *
 * Fenêtre de FENETRE_VITESSE_MS = 250 ms ; 748 fronts par tour de l'arbre de
 * sortie (2 fronts x 11 impulsions x 34). tr/min = fronts x 4 x 60 / 748
 * = fronts x 240 / 748.
 * Format Q16.16 : résultat = tr/min x 65536 = fronts x (240 x 65536 / 748).
 * À faire : calculer la constante entière k (arrondie), son erreur relative,
 * la valeur maximale de fronts sans débordement 32 bits, et vérifier sur les
 * cas de l'essai T3 (0, 1, 748, 1870, 65535). Repère : le banc du laboratoire 2
 * donnait ~350 tr/min à pleine vitesse, soit ~1 090 fronts par fenêtre.
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .equ RPM_Q16_K, 0            /* À COMPLÉTER : identique à RPM_Q16_K de lab3.h */

    .text
    .align  2

/* uint32_t enc_fronts_vers_rpm_q16(uint32_t fronts)
 * r0 = fronts comptés dans la fenêtre ; retour r0 = tr/min en Q16.16.
 * Routine feuille : r0-r3 utilisables, rien à préserver, retour par bx lr.
 * Indice : ldr r1, =RPM_Q16_K puis mul (32 bits suffisent si fronts <= 204 249 ;
 * umull si l'on veut 64 bits).                                              */
    .global enc_fronts_vers_rpm_q16
    .type   enc_fronts_vers_rpm_q16, %function
enc_fronts_vers_rpm_q16:
    /* À COMPLÉTER */
    movs    r0, #0
    bx      lr
    .size   enc_fronts_vers_rpm_q16, .-enc_fronts_vers_rpm_q16
