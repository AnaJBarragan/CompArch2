/* ---------------------------------------------------------------------------
 * crit.s — sections critiques (CEG 3536, projet final, partie II, E13)
 *          — complété par l'équipe
 *
 * Routines exportées (AAPCS), prototypes dans projet.h :
 *     uint32_t crit_entrer(void);
 *     void     crit_sortir(uint32_t etat);
 *
 * La file d'événements est modifiée par des gestionnaires d'interruption
 * (E-Stop, PIR, rafale EXTI15) et lue par la boucle principale. Entre la
 * lecture d'un indice et sa réécriture, une interruption peut intervenir et
 * modifier le même indice : la mise à jour est perdue. Une section critique
 * rend ces quelques instructions indivisibles en masquant les interruptions.
 *
 * PRIMASK (bit 0) vaut 1 quand toutes les interruptions à priorité
 * configurable sont masquées (NMI et HardFault restent actives). MRS le lit,
 * MSR l'écrit ; CPSID i le met à 1.
 *
 * Pourquoi RESTAURER plutôt que démasquer (CPSIE i) : si une section est
 * ouverte dans une autre (ou dans un gestionnaire qui a déjà masqué), la
 * sortie de la section intérieure doit laisser PRIMASK à 1. En rendant à
 * PRIMASK la valeur lue à l'entrée, les sections s'imbriquent sans danger.
 *
 * Tant que PRIMASK vaut 1, même E-Stop (priorité 0) attend : la section doit
 * rester très courte (CRIT_MAX_CYCLES = 200 cycles). Mesure : CRIT_CYCLES et
 * CRIT_MESURER (projet.h), colonne crit_max_cycles de la télémétrie.
 *
 * Coût : crit_entrer = BL + 3 instructions, crit_sortir = BL + 2 instructions,
 * soit une dizaine de cycles à eux deux. Banc : crit_autotest (commande c).
 * ------------------------------------------------------------------------- */
    .syntax unified
    .cpu    cortex-m33
    .thumb

    .text
    .align  2

/* uint32_t crit_entrer(void)
 * retour r0 = valeur de PRIMASK AVANT le masquage (0 ou 1).
 * Routine feuille : seul r0 est modifié, rien à préserver, retour par bx lr.
 * L'ordre compte : lire d'abord, masquer ensuite (sinon on lirait toujours 1). */
    .global crit_entrer
    .type   crit_entrer, %function
crit_entrer:
    mrs     r0, primask                 /* r0 = état avant la section        */
    cpsid   i                           /* PRIMASK = 1 : interruptions masquées */
    bx      lr
    .size   crit_entrer, .-crit_entrer

/* void crit_sortir(uint32_t etat)
 * r0 = valeur retournée par le crit_entrer correspondant.
 * Rend à PRIMASK son état d'avant la section : 0 démasque, 1 laisse masqué.
 * Seul le bit 0 de r0 est utilisé par MSR PRIMASK.                           */
    .global crit_sortir
    .type   crit_sortir, %function
crit_sortir:
    msr     primask, r0                 /* PRIMASK = état sauvegardé         */
    bx      lr
    .size   crit_sortir, .-crit_sortir
