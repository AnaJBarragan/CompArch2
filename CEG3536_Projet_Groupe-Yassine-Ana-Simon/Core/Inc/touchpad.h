/* ---------------------------------------------------------------------------
 * touchpad.h — contrôleur tactile capacitif à 12 électrodes (I2C_ADR_TOUCHPAD)
 *              (CEG 3536, laboratoire 4, E4)
 * ------------------------------------------------------------------------- */
#ifndef TOUCHPAD_H
#define TOUCHPAD_H

#include <stdint.h>
#include <stdbool.h>

#define TP_N_ELECTRODES 12u

/* configuration complète du contrôleur ; false si une écriture a échoué */
bool tp_init(void);
/* état des 12 électrodes (bit i = électrode i touchée) ; cette lecture retire
 * la demande d'interruption THI. false si la transaction a échoué.           */
bool tp_lire(uint16_t *etat12);
/* traduit le passage de l'état avant à l'état apres en code de touche :
 * 0 à 9, TOUCHE_FN, TOUCHE_NEXT ; -1 s'il n'y a pas de nouvel appui, si
 * plusieurs électrodes sont touchées, ou si l'électrode n'est pas dans la
 * table. Fonction sans matériel.                                             */
int32_t tp_touche(uint16_t avant, uint16_t apres);

/* table électrode -> touche, RELEVÉE PAR L'ÉQUIPE (touchpad.c) */
extern const int8_t tp_table[TP_N_ELECTRODES];

#endif /* TOUCHPAD_H */
