/* ---------------------------------------------------------------------------
 * evq.c — file circulaire d'événements (CEG 3536)
 *         Base : corrigé du laboratoire 3 (à citer). Étendue pour le projet,
 *         partie II (E13, E14, composante L5-A).
 *
 * STRUCTURE
 *   file[32]   tampon circulaire principal ; tete = prochain à lire,
 *              queue = prochaine case libre, nombre = éléments présents.
 *   reserve[8] petit tampon circulaire où l'on SAUVE un événement prioritaire
 *              (EV_ESTOP, EV_DEFAUT) qu'un débordement allait écraser.
 *   Invariants (hors section critique) :
 *     queue == (tete + nombre) mod 32         0 <= nombre   <= 32
 *     r_queue == (r_tete + r_nombre) mod 8    0 <= r_nombre <= 8
 *     tout élément de reserve est PLUS ANCIEN que tout élément de file.
 *   « vide » : nombre == 0 et r_nombre == 0 ; « pleine » : nombre == 32.
 *   (Le compteur lève l'ambiguïté tete == queue entre vide et pleine.)
 *
 * POLITIQUE DE DÉBORDEMENT (décision d'équipe, JOURNAL.md) : ÉCRASER LE PLUS
 * ANCIEN. File pleine : l'élément de tête est retiré pour faire place au
 * nouveau ; s'il est ordinaire il est perdu (debordements++), s'il est
 * prioritaire il est d'abord copié dans la réserve, donc jamais perdu.
 * evq_pop lit la réserve avant la file : comme la réserve ne contient que des
 * éléments venus de la tête, l'ordre d'arrivée des événements conservés est
 * respecté. Le cas « réserve pleine ET tête prioritaire » demande plus de 8
 * événements prioritaires en attente dans une file pleine (la boucle
 * principale vide la file à chaque tick) : il est traité par le refus du
 * nouvel événement, compté lui aussi dans debordements.
 *
 * CONCURRENCE (E13) : producteurs = gestionnaires EXTI2 (E-Stop), EXTI9 (PIR),
 * EXTI15 (rafale « e ») et boucle principale ; consommateur unique = boucle
 * principale. Toute lecture ou écriture des indices, des compteurs et des
 * cases se fait entre crit_entrer et crit_sortir. La section ne contient ni
 * appel de fonction, ni boucle, ni accès périphérique (sauf la lecture du
 * compteur DWT qui la mesure) : coût constant, O(1).
 * ------------------------------------------------------------------------- */
#include "evq.h"
#include "projet.h"

#define EVQ_MASQUE      (EVQ_CAPACITE - 1u)     /* indice suivant = (i + 1) & 31 */
#define EVQ_RESERVE     8u
#define RES_MASQUE      (EVQ_RESERVE - 1u)

_Static_assert((EVQ_CAPACITE & (EVQ_CAPACITE - 1u)) == 0u, "EVQ_CAPACITE : puissance de deux");
_Static_assert((EVQ_RESERVE & (EVQ_RESERVE - 1u)) == 0u, "EVQ_RESERVE : puissance de deux");
/* estop.s et exti_l4.s poussent des numéros de type écrits en dur */
_Static_assert(EV_ESTOP == EV_NUM_ESTOP && EV_PIR == EV_NUM_PIR, "registres_lab4.inc : EV_NUM_x");

/* macro et non fonction : aucun appel dans la section critique */
#define PRIORITAIRE(t)  ((t) == EV_ESTOP || (t) == EV_DEFAUT)

static evq_event_t file[EVQ_CAPACITE];
static evq_event_t reserve[EVQ_RESERVE];
static uint32_t    tete, queue, nombre;         /* file principale            */
static uint32_t    r_tete, r_queue, r_nombre;   /* réserve des prioritaires   */
static uint32_t    debordements;    /* événements perdus (écrasés ou refusés)  */
static uint32_t    total;           /* événements acceptés depuis evq_init     */
static uint32_t    maximum;         /* occupation maximale (file + réserve)    */

void evq_init(void)
{
    uint32_t p = crit_entrer();
    tete = queue = nombre = 0u;
    r_tete = r_queue = r_nombre = 0u;
    debordements = total = maximum = 0u;
    crit_sortir(p);
}

/* Retourne true si l'événement est rangé sans qu'aucun autre ne soit perdu ;
 * false s'il y a eu débordement (le plus ancien ordinaire a été écrasé, ou,
 * cas limite, le nouvel événement a été refusé). Appelable d'un gestionnaire. */
bool evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms)
{
    bool sans_perte = true;
    bool range = true;
    uint32_t p  = crit_entrer();
    uint32_t t0 = CRIT_CYCLES();

    if (nombre == EVQ_CAPACITE) {                   /* pleine                 */
        if (!PRIORITAIRE(file[tete].type)) {
            debordements++;                         /* le plus ancien est perdu */
            sans_perte = false;
            tete = (tete + 1u) & EVQ_MASQUE;
            nombre--;
        } else if (r_nombre < EVQ_RESERVE) {
            reserve[r_queue] = file[tete];          /* sauvé dans la réserve  */
            r_queue = (r_queue + 1u) & RES_MASQUE;
            r_nombre++;
            tete = (tete + 1u) & EVQ_MASQUE;
            nombre--;
        } else {
            debordements++;                         /* cas limite : refus     */
            sans_perte = false;
            range = false;
        }
    }
    if (range) {
        file[queue].type   = type;
        file[queue].valeur = valeur;
        file[queue].t_ms   = t_ms;
        queue = (queue + 1u) & EVQ_MASQUE;
        nombre++;
        total++;
        if (nombre + r_nombre > maximum) {
            maximum = nombre + r_nombre;
        }
    }

    CRIT_MESURER(t0);
    crit_sortir(p);
    return sans_perte;
}

bool evq_pop(evq_event_t *e)
{
    bool lu = false;
    uint32_t p, t0;

    if (e == 0) {
        return false;                       /* aucune donnée partagée lue ici */
    }
    p  = crit_entrer();
    t0 = CRIT_CYCLES();
    if (r_nombre != 0u) {                           /* les plus anciens d'abord */
        *e = reserve[r_tete];
        r_tete = (r_tete + 1u) & RES_MASQUE;
        r_nombre--;
        lu = true;
    } else if (nombre != 0u) {
        *e = file[tete];
        tete = (tete + 1u) & EVQ_MASQUE;
        nombre--;
        lu = true;
    }
    CRIT_MESURER(t0);
    crit_sortir(p);
    return lu;
}

/* Lectures : protégées elles aussi (énoncé E13 : « toute lecture ») ; la
 * somme nombre + r_nombre doit être cohérente.                              */
uint32_t evq_count(void)
{
    uint32_t p = crit_entrer();
    uint32_t n = nombre + r_nombre;
    crit_sortir(p);
    return n;
}

uint32_t evq_debordements(void)
{
    uint32_t p = crit_entrer();
    uint32_t n = debordements;
    crit_sortir(p);
    return n;
}

uint32_t evq_total(void)
{
    uint32_t p = crit_entrer();
    uint32_t n = total;
    crit_sortir(p);
    return n;
}

uint32_t evq_max(void)
{
    uint32_t p = crit_entrer();
    uint32_t n = maximum;
    crit_sortir(p);
    return n;
}
