/* ---------------------------------------------------------------------------
 * env.c — capteur environnemental (CEG 3536, laboratoire 4, E7)
 *         — SQUELETTE À COMPLÉTER
 *
 * Repères (énoncé, 4.7 ; capteur BME680, À CONFIRMER dans la fiche technique
 * liée en annexe D du guide) :
 *   - premier octet d'un échange : bit 7 = 1 pour une lecture, 0 pour une
 *     écriture ; les 7 autres bits sont l'adresse du registre dans la page ;
 *   - deux pages, choisies par spi_mem_page (bit 4 du registre 0x73, accessible
 *     dans les deux pages) : page 0 = registres 0x80 à 0xFF, page 1 = registres
 *     0x00 à 0x7F. Changer de page seulement quand c'est nécessaire ;
 *   - identifiant : registre 0xD0 (page 0), valeur attendue 0x61 ;
 *   - coefficients : 25 octets à partir de 0x89 puis 16 octets à partir de
 *     0xE1 (page 0), à ranger dans cet ordre dans un tableau de 41 octets et à
 *     passer à bme680_coef_decoder ;
 *   - mesure forcée, sans capteur de gaz (page 1) : ctrl_hum (0x72) <- 0x01,
 *     ctrl_meas (0x74) <- 0x25 ; attendre que le bit new_data (bit 7) de 0x1D
 *     soit à 1 — quelques millisecondes, comptées en ticks, jamais en boucle
 *     d'attente ; lire en un seul échange les 8 registres 0x1F à 0x26 ;
 *   - conversion : bme680_brut_decoder puis bme680_temperature_c100 (d'abord :
 *     elle calcule t_fine), bme680_pression_pa, bme680_humidite_x1000 ; mettre
 *     à l'échelle de env_mesure_t (0,1 °C ; 0,1 % ; 0,1 hPa).
 * Une lecture de n registres est un échange de n + 1 octets : le premier
 * octet reçu est sans signification.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "spi.h"
#include "bme680_comp.h"
#include "env.h"

#define ENV_ID_ATTENDU      0x61u
#define ENV_ATTENTE_MAX_MS  50u     /* au-delà, la mesure est abandonnée       */

typedef enum {
    ENV_ABSENT,         /* identifiant incorrect : plus aucun échange          */
    ENV_REPOS,          /* attend la prochaine période                         */
    ENV_CONVERSION      /* mesure lancée, attend new_data                      */
} env_phase_t;

static env_phase_t   phase = ENV_ABSENT;
static bme680_coef_t coef;
static env_mesure_t  derniere;
static uint32_t      nouvelle, nb_mesures, t_phase;
static uint8_t       identifiant;

bool env_init(void)
{
    derniere.valide = 0u;
    nouvelle = nb_mesures = 0u;
    identifiant = 0u;
    phase = ENV_ABSENT;
    /* À COMPLÉTER : lire l'identifiant ; s'il est correct, lire les 41 octets
     * de coefficients, bme680_coef_decoder(..., &coef), phase = ENV_REPOS,
     * t_phase = ticks_ms.                                                     */
    (void)coef; (void)t_phase;
    return identifiant == ENV_ID_ATTENDU;
}

void env_step(void)
{
    /* À COMPLÉTER : machine à trois phases cadencée par ticks_ms.
     *   ENV_REPOS      : période écoulée -> lancer la mesure forcée ;
     *   ENV_CONVERSION : new_data à 1 -> lire, convertir, ranger dans derniere
     *                    (valide = 1), nouvelle = 1, nb_mesures++ ; au-delà de
     *                    ENV_ATTENTE_MAX_MS -> abandonner et revenir au repos. */
    (void)phase;
}

/* [FOURNI] */
bool env_mesure(env_mesure_t *m)
{
    if (!nouvelle) {
        return false;
    }
    nouvelle = 0u;
    m->t_c10 = derniere.t_c10;
    m->h_x10 = derniere.h_x10;
    m->p_hpa_x10 = derniere.p_hpa_x10;
    m->valide = derniere.valide;
    return true;
}

uint8_t  env_identifiant(void) { return identifiant; }
uint32_t env_nb_mesures(void)  { return nb_mesures; }
