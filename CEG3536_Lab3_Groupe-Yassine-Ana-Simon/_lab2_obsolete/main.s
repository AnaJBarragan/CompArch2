/* ---------------------------------------------------------------------------
 * main.s — point d'entrée et boucle principale (CEG 3536, laboratoire 1)
 *
 * Structure : gpio_init -> estop_init -> fsm_init, puis boucle infinie qui
 * appelle fsm_step toutes les PERIODE_SCRUTATION_MS millisecondes.
 * Le fichier de démarrage (Core/Startup, généré par STM32CubeIDE) initialise
 * la pile, copie .data, efface .bss, appelle SystemInit puis main.
 * ------------------------------------------------------------------------- */
#include "registres.inc"
#include "registres_lab2.inc" //ajoute Ana LAB 2

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

    /* ajoute Ana */
    bl      motor_init              /* TIM1 à 20 kHz, CCR1 = 0, STB = 0 (E1, E2)  */
    bl      buzzer_init             /* PA0 en sortie, silence                */
    bl      encoder_init            /* EXTI0, priorité 1 (E5)                */
    /* fin */

    bl      estop_init              /* EXTI2 / NVIC pour PB2 (à compléter) (priorité 0 (E7) )  */
    bl      fsm_init                /* état ARRÊT, DEL rouge seule (E1)     */

    /* ajoute Ana */
    ldr     r0, =SYSTICK_PERIODE_US
    bl      systick_init            /* en dernier : les ticks démarrent ici  */
    /* fin */

boucle_principale:
/* Boucle principale du lab 1:
    bl      fsm_step                // un pas : boutons, drapeau E-Stop, DEL
    movs    r0, #PERIODE_SCRUTATION_MS
    bl      delay_ms
    b       boucle_principale */
   
/* Boucle principale du lab 2: (ajoute Ana) */   
    ldr     r0, =tick_1ms
    ldr     r1, [r0]
    cmp     r1, #0
    beq     boucle_principale       /* attente du tick de 1 ms               */
    movs    r1, #0
    str     r1, [r0]                /* tick consommé                         */
    bl      fsm_step                /* exactement un pas par milliseconde    */
    b       boucle_principale
    .size   main, .-main
