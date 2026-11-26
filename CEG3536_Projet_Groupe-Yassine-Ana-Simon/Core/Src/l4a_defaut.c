/* ---------------------------------------------------------------------------
 * l4a_defaut.c — pilote de TouchPad d'une équipe fictive (CEG 3536,
 *                laboratoire 4, composante L4-A) — FOURNI, DÉFECTUEUX
 *
 * Ce pilote compile sans avertissement, mais le TouchPad ne fonctionne pas
 * correctement. La variante est choisie par L4A_VARIANTE (lab4.h ou option
 * -DL4A_VARIANTE=n), attribuée à chaque équipe sur Brightspace.
 *
 * Travail demandé (énoncé, section 4.5) : symptôme reproductible, au moins
 * trois hypothèses classées, mesures, essai discriminant avec prédiction
 * écrite AVANT l'essai, interprétation, correction, conclusion et limites.
 * Lire ce fichier donne des hypothèses, pas une conclusion : la conclusion
 * repose sur des mesures (registres de I2C2, relecture des registres du
 * composant, i2c_scan, drapeau thi_flag, compteurs de la télémétrie).
 * La correction se fait dans ce fichier, dans une validation Git distincte.
 *
 * Les paramètres des quatre variantes diffèrent sur plusieurs points ; tous
 * ne sont pas des défauts.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "i2c.h"
#include "l4a_defaut.h"

#define REG_ETAT        0x00u
#define REG_FILTRES     0x2Bu
#define REG_SEUILS      0x41u
#define REG_REBOND      0x5Bu
#define REG_CONFIG1     0x5Cu
#define REG_CONFIG2     0x5Du
#define REG_SERVICE     0x5Eu
#define REG_RAZ         0x80u
#define N_ELECTRODES    12u

typedef struct {
    uint8_t adresse;        /* adresse du composant passée au pilote I2C       */
    uint8_t seuil_toucher;
    uint8_t seuil_relache;
    uint8_t rebond;         /* registre 0x5B                                   */
    uint8_t courant;        /* registre 0x5C                                   */
    uint8_t service;        /* registre 0x5E                                   */
    uint8_t ordre;          /* ordre des étapes de la configuration            */
    uint8_t rang;           /* rang de l'octet des électrodes 0 à 7 dans la lecture */
} l4a_param_t;

static const l4a_param_t param[4] = {
    /* adresse  toucher relâche rebond courant service ordre rang */
    {  0x5Au,   12u,    6u,     0x11u, 0x10u,  0x80u,  0u,   0u },
    {  0x5Au,   15u,    8u,     0x00u, 0x10u,  0x8Cu,  0u,   1u },
    {  0xB4u,   12u,    6u,     0x11u, 0x20u,  0x8Cu,  0u,   0u },
    {  0x5Au,   10u,    5u,     0x22u, 0x10u,  0x8Cu,  1u,   0u },
};

static const uint8_t filtres[] = {
    0x01u, 0x01u, 0x00u, 0x00u,     /* 0x2B à 0x2E : front montant             */
    0x01u, 0x01u, 0xFFu, 0x02u,     /* 0x2F à 0x32 : front descendant          */
    0x00u, 0x00u, 0x00u             /* 0x33 à 0x35 : électrode touchée         */
};

static bool reg_ecrire(const l4a_param_t *p, uint8_t reg, uint8_t valeur)
{
    uint8_t d[2] = { reg, valeur };

    return i2c_ecrire(p->adresse, d, 2u) == I2C_OK;
}

static bool configurer(const l4a_param_t *p)
{
    bool ok = true;

    for (uint32_t i = 0u; i < sizeof filtres; i++) {
        ok = reg_ecrire(p, (uint8_t)(REG_FILTRES + i), filtres[i]) && ok;
    }
    for (uint32_t i = 0u; i < N_ELECTRODES; i++) {
        ok = reg_ecrire(p, (uint8_t)(REG_SEUILS + 2u * i), p->seuil_toucher) && ok;
        ok = reg_ecrire(p, (uint8_t)(REG_SEUILS + 2u * i + 1u), p->seuil_relache) && ok;
    }
    ok = reg_ecrire(p, REG_REBOND, p->rebond) && ok;
    ok = reg_ecrire(p, REG_CONFIG1, p->courant) && ok;
    ok = reg_ecrire(p, REG_CONFIG2, 0x20u) && ok;
    return ok;
}

bool l4a_tp_init(void)
{
    const l4a_param_t *p = &param[L4A_VARIANTE & 3];
    bool ok;

    ok = reg_ecrire(p, REG_RAZ, 0x63u);
    ok = reg_ecrire(p, REG_SERVICE, 0x00u) && ok;
    if (p->ordre == 0u) {
        ok = configurer(p) && ok;
        ok = reg_ecrire(p, REG_SERVICE, p->service) && ok;
    } else {
        ok = reg_ecrire(p, REG_SERVICE, p->service) && ok;
        ok = configurer(p) && ok;
    }
    return ok;
}

bool l4a_tp_lire(uint16_t *etat12)
{
    const l4a_param_t *p = &param[L4A_VARIANTE & 3];
    uint8_t reg = REG_ETAT;
    uint8_t d[2];

    if (i2c_ecrire_lire(p->adresse, &reg, 1u, d, 2u) != I2C_OK) {
        return false;
    }
    *etat12 = (uint16_t)(((uint32_t)d[p->rang] | ((uint32_t)d[1u - p->rang] << 8)) & 0x0FFFu);
    return true;
}
