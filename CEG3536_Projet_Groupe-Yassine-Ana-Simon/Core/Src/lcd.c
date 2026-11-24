/* ---------------------------------------------------------------------------
 * lcd.c — afficheur 16 x 2 (I2C_ADR_LCD) et rétroéclairage RVB (I2C_ADR_RVB)
 *         (CEG 3536, laboratoire 4, E3) — complété par l'équipe
 *
 * Repères (énoncé, 4.7 ; valeurs à confirmer dans la fiche du AiP31068L) :
 *   - attendre LCD_DEMARRAGE_MS après la mise sous tension ;
 *   - commande : deux octets {0x80, commande} ; caractère : {0x40, caractère} ;
 *   - séquence : 0x28 (deux lignes), 0x0C (afficheur allumé, sans curseur),
 *     0x01 (effacement, puis 2 ms d'attente), 0x06 (mode d'entrée) ;
 *   - position du curseur : 0x80 | (colonne + 0x40 x ligne) ;
 *   - rétroéclairage : {0x01, rouge}, {0x02, vert}, {0x03, bleu} à I2C_ADR_RVB.
 * Contraintes : une transaction par appel de lcd_step, six octets au plus sur
 * le bus (E11), aucune attente bloquante (les délais se comptent en ticks_ms).
 * Après un caractère, le curseur de l'afficheur avance d'une colonne : une
 * nouvelle position n'est nécessaire que si le prochain caractère à écrire
 * n'est pas à la suite du précédent.
 * En cas d'erreur (module absent, E12) : compter l'échec, puis, quand le
 * module répond de nouveau, tout réécrire (image_invalider) — et relancer
 * l'initialisation si l'afficheur a perdu son alimentation.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "i2c.h"
#include "lcd.h"

#define CAR_INCONNU     0xFFu       /* jamais demandé : force la réécriture    */
#define CURSEUR_INCONNU (LCD_LIGNES * LCD_COLONNES)

/* images [FOURNI] */
static uint8_t voulu[LCD_LIGNES][LCD_COLONNES];     /* demandé par lcd_ligne   */
static uint8_t montre[LCD_LIGNES][LCD_COLONNES];    /* connu de l'afficheur    */
static uint8_t rvb_voulu[3], rvb_montre[3];
static uint8_t rvb_connu[3];                        /* 0 = composante à envoyer */

/* séquence d'initialisation : phases à compléter au besoin */
typedef enum {
    LCD_ATTENTE,        /* délai de mise sous tension                          */
    LCD_FONCTION,       /* 0x28                                                */
    LCD_ALLUMAGE,       /* 0x0C                                                */
    LCD_EFFACEMENT,     /* 0x01, puis 2 ms                                     */
    LCD_MODE,           /* 0x06                                                */
    LCD_PRET
} lcd_phase_t;

static lcd_phase_t phase;
static uint32_t    t_phase;         /* ticks_ms à l'entrée dans la phase       */
static uint32_t    curseur;         /* ligne x 16 + colonne, ou CURSEUR_INCONNU */
static uint32_t    echecs;          /* erreurs consécutives du pilote 0x3E    */
static uint32_t    t_essai;         /* ticks_ms du dernier échec               */
static uint32_t    recherche;       /* où reprendre la recherche (0 à 31)      */
static uint32_t    rvb_echecs;      /* erreurs du rétroéclairage (0x2D), à part */

/* [FOURNI] force la réécriture complète (après l'initialisation ou une panne) */
static void image_invalider(void)
{
    for (uint32_t l = 0u; l < LCD_LIGNES; l++) {
        for (uint32_t c = 0u; c < LCD_COLONNES; c++) {
            montre[l][c] = CAR_INCONNU;
        }
    }
    rvb_connu[0] = rvb_connu[1] = rvb_connu[2] = 0u;
    curseur = CURSEUR_INCONNU;
}

/* [FOURNI] */
void lcd_init(void)
{
    for (uint32_t l = 0u; l < LCD_LIGNES; l++) {
        for (uint32_t c = 0u; c < LCD_COLONNES; c++) {
            voulu[l][c] = (uint8_t)' ';
        }
    }
    rvb_voulu[0] = rvb_voulu[1] = rvb_voulu[2] = 0u;
    image_invalider();
    phase = LCD_ATTENTE;
    t_phase = ticks_ms;
    echecs = 0u;
    rvb_echecs = 0u;
    recherche = 0u;
}

/* [FOURNI] */
void lcd_ligne(uint32_t ligne, const char *texte16)
{
    uint32_t c = 0u;

    if (ligne >= LCD_LIGNES) {
        return;
    }
    while (c < LCD_COLONNES && texte16[c] != '\0') {
        voulu[ligne][c] = (uint8_t)texte16[c];
        c++;
    }
    while (c < LCD_COLONNES) {
        voulu[ligne][c++] = (uint8_t)' ';
    }
}

/* [FOURNI] */
void lcd_couleur(uint8_t r, uint8_t v, uint8_t b)
{
    rvb_voulu[0] = r;
    rvb_voulu[1] = v;
    rvb_voulu[2] = b;
}

/* [FOURNI] */
bool lcd_pret(void)
{
    return phase == LCD_PRET;
}

