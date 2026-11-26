/* ---------------------------------------------------------------------------
 * env.h — capteur environnemental sur SPI1 (CEG 3536, laboratoire 4, E7)
 * Température, humidité relative et pression ; une mesure par seconde, sans
 * jamais bloquer la boucle principale.
 * ------------------------------------------------------------------------- */
#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stdbool.h>
#include "lab4.h"

/* lit et vérifie l'identifiant, lit les coefficients ; false si l'identifiant
 * n'est pas celui attendu (le véhicule fonctionne alors sans ce capteur).     */
bool env_init(void);
/* à appeler chaque tick : déclenche une mesure toutes les ENV_PERIODE_MS,
 * attend sa fin en ticks, la lit et la convertit.                             */
void env_step(void);
/* copie la dernière mesure dans *m (t_c10, h_x10, p_hpa_x10, valide ; le champ
 * ta_c10 n'est pas touché). Retourne true une seule fois par nouvelle mesure. */
bool env_mesure(env_mesure_t *m);

uint8_t  env_identifiant(void);     /* octet lu au registre d'identification   */
uint32_t env_nb_mesures(void);      /* mesures abouties depuis env_init        */

#endif /* ENV_H */
