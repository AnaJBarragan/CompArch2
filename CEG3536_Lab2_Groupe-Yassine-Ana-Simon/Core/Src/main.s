/* ---------------------------------------------------------------------------
 * main.s — point d'entrée et boucle principale (CEG 3536, laboratoire 1)
 *
 * Structure : gpio_init -> estop_init -> fsm_init, puis boucle infinie qui
 * appelle fsm_step toutes les PERIODE_SCRUTATION_MS millisecondes.
 * Le fichier de démarrage (Core/Startup, généré par STM32CubeIDE) initialise
 * la pile, copie .data, efface .bss, appelle SystemInit puis main.
 * ------------------------------------------------------------------------- */
#include "registres.inc"

    .syntax unified
    .cpu    cortex-m33
    .thumb

    .text
    .align  2

/* void SystemInit(void)
 * Appelé par le démarrage avant main. Au laboratoire 1, l'horloge reste
 * celle de la réinitialisation (MSI, 4 MHz) : rien à configurer.
 * Registres modifiés : aucun.                                               */
    .global SystemInit
    .type   SystemInit, %function
SystemInit:
    bx      lr
    .size   SystemInit, .-SystemInit

/* int main(void)
 * Ne retourne jamais.                                                       */
    .global main
    .type   main, %function
main:
    bl      gpio_init               /* horloges, DEL en sortie, boutons en entrée */
    bl      estop_init              /* EXTI2 / NVIC pour PB2 (à compléter)  */
    bl      fsm_init                /* état ARRÊT, DEL rouge seule (E1)     */

boucle_principale:
    bl      fsm_step                /* un pas : boutons, drapeau E-Stop, DEL */
    movs    r0, #PERIODE_SCRUTATION_MS
    bl      delay_ms
    b       boucle_principale
    .size   main, .-main
