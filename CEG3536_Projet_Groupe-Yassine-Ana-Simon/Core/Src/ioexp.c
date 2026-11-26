/* ---------------------------------------------------------------------------
 * ioexp.c — objet « extenseur d'entrées-sorties » (CEG 3536, laboratoire 4,
 *           E2) — complété par l'équipe
 *
 * Rappels (énoncé, 4.7) :
 *   - DEL : bit à 0 = DEL allumée ; écrire 0xFF pour tout éteindre. L'image
 *     logique de lab4.h (DEL_x, bit à 1 = allumée) est donc complémentée par
 *     l'appelant : ioexp_ecrire(&del, (uint8_t)~image).
 *   - Commutateurs : écrire 0xFF une fois pour laisser les broches en entrée,
 *     puis lire un octet ; bit à 0 = commutateur enfoncé. La lecture retire la
 *     demande IOI.
 * Aucune variable globale ou statique ici : tout l'état est dans *x.
 * L'anti-rebond n'est pas dans cet objet (il est dans main.c ou dans un
 * module à part) : ioexp ne connaît ni le temps ni les événements.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "i2c.h"
#include "ioexp.h"

/* [FOURNI] */
void ioexp_init(ioexp_t *x, uint8_t adr7)
{
    x->adr7 = adr7;
    x->sortie = 0xFFu;
    x->entree = 0xFFu;
    x->sortie_connue = 0u;
    x->ecritures = 0u;
    x->echecs = 0u;
}

bool ioexp_ecrire(ioexp_t *x, uint8_t valeur)
{
    uint8_t d = valeur;

    if (x->sortie_connue && valeur == x->sortie) {
        return true;                    /* déjà en place : aucune transaction */
    }
    x->ecritures++;
    if (i2c_ecrire(x->adr7, &d, 1u) != I2C_OK) {
        x->echecs++;                    /* sortie_connue inchangé : à refaire */
        return false;
    }
    x->sortie = valeur;
    x->sortie_connue = 1u;
    x->echecs = 0u;
    return true;
}

bool ioexp_lire(ioexp_t *x, uint8_t *valeur)
{
    uint8_t d;

    if (i2c_lire(x->adr7, &d, 1u) != I2C_OK) {
        x->echecs++;
        return false;                   /* *valeur inchangé                   */
    }
    x->entree = d;
    *valeur = d;
    x->echecs = 0u;
    return true;
}

/* Ajouts de l'équipe : le répartiteur du bus (main.c) doit savoir s'il y a
 * quelque chose à écrire AVANT de donner le tick à cet extenseur, sans lire
 * les champs privés de la structure.                                        */
bool ioexp_differe(const ioexp_t *x, uint8_t valeur)
{
    return !x->sortie_connue || valeur != x->sortie;
}

uint32_t ioexp_echecs(const ioexp_t *x)
{
    return x->echecs;
}
