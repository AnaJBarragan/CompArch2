/* ---------------------------------------------------------------------------
 * ioexp.c — objet « extenseur d'entrées-sorties » (CEG 3536, laboratoire 4,
 *           E2) — SQUELETTE À COMPLÉTER
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
    /* À COMPLÉTER :
     *   - si sortie_connue et valeur == sortie : retourner true, sans transaction ;
     *   - sinon ecritures++, i2c_ecrire d'un octet ;
     *   - succès : sortie = valeur, sortie_connue = 1, echecs = 0, true ;
     *   - échec : echecs++, false (sortie_connue inchangé : nouvel essai plus tard). */
    (void)x; (void)valeur;
    return false;
}

bool ioexp_lire(ioexp_t *x, uint8_t *valeur)
{
    /* À COMPLÉTER : i2c_lire d'un octet ; succès : entree et *valeur mis à
     * jour, echecs = 0 ; échec : echecs++, false.                             */
    (void)x; (void)valeur;
    return false;
}
