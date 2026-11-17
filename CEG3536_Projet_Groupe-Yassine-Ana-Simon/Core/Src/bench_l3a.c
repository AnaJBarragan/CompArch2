/* ---------------------------------------------------------------------------
 * bench_l3a.c — banc de comparaison assembleur / C (CEG 3536, lab. 3, L3-A)
 *               — FOURNI
 *
 * Appelle motor_set_pwm (assembleur, lab. 2) et motor_set_pwm_c (C, l3a_c.c)
 * sur les mêmes cas, compare la valeur de retour et l'effet de bord (TIM1_CCR1),
 * et mesure les cycles DWT de chaque appel (moyenne, coût de mesure déduit).
 * Résultats dans des variables globales (watch) ; retour = nombre d'écarts.
 * Adapter bench_cas et les deux appels si une autre routine est comparée
 * (button_pressed, led_set, encoder_read).
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab3.h"
#include "bench_l3a.h"

uint32_t bench_cas[BENCH_N_CAS] = { 0u, 1u, 25u, 50u, 75u, 99u, 100u, 0xFFFFFFFFu };
uint32_t bench_ret_asm[BENCH_N_CAS], bench_ret_c[BENCH_N_CAS];
uint32_t bench_ccr_asm[BENCH_N_CAS], bench_ccr_c[BENCH_N_CAS];
uint32_t bench_cycles_asm, bench_cycles_c;

static uint32_t cycles_vide(void)
{
    uint32_t d = DWT->CYCCNT;
    uint32_t f = DWT->CYCCNT;
    return f - d;
}

uint32_t bench_l3a(void)
{
    uint32_t ecarts = 0u, ca = 0u, cc = 0u;
    uint32_t vide = cycles_vide();

    for (uint32_t i = 0u; i < BENCH_N_CAS; i++) {
        uint32_t d, f;
        d = DWT->CYCCNT;
        bench_ret_asm[i] = motor_set_pwm(bench_cas[i]);
        f = DWT->CYCCNT;
        ca += f - d - vide;
        bench_ccr_asm[i] = TIM1->CCR1;

        TIM1->CCR1 = 0xFFFFu;                       /* valeur sentinelle       */
        d = DWT->CYCCNT;
        bench_ret_c[i] = motor_set_pwm_c(bench_cas[i]);
        f = DWT->CYCCNT;
        cc += f - d - vide;
        bench_ccr_c[i] = TIM1->CCR1;

        if (bench_ret_asm[i] != bench_ret_c[i] || bench_ccr_asm[i] != bench_ccr_c[i]) {
            ecarts++;
        }
    }
    (void)motor_set_pwm(0u);                        /* état sûr                */
    bench_cycles_asm = ca / BENCH_N_CAS;
    bench_cycles_c   = cc / BENCH_N_CAS;
    return ecarts;
}
