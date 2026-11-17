/* ---------------------------------------------------------------------------
 * i2c.h — pilote I2C2 maître (CEG 3536, laboratoire 4, E1)
 * Bus : SDA = PF0, SCL = PF1 (AF4, drain ouvert), 100 kHz.
 * Toutes les adresses sont sur 7 bits (I2C_ADR_x de lab4.h) ; le pilote les
 * décale lui-même dans le champ SADD de I2C_CR2.
 * Chaque opération est bornée dans le temps (I2C_DELAI_MS) et ne laisse jamais
 * le bus occupé : en cas d'erreur, elle retourne un statut et compte l'erreur.
 * Aucune opération n'est appelée depuis une interruption.
 * ------------------------------------------------------------------------- */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    I2C_OK = 0,
    I2C_NACK,       /* adresse ou octet non acquitté (NACKF)                   */
    I2C_BUS,        /* erreur de bus ou perte d'arbitrage (BERR, ARLO)         */
    I2C_DELAI       /* échéance I2C_DELAI_MS dépassée                          */
} i2c_statut_t;

void         i2c_init(void);
/* écriture de n octets (n = 0 : adresse seule, utilisée par le balayage) */
i2c_statut_t i2c_ecrire(uint8_t adr7, const uint8_t *d, uint32_t n);
/* lecture de n octets */
i2c_statut_t i2c_lire(uint8_t adr7, uint8_t *d, uint32_t n);
/* écriture de ne octets, redémarrage, puis lecture de nl octets */
i2c_statut_t i2c_ecrire_lire(uint8_t adr7, const uint8_t *e, uint32_t ne,
                             uint8_t *l, uint32_t nl);

/* balayage des adresses 0x08 à 0x77 : range dans adresses[] celles qui
 * acquittent (au plus max) et retourne leur nombre. À appeler avant la boucle
 * principale (112 transactions).                                              */
uint32_t     i2c_scan(uint8_t *adresses, uint32_t max);

uint32_t     i2c_erreurs(void);         /* opérations en échec depuis i2c_init */
uint32_t     i2c_transactions(void);    /* opérations lancées (T2 : aucune écriture inutile) */

/* essai E12 (commande série « d ») : pendant duree_ms, toute opération vers
 * adr7 échoue avec I2C_NACK, comme si le module était absent.                 */
void         i2c_simuler_absence(uint8_t adr7, uint32_t duree_ms);

#endif /* I2C_H */
