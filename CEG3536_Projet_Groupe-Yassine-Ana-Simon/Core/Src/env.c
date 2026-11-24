/* ---------------------------------------------------------------------------
 * env.c — capteur environnemental (CEG 3536, laboratoire 4, E7)
 *         — complété par l'équipe
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

/* Registres du BME680 (fiche technique, section 5.3) */
#define REG_ETAT_MESURE     0x1Du   /* bit 7 : new_data_0                      */
#define REG_DONNEES         0x1Fu   /* 0x1F à 0x26 : P (20 b), T (20 b), H (16 b) */
#define REG_CTRL_HUM        0x72u   /* osrs_h                                  */
#define REG_STATUS          0x73u   /* bit 4 : spi_mem_page                    */
#define REG_CTRL_MEAS       0x74u   /* osrs_t, osrs_p, mode                    */
#define REG_COEF1           0x89u   /* 25 octets                               */
#define REG_ID              0xD0u
#define REG_COEF2           0xE1u   /* 16 octets                               */
#define NEW_DATA            0x80u
#define CTRL_HUM_X1         0x01u   /* humidité suréchantillonnée x 1          */
#define CTRL_MEAS_FORCEE    0x25u   /* T x 1 (001), P x 1 (001), mode forcé (01) */
#define ENV_N_MAX           25u     /* plus longue lecture (coefficients)      */

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
static uint8_t       page_courante = 0xFFu;     /* inconnue au départ          */

/* Choisit la page qui contient reg : page 0 = 0x80..0xFF, page 1 = 0x00..0x7F.
 * Le registre 0x73 est visible dans les deux pages.                         */
static void choisir_page(uint8_t reg)
{
    uint8_t p = (reg < 0x80u) ? 1u : 0u;
    uint8_t tx[2];

    if (p == page_courante) {
        return;
    }
    tx[0] = REG_STATUS & 0x7Fu;                 /* bit 7 = 0 : écriture        */
    tx[1] = (uint8_t)(p << 4);                  /* spi_mem_page                */
    spi_echange(tx, (uint8_t *)0, 2u);
    page_courante = p;
}

/* Lecture de n registres consécutifs : un échange de n + 1 octets (le premier
 * octet reçu, pendant l'envoi de l'adresse, est sans signification).        */
static void lire(uint8_t reg, uint8_t *d, uint32_t n)
{
    uint8_t tx[ENV_N_MAX + 1u] = { 0u };
    uint8_t rx[ENV_N_MAX + 1u];

    if (n > ENV_N_MAX) {
        return;
    }
    choisir_page(reg);
    tx[0] = (uint8_t)(reg | 0x80u);             /* bit 7 = 1 : lecture         */
    spi_echange(tx, rx, n + 1u);
    for (uint32_t i = 0u; i < n; i++) {
        d[i] = rx[i + 1u];
    }
}

static void ecrire(uint8_t reg, uint8_t valeur)
{
    uint8_t tx[2];

    choisir_page(reg);
    tx[0] = (uint8_t)(reg & 0x7Fu);             /* bit 7 = 0 : écriture        */
    tx[1] = valeur;
    spi_echange(tx, (uint8_t *)0, 2u);
}

bool env_init(void)
{
    uint8_t brut[BME680_N_COEF];

    derniere.valide = 0u;
    nouvelle = nb_mesures = 0u;
    identifiant = 0u;
    phase = ENV_ABSENT;
    page_courante = 0xFFu;

    lire(REG_ID, &identifiant, 1u);
    if (identifiant != ENV_ID_ATTENDU) {
        return false;                           /* véhicule sans ce capteur    */
    }
    lire(REG_COEF1, &brut[0], 25u);
    lire(REG_COEF2, &brut[25], 16u);
    bme680_coef_decoder(brut, &coef);
    phase = ENV_REPOS;
    t_phase = ticks_ms - ENV_PERIODE_MS;        /* première mesure tout de suite */
    return true;
}

/* Appelée toutes les ENV_SCRUTATION_MS par main.c ; ne bloque jamais. */
void env_step(void)
{
    uint8_t etat, d[BME680_N_BRUT];
    uint32_t adc_p, adc_t, adc_h;
    int32_t t_c100;

    switch (phase) {
    case ENV_REPOS:
        if ((ticks_ms - t_phase) >= ENV_PERIODE_MS) {
            ecrire(REG_CTRL_HUM, CTRL_HUM_X1);      /* avant ctrl_meas         */
            ecrire(REG_CTRL_MEAS, CTRL_MEAS_FORCEE);/* lance la mesure forcée  */
            t_phase = ticks_ms;                     /* cadence : 1 par seconde */
            phase = ENV_CONVERSION;
        }
        break;

    case ENV_CONVERSION:
        lire(REG_ETAT_MESURE, &etat, 1u);
        if (etat & NEW_DATA) {
            lire(REG_DONNEES, d, BME680_N_BRUT);
            bme680_brut_decoder(d, &adc_p, &adc_t, &adc_h);
            t_c100 = bme680_temperature_c100(&coef, adc_t);   /* EN PREMIER  */
            derniere.t_c10 = (t_c100 >= 0) ? (t_c100 + 5) / 10 : (t_c100 - 5) / 10;
            derniere.p_hpa_x10 = (bme680_pression_pa(&coef, adc_p) + 5u) / 10u;
            derniere.h_x10 = (bme680_humidite_x1000(&coef, adc_h) + 50u) / 100u;
            derniere.valide = 1u;
            nouvelle = 1u;
            nb_mesures++;
            phase = ENV_REPOS;
        } else if ((ticks_ms - t_phase) > ENV_ATTENTE_MAX_MS) {
            phase = ENV_REPOS;                      /* abandon : essai suivant  */
        }
        break;

    default:                                        /* ENV_ABSENT              */
        break;
    }
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
