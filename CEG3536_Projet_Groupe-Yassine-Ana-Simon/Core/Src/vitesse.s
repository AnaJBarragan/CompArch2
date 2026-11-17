/* ---------------------------------------------------------------------------
 * vitesse.s — conversion des fronts d'encodeur en vitesse, virgule fixe
 *             (CEG 3536, laboratoire 3, E3) — CORRIGÉ
 *
 * Routine exportée (AAPCS) : enc_fronts_vers_rpm_q16
 *
 * Fenêtre de FENETRE_VITESSE_MS = 250 ms ; 748 fronts par tour de l'arbre de
 * sortie (2 fronts x 11 impulsions x 34). tr/min = fronts x 4 x 60 / 748
 * = fronts x 240 / 748 = fronts x 0,320856...
 * Format Q16.16 : résultat = tr/min x 65536 = fronts x (240 x 65536 / 748)
 * = fronts x 21027,59... -> constante entière k = 21028 (erreur relative
 * +0,002 % < 0,05 % exigé). Pas de débordement tant que fronts <= 204 249.
 * Vérification : 1870 fronts -> 39 322 360 = 600,01 tr/min (600,00 réels,
 * 2,5 tours par fenêtre : l'écart visible vient de l'arrondi de k).
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .equ RPM_Q16_K, 21028       /* identique à lab3.h                         */

    .text
    .align  2

/* uint32_t enc_fronts_vers_rpm_q16(uint32_t fronts)
 * r0 = fronts comptés dans la fenêtre ; retour r0 = tr/min en Q16.16.
 * Routine feuille : r0, r1 modifiés, rien à préserver, retour par bx lr.     */
    .global enc_fronts_vers_rpm_q16
    .type   enc_fronts_vers_rpm_q16, %function
enc_fronts_vers_rpm_q16:
    ldr     r1, =RPM_Q16_K
    mul     r0, r0, r1                  /* fronts x k (32 bits)              */
    bx      lr
    .size   enc_fronts_vers_rpm_q16, .-enc_fronts_vers_rpm_q16
