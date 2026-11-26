/* ---------------------------------------------------------------------------
 * telem.h — télémétrie LPUART1 (port COM virtuel du ST-LINK)
 *           (CEG 3536, laboratoire 3, E10)
 * Émission par file logicielle vidée à chaque tick (non bloquante) ;
 * réception facultative d'un caractère de commande.
 * ------------------------------------------------------------------------- */
#ifndef TELEM_H
#define TELEM_H

#include <stdint.h>
#include <stdbool.h>
#include "lab4.h"     /* lab3.h, env_mesure_t, trame_xor8 */
#include "evq.h"

void telem_init(uint32_t bauds);
bool telem_put(const char *s);          /* false si la file d'émission est pleine */
void telem_step(void);                  /* à appeler chaque tick : vide la file   */
int  telem_rx(void);                    /* caractère reçu, ou -1                  */
void telem_entete(void);                /* ligne H,... (E1)                        */
/* ligne T,... : 24 colonnes (laboratoire 3, partie I, puis partie II :
 * mode_essai, segment, dist_x10, consigne_rpm, code_defaut, evq_max,
 * crit_max_cycles), puis *HH (E10, E21). Les cinq premières colonnes de la
 * partie II viennent de *v ; occupation_max (evq_max) et crit_max
 * (crit_max_cycles) sont passés par main.c.                               */
void telem_trame(const etat_vehicule_t *v, int32_t temp_c_x10,
                 uint32_t nb_evenements, uint32_t debordements,
                 const env_mesure_t *m, uint32_t i2c_err, uint32_t ticks_perdus,
                 uint32_t occupation_max, uint32_t crit_max);
void telem_evenement(const evq_event_t *e);                         /* ligne E,... */
void telem_balayage(const uint8_t *adresses, uint32_t n);           /* ligne I,... */
uint32_t telem_pertes(void);            /* caractères refusés (file pleine)       */

/* utilitaires de formatage entier (pas de printf sur la cible) */
char *telem_fmt_u32(char *p, uint32_t v);
char *telem_fmt_i32(char *p, int32_t v);

#endif /* TELEM_H */
