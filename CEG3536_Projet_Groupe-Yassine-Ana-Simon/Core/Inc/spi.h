/* ---------------------------------------------------------------------------
 * spi.h — pilote SPI1 maître (CEG 3536, laboratoire 4, E7)
 * SCK = PA5, MISO = PA6, MOSI = PA7 (AF5) ; sélection logicielle sur PD14.
 * 8 bits, mode 0 (CPOL = CPHA = 0), 1 MHz au plus.
 * ------------------------------------------------------------------------- */
#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void spi_init(void);
/* Échange n octets, encadrés par la sélection (PD14 à 0 pendant l'échange).
 * emis nul : des octets 0x00 sont émis ; recus nul : les octets reçus sont
 * jetés. Durée : environ 8 µs par octet à 1 MHz.                              */
void spi_echange(const uint8_t *emis, uint8_t *recus, uint32_t n);
/* ajout de l'équipe : échanges abandonnés sur échéance (attente bornée)     */
uint32_t spi_erreurs(void);

#endif /* SPI_H */
