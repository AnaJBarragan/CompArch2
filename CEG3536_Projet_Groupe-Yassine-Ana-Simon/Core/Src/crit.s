/* ---------------------------------------------------------------------------
 * crit.s — sections critiques (CEG 3536, projet final, partie II, E13)
 *          SQUELETTE À COMPLÉTER
 *
 * Routines exportées (AAPCS), prototypes dans projet.h :
 *     uint32_t crit_entrer(void);
 *     void     crit_sortir(uint32_t etat);
 *
 * La file d'événements est modifiée par des gestionnaires d'interruption
 * (E-Stop, PIR, THI, SysTick) et lue par la boucle principale. Entre la
 * lecture d'un indice et sa réécriture, une interruption peut intervenir et
 * modifier le même indice : la mise à jour est perdue. Une section critique
 * rend ces quelques instructions indivisibles en masquant les interruptions.
 *
 * Le registre PRIMASK du Cortex-M33 vaut 1 quand les interruptions à priorité
 * configurable sont masquées. Il se lit par MRS et s'écrit par MSR; CPSID i
 * le met à 1 et CPSIE i le remet à 0.
 *
 * Pourquoi restaurer plutôt que démasquer : si crit_sortir exécutait CPSIE i,
 * une section critique imbriquée dans une autre démasquerait les
 * interruptions au milieu de la section englobante. En rendant à PRIMASK la
 * valeur lue par crit_entrer, les sections s'imbriquent sans danger, y
 * compris lorsqu'elles sont appelées depuis un gestionnaire.
 *
 * Tant que PRIMASK vaut 1, même E-Stop (priorité 0) attend : la section doit
 * rester très courte (CRIT_MAX_CYCLES = 200 cycles), et sa durée se mesure
 * (macros CRIT_CYCLES et CRIT_MESURER de projet.h, colonne crit_max_cycles).
 *
 * Banc d'essai : crit_autotest() dans crit_test.c (commande série c), cinq cas.
 * Tant que ce squelette n'est pas complété, crit_entrer ne masque rien : le
 * projet fonctionne comme à la partie I, sans protection, et le banc échoue.
 * ------------------------------------------------------------------------- */
    .syntax unified
    .cpu    cortex-m33
    .thumb

    .text
    .align  2

/* uint32_t crit_entrer(void)
 * retour r0 = valeur de PRIMASK AVANT le masquage (0 ou 1).
 * Routine feuille : seul r0 est utilisé, rien à préserver, retour par bx lr.
 * Indices : mrs r0, primask lit l'état courant ; cpsid i masque ensuite.
 * L'ordre compte : lire d'abord, masquer après.                              */
    .global crit_entrer
    .type   crit_entrer, %function
crit_entrer:
    /* À COMPLÉTER */
    movs    r0, #0
    bx      lr
    .size   crit_entrer, .-crit_entrer

/* void crit_sortir(uint32_t etat)
 * r0 = valeur retournée par le crit_entrer correspondant.
 * Indice : msr primask, r0 rend à PRIMASK son état d'avant la section.
 * Ne pas utiliser cpsie i (voir l'en-tête du fichier).                        */
    .global crit_sortir
    .type   crit_sortir, %function
crit_sortir:
    /* À COMPLÉTER */
    bx      lr
    .size   crit_sortir, .-crit_sortir
