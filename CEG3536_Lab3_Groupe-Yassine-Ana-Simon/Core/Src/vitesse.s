/* ---------------------------------------------------------------------------
 * vitesse.s — conversion des fronts d'encodeur en vitesse, virgule fixe
 *             (CEG 3536, laboratoire 3, E3)
 *
 * Routine exportée (AAPCS) : enc_fronts_vers_rpm_q16 (prototype dans lab3.h)
 *
 * Fenêtre de FENETRE_VITESSE_MS = 250 ms ; 748 fronts par tour de l'arbre de
 * sortie (2 fronts x 11 impulsions x 34). tr/min = fronts x 4 x 60 / 748
 * = fronts x 240 / 748.
 * Format Q16.16 : résultat = tr/min x 65536 = fronts x (240 x 65536 / 748).
 *   240 x 65536 / 748 = 21027,936... -> k = 21028 (arrondi)
 *   erreur relative de k : (21028 - 21027,936) / 21027,936 = +0,0019 % (< 0,05 %)
 *   pas de débordement 32 bits tant que fronts <= floor(0xFFFFFFFF / 21028)
 *   = 204 249 (le maximum attendu est ~1 100 fronts par fenêtre)
 * Cas de l'essai T3 (résultat Q16.16 -> tr/min) :
 *   0 -> 0 ; 1 -> 21028 (0,32) ; 748 -> 15 728 944 (240,00) ;
 *   1870 -> 39 322 360 (600,01) ; 65535 -> 1 378 069 980 (21 027,68)
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .equ RPM_Q16_K, 21028        /* identique à RPM_Q16_K de lab3.h          */

    .text
    .align  2

/* uint32_t enc_fronts_vers_rpm_q16(uint32_t fronts)
 * r0 = fronts comptés dans la fenêtre ; retour r0 = tr/min en Q16.16.
 * Routine feuille : r0-r1 seulement, rien à empiler, retour par bx lr.     */
    .global enc_fronts_vers_rpm_q16
    .type   enc_fronts_vers_rpm_q16, %function
enc_fronts_vers_rpm_q16:
    ldr     r1, =RPM_Q16_K          /* r1 = k (21028 ne tient pas dans movs)  */
    mul     r0, r0, r1              /* r0 = fronts x k (32 bits bas)          */
    bx      lr                      /* retour : r0 = tr/min x 65536           */
    .size   enc_fronts_vers_rpm_q16, .-enc_fronts_vers_rpm_q16
