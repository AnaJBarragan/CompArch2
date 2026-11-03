/* ---------------------------------------------------------------------------
 * l3a_c.c — version C de la routine assembleur conservée pour L3-A
 *           (CEG 3536, laboratoire 3)
 *
 * Routine choisie : motor_set_pwm (motor.s, laboratoire 2), inchangée.
 * Même prototype et même comportement :
 *   - pct > 100 est ramené à 100 (motor.s : cmp r0, #100 / it hi / movhi)
 *   - CCR1 = pct x PWM_PLEINE / 100, avec PWM_PLEINE = ARR + 1 = 200
 *     (4 MHz / 20 kHz, registres_lab2.inc)
 *   - retour : la valeur écrite dans CCR1
 * Comparée par bench_l3a.c (commande série b) : retour, CCR1 et cycles DWT.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "bench_l3a.h"

#define PWM_PLEINE_C 200u        /* = PWM_PLEINE de registres_lab2.inc      */

uint32_t motor_set_pwm_c(uint32_t pct)
{
    if (pct > 100u) {
        pct = 100u;                              /* saturation à 100 %      */
    }
    uint32_t ccr = pct * PWM_PLEINE_C / 100u;    /* rapport cyclique -> CCR1 */
    TIM1->CCR1 = ccr;                            /* effet de bord           */
    return ccr;                                  /* valeur écrite (R0)      */
}
