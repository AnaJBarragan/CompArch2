/* ---------------------------------------------------------------------------
 * somme_test.c — banc de la routine assembleur trame_xor8
 *                (CEG 3536, laboratoire 4, E10, T9) — FOURNI
 * Les cinq cas obligatoires de l'énoncé. Le tampon du dernier cas est encadré
 * par deux octets de garde à 0xFF : une lecture hors des n octets fausse la
 * somme. À appeler depuis la commande série x ; le tableau attendu / obtenu
 * (somme_test_attendu, somme_test_obtenu) se relève aussi au débogueur.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "somme_test.h"

const uint32_t somme_test_attendu[SOMME_TEST_N_CAS] = { 0x00u, 0x41u, 0x48u, 0x31u, 0x00u };
uint32_t somme_test_obtenu[SOMME_TEST_N_CAS];

uint32_t somme_autotest(void)
{
    static uint8_t tous[258];
    uint32_t echecs = 0u;

    tous[0] = 0xFFu;                                /* garde avant           */
    for (uint32_t i = 0u; i < 256u; i++) {
        tous[i + 1u] = (uint8_t)i;
    }
    tous[257] = 0xFFu;                              /* garde après           */

    somme_test_obtenu[0] = trame_xor8((const uint8_t *)"", 0u);
    somme_test_obtenu[1] = trame_xor8((const uint8_t *)"A", 1u);
    somme_test_obtenu[2] = trame_xor8((const uint8_t *)"T,0", 3u);
    somme_test_obtenu[3] = trame_xor8((const uint8_t *)"123456789", 9u);
    somme_test_obtenu[4] = trame_xor8(&tous[1], 256u);

    for (uint32_t i = 0u; i < SOMME_TEST_N_CAS; i++) {
        if (somme_test_obtenu[i] != somme_test_attendu[i]) {
            echecs++;
        }
    }
    return echecs;
}
