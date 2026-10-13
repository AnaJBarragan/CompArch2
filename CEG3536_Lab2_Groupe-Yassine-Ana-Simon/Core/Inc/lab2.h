/* ---------------------------------------------------------------------------
 * lab2.h — interface C des routines assembleur des laboratoires 1 et 2
 *          (CEG 3536, automne 2026). Utilisé à partir du laboratoire 3.
 * Toutes les routines respectent l'AAPCS : paramètres R0-R3, retour R0,
 * registres R4-R11 préservés, symboles globaux.
 * ------------------------------------------------------------------------- */
#ifndef LAB2_H
#define LAB2_H

#include <stdint.h>
#include "lab1.h"   /* DEL, boutons, états, gpio_init, led_set, button_*, delay_ms,
                       estop_init, fsm_init, fsm_step, etat, estop_flag, ... */

/* ---- Moteur (motor.s) --------------------------------------------------- */
#define MOTOR_ARRET     0u      /* motor_set_dir : commande nulle, STB = 0   */
#define MOTOR_AVANT     1u      /* AI1 = 1, AI2 = 0, STB = 1                 */
#define MOTOR_ARRIERE   2u      /* AI1 = 0, AI2 = 1, STB = 1                 */

void     motor_init(void);                 /* GPIO PE9/PB10/PB11/PE15, TIM1 PWM 20 kHz, CCR1 = 0 */
uint32_t motor_set_pwm(uint32_t pct);      /* 0..100 % -> CCR1 ; retourne la valeur écrite       */
uint32_t motor_set_dir(uint32_t dir);      /* 0 accepté, 1 refusé (CCR1 != 0)                    */
void     motor_stop_now(void);             /* CCR1 = 0, STB = 0, AI1 = AI2 = 0 (appelable d'une ISR) */

/* ---- Encodeur (encoder.s) ---------------------------------------------- */
#define SENS_IMMOBILE   0u
#define SENS_AVANT      1u
#define SENS_ARRIERE    2u

void     encoder_init(void);               /* PB0/PB1, EXTI0 sur les deux fronts, NVIC prio 1 */
uint32_t encoder_read(volatile uint32_t *sens); /* fronts depuis la dernière lecture ; *sens = 0/1/2 */

extern volatile int32_t  enc_count;        /* compteur signé de fronts (ISR EXTI0)      */
extern volatile uint32_t vitesse_fronts;   /* fronts dans la dernière fenêtre (500 ms)  */
extern volatile uint32_t sens_mesure;      /* SENS_x de la dernière fenêtre             */

/* ---- SysTick (systick.s) ----------------------------------------------- */
void     systick_init(uint32_t periode_us); /* 1000 -> LOAD = 3999 à 4 MHz, prio 2, IT activée */
/* delay_ms(uint32_t ms) : déclarée dans lab1.h, réécrite sur ticks_ms     */

extern volatile uint32_t ticks_ms;         /* millisecondes depuis systick_init         */
extern volatile uint32_t tick_1ms;         /* 1 à chaque interruption ; consommé par main */

/* ---- Buzzer (buzzer.s) ------------------------------------------------- */
void     buzzer_init(void);                /* PA0 en sortie, silence                    */
void     buzzer_set(uint32_t on);          /* 0 silence, 1 son (R0-R3 seulement)        */
void     buzzer_bip(uint32_t n);           /* n bips de BIP_MS, non bloquant            */
void     buzzer_step(void);                /* à appeler toutes les 1 ms (fsm_step)      */

extern volatile uint32_t buzzer_continu;   /* 1 = son continu forcé (ARRÊT_URGENCE)     */

/* ---- Machine à états (fsm.s) : variables ajoutées au laboratoire 2 ----- */
extern volatile uint32_t consigne_pct;     /* 25, 50, 75 ou 100 %                       */
extern volatile uint32_t pwm_pct;          /* rapport cyclique appliqué (rampe)         */

#endif /* LAB2_H */