/* [FOURNI] */
bool lcd_a_jour(void)
{
    if (phase != LCD_PRET) {
        return false;
    }
    for (uint32_t i = 0u; i < 3u; i++) {
        if (!rvb_connu[i] || rvb_montre[i] != rvb_voulu[i]) {
            return false;
        }
    }
    for (uint32_t l = 0u; l < LCD_LIGNES; l++) {
        for (uint32_t c = 0u; c < LCD_COLONNES; c++) {
            if (montre[l][c] != voulu[l][c]) {
                return false;
            }
        }
    }
    return true;
}

/* [FOURNI] */
uint32_t lcd_echecs(void)
{
    return echecs;
}

/* Une transaction vers le pilote des caractères : {contrôle, octet}.
 * controle = 0x80 (commande) ou 0x40 (caractère). Tient le compte des échecs
 * consécutifs ; quand le module répond de nouveau après une panne (trois
 * échecs ou plus), toute l'image est marquée à réécrire.                    */
static bool lcd_envoyer(uint8_t controle, uint8_t octet)
{
    uint8_t d[2] = { controle, octet };

    if (i2c_ecrire(I2C_ADR_LCD, d, 2u) != I2C_OK) {
        echecs++;
        t_essai = ticks_ms;
        return false;
    }
    if (echecs >= I2C_ERREURS_DEFAUT) {
        image_invalider();              /* retour après une panne (E12)       */
    }
    echecs = 0u;
    return true;
}

/* Séquence d'initialisation : une commande par appel. */
static void lcd_init_step(void)
{
    switch (phase) {
    case LCD_ATTENTE:                   /* délai de mise sous tension         */
        if ((ticks_ms - t_phase) >= LCD_DEMARRAGE_MS) {
            phase = LCD_FONCTION;
        }
        break;
    case LCD_FONCTION:                  /* 2 lignes, caractères 5 x 8         */
        if (lcd_envoyer(0x80u, 0x28u)) {
            phase = LCD_ALLUMAGE;
        }
        break;
    case LCD_ALLUMAGE:                  /* afficheur allumé, sans curseur     */
        if (lcd_envoyer(0x80u, 0x0Cu)) {
            phase = LCD_EFFACEMENT;
        }
        break;
    case LCD_EFFACEMENT:                /* effacement : ~1,5 ms d'exécution   */
        if (lcd_envoyer(0x80u, 0x01u)) {
            phase = LCD_MODE;
            t_phase = ticks_ms;
        }
        break;
    case LCD_MODE:                      /* curseur +1 après chaque caractère  */
        if ((ticks_ms - t_phase) <= LCD_EFFACEMENT_MS) {
            break;                      /* > 2 ms écoulées (pas de 1 ms)      */
        }
        if (lcd_envoyer(0x80u, 0x06u)) {
            phase = LCD_PRET;
            image_invalider();          /* l'afficheur est vide : tout écrire */
        }
        break;
    default:
        break;
    }
}

void lcd_step(void)
{
    /* module muet : un seul essai par I2C_REESSAI_MS, pour ne pas occuper
     * le bus avec des transactions vouées à l'échec                         */
    if (echecs >= I2C_ERREURS_DEFAUT && (ticks_ms - t_essai) < I2C_REESSAI_MS) {
        return;
    }

    /* 1. initialisation */
    if (phase != LCD_PRET) {
        lcd_init_step();
        return;
    }

    /* 2. rétroéclairage : une composante par appel ({registre, valeur}) */
    for (uint32_t i = 0u; i < 3u; i++) {
        if (!rvb_connu[i] || rvb_montre[i] != rvb_voulu[i]) {
            uint8_t d[2] = { (uint8_t)(i + 1u), rvb_voulu[i] };  /* 0x01 R, 0x02 V, 0x03 B */
            if (i2c_ecrire(I2C_ADR_RVB, d, 2u) == I2C_OK) {
                rvb_montre[i] = rvb_voulu[i];
                rvb_connu[i] = 1u;
                rvb_echecs = 0u;
            } else {
                rvb_echecs++;
            }
            return;
        }
    }

    /* 3. premier caractère différent, à partir de la dernière position */
    for (uint32_t k = 0u; k < LCD_LIGNES * LCD_COLONNES; k++) {
        uint32_t pos = (recherche + k) % (LCD_LIGNES * LCD_COLONNES);
        uint32_t l = pos / LCD_COLONNES, c = pos % LCD_COLONNES;

        if (montre[l][c] == voulu[l][c]) {
            continue;
        }
        recherche = pos;
        if (curseur != pos) {
            /* position : 0x80 | adresse DDRAM (ligne 1 à 0x00, ligne 2 à 0x40) */
            if (lcd_envoyer(0x80u, (uint8_t)(0x80u | (c + 0x40u * l)))) {
                curseur = pos;
            }
        } else if (lcd_envoyer(0x40u, voulu[l][c])) {
            montre[l][c] = voulu[l][c];
            /* le curseur avance seul, mais pas de la colonne 15 à la ligne 2 */
            curseur = (c + 1u < LCD_COLONNES) ? pos + 1u : CURSEUR_INCONNU;
        }
        return;                         /* une seule transaction par appel    */
    }
}
