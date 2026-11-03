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
#include "lab3.h"

void telem_init(uint32_t bauds);
bool telem_put(const char *s);          /* false si la file d'émission est pleine */
void telem_step(void);                  /* à appeler chaque tick : vide la file   */
int  telem_rx(void);                    /* caractère reçu, ou -1                  */
void telem_entete(void);                /* ligne H,... (E1)                        */
void telem_trame(const etat_vehicule_t *v, int32_t temp_c_x10,
                 uint32_t nb_evenements, uint32_t debordements);   /* ligne T,... */
uint32_t telem_pertes(void);            /* caractères refusés (file pleine)       */
void telem_evenement(const char *nom, uint32_t t_ms);  /* ligne E,<t_ms>,<nom> (E9) */

/* utilitaires de formatage entier (pas de printf sur la cible) */
char *telem_fmt_u32(char *p, uint32_t v);
char *telem_fmt_i32(char *p, int32_t v);

#endif /* TELEM_H */
