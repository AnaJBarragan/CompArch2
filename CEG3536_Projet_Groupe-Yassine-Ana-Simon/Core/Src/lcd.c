/* ---------------------------------------------------------------------------
 * lcd.c — afficheur 16 x 2 (I2C_ADR_LCD) et rétroéclairage RVB (I2C_ADR_RVB)
 *         (CEG 3536, laboratoire 4, E3) — SQUELETTE À COMPLÉTER
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
static uint32_t    echecs;

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

void lcd_step(void)
{
    /* À COMPLÉTER — une seule transaction par appel :
     *  1. phase != LCD_PRET : dérouler la séquence d'initialisation, une
     *     commande par appel, en respectant les délais (ticks_ms - t_phase) ;
     *  2. sinon, une composante de couleur à envoyer (rvb_connu, rvb_montre) :
     *     l'envoyer à I2C_ADR_RVB ;
     *  3. sinon, chercher le premier caractère où montre != voulu :
     *       - si curseur n'est pas à cette position : envoyer la position
     *         (et la mémoriser dans curseur) ;
     *       - sinon envoyer le caractère, mettre montre à jour, curseur++
     *         (curseur devient inconnu en fin de ligne) ;
     *  4. sur un succès : echecs = 0 ; sur un échec : echecs++, et ne rien
     *     marquer comme envoyé. Quand le module répond de nouveau après une
     *     panne : image_invalider().
     * Indice : mémoriser où la recherche s'est arrêtée évite de reparcourir
     * les 32 caractères à chaque tick.                                         */
    (void)t_phase;
}
