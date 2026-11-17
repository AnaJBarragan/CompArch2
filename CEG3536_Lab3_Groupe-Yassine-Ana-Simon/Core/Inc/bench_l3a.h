/* ---------------------------------------------------------------------------
 * bench_l3a.h — banc de comparaison assembleur / C (CEG 3536, lab. 3, L3-A)
 * ------------------------------------------------------------------------- */
#ifndef BENCH_L3A_H
#define BENCH_L3A_H

#include <stdint.h>

/* version C de la routine assembleur conservée (l3a_c.c) */
uint32_t motor_set_pwm_c(uint32_t pct);

/* exécute les cas d'essai sur les deux versions ; retourne le nombre d'écarts.
 * À appeler en ARRÊT seulement (CCR1 est remis à 0 à la fin).               */
uint32_t bench_l3a(void);

#define BENCH_N_CAS 8u
extern uint32_t bench_cas[BENCH_N_CAS];
extern uint32_t bench_ret_asm[BENCH_N_CAS], bench_ret_c[BENCH_N_CAS];
extern uint32_t bench_ccr_asm[BENCH_N_CAS], bench_ccr_c[BENCH_N_CAS];
extern uint32_t bench_cycles_asm, bench_cycles_c;   /* cycles DWT par appel (moyenne) */

#endif /* BENCH_L3A_H */
