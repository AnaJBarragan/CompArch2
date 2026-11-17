/* ---------------------------------------------------------------------------
 * l3a_c.c — version C de la routine assembleur conservée pour L3-A
 *           (CEG 3536, laboratoire 3) — CORRIGÉ : motor_set_pwm
 *
 * Même prototype et même comportement que motor_set_pwm (motor.s, lab. 2) :
 *   pct > 100 saturé à 100 ; CCR1 = pct x (ARR + 1) / 100 ; retour = CCR1.
 * Compilé avec les mêmes options que le projet ; comparer à -O0 et -O2 le
 * désassemblage avec la version assembleur (section 4.5 de l'énoncé).
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "bench_l3a.h"

#define PWM_PLEINE_C 200u          /* ARR + 1, identique à registres_lab2.inc */

uint32_t motor_set_pwm_c(uint32_t pct)
{
    if (pct > 100u) {
        pct = 100u;
    }
    uint32_t ccr = pct * PWM_PLEINE_C / 100u;
    TIM1->CCR1 = ccr;
    return ccr;
}
