/* ---------------------------------------------------------------------------
 * ioexp.h — objet « extenseur d'entrées-sorties » sur le bus I2C
 *           (CEG 3536, laboratoire 4, E2)
 * Deux instances : les huit DEL (I2C_ADR_DEL) et les huit commutateurs
 * (I2C_ADR_COMMUT). Le composant n'a aucun registre interne : l'octet écrit
 * fixe les huit broches, l'octet lu en retourne le niveau (énoncé, 4.7).
 * L'état de chaque instance est porté par sa structure : aucune variable
 * globale dans ioexp.c (même principe que l'objet acq du laboratoire 3).
 * ------------------------------------------------------------------------- */
#ifndef IOEXP_H
#define IOEXP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  adr7;          /* adresse sur 7 bits                              */
    uint8_t  sortie;        /* dernier octet écrit avec succès                 */
    uint8_t  entree;        /* dernier octet lu avec succès                    */
    uint8_t  sortie_connue; /* 0 tant qu'aucune écriture n'a abouti            */
    uint32_t ecritures;     /* transactions d'écriture lancées (T2)            */
    uint32_t echecs;        /* erreurs consécutives (remis à 0 sur un succès)  */
} ioexp_t;

/* Initialise la structure seulement : aucune transaction sur le bus. */
void ioexp_init(ioexp_t *x, uint8_t adr7);

/* Écrit valeur sur les huit broches. Aucune transaction si valeur est déjà
 * l'octet en place (E2) : retourne alors true sans toucher au bus.
 * Retourne false si la transaction a échoué (l'octet reste à réécrire).       */
bool ioexp_ecrire(ioexp_t *x, uint8_t valeur);

/* Lit les huit broches. Retourne false en cas d'échec (*valeur inchangé).     */
bool ioexp_lire(ioexp_t *x, uint8_t *valeur);

#endif /* IOEXP_H */
