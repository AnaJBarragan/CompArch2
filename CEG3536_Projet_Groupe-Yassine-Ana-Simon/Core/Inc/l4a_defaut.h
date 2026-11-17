/* ---------------------------------------------------------------------------
 * l4a_defaut.h — pilote de TouchPad défectueux de la composante L4-A
 *                (CEG 3536, laboratoire 4, section 4.5 de l'énoncé)
 * Ce pilote ne fait pas partie du véhicule : il est appelé à la place de
 * tp_init et tp_lire le temps de l'investigation (LISEZMOI.md, « L4-A »).
 * ------------------------------------------------------------------------- */
#ifndef L4A_DEFAUT_H
#define L4A_DEFAUT_H

#include <stdint.h>
#include <stdbool.h>

bool l4a_tp_init(void);                 /* false si une écriture a échoué      */
bool l4a_tp_lire(uint16_t *etat12);     /* false si la lecture a échoué        */

#endif /* L4A_DEFAUT_H */
