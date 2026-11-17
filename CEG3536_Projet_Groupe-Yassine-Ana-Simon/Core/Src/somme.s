/* ---------------------------------------------------------------------------
 * somme.s — somme de contrôle des trames de télémétrie
 *           (CEG 3536, laboratoire 4, E10) — SQUELETTE À COMPLÉTER
 *
 * Routine exportée (AAPCS) : trame_xor8 (prototype dans lab4.h)
 *
 * La somme de contrôle est le OU exclusif de tous les octets de la trame,
 * du premier caractère (« T », « E » ou « I ») à celui qui précède
 * l'astérisque. Elle est émise sur deux chiffres hexadécimaux en majuscules :
 *     T,1200,1,1,60,...,0*5C\r\n
 * Cas d'essai obligatoires (T9, autotest somme_autotest, commande série x) :
 *     chaîne vide          -> 0x00
 *     "A"                  -> 0x41
 *     "T,0"                -> 0x48
 *     "123456789"          -> 0x31
 *     octets 0x00 à 0xFF   -> 0x00
 * ------------------------------------------------------------------------- */
    .syntax unified
    .cpu    cortex-m33
    .thumb

    .text
    .align  2

/* uint32_t trame_xor8(const uint8_t *octets, uint32_t n)
 * r0 = adresse du premier octet ; r1 = nombre d'octets ; retour r0 = somme
 * (8 bits de poids faible, les autres à zéro).
 * Routine feuille : r0-r3 utilisables, rien à préserver, retour par bx lr.
 * Indices : traiter n = 0 avant la boucle (cbz) ; ldrb r3, [r0], #1 lit un
 * octet et avance le pointeur ; eors accumule ; subs / bne décompte.
 * Ne jamais lire au-delà de octets[n - 1].                                  */
    .global trame_xor8
    .type   trame_xor8, %function
trame_xor8:
    /* À COMPLÉTER */
    movs    r0, #0
    bx      lr
    .size   trame_xor8, .-trame_xor8
