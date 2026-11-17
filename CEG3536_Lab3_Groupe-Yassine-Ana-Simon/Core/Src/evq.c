/* ---------------------------------------------------------------------------
 * evq.c — file circulaire d'événements (CEG 3536, laboratoire 3, E7)
 * Tableau fixe de EVQ_CAPACITE éléments, indice de tête (lecture), indice de
 * queue (écriture), compteur d'éléments. Débordement : push refusé et compté,
 * la file n'est jamais corrompue. Aucun accès matériel : ce fichier est
 * compilé tel quel sur hôte (hote/Makefile) et sur la cible.
 * Vérification : hote/test_evq.c (12 cas) et commande série q sur la cible.
 * ------------------------------------------------------------------------- */
#include "evq.h"

static evq_event_t file[EVQ_CAPACITE];
static uint32_t    tete;          /* prochain élément à lire                   */
static uint32_t    queue;         /* prochaine case à écrire                   */
static uint32_t    nombre;        /* éléments présents                         */
static uint32_t    debordements;  /* push refusés (file pleine)                */
static uint32_t    total;         /* push acceptés depuis evq_init             */

void evq_init(void)
{
    tete = queue = nombre = debordements = total = 0u;
}

/* Ajoute un événement en fin de file.
 * Retour : true si accepté, false si la file est pleine (débordement compté). */
bool evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms)
{
    if (nombre >= EVQ_CAPACITE) {           /* file pleine : refus            */
        debordements++;                     /* compte le débordement          */
        return false;
    }
    file[queue].type   = type;              /* type de l'événement            */
    file[queue].valeur = valeur;            /* valeur associée                */
    file[queue].t_ms   = t_ms;              /* horodatage                     */
    queue = (queue + 1u) % EVQ_CAPACITE;    /* enroulement au bout du tableau */
    nombre++;                               /* un élément de plus             */
    total++;                                /* compte ce push accepté         */
    return true;
}

/* Retire l'événement le plus ancien et le copie dans *e.
 * Retour : false si la file est vide ou si e est nul (rien n'est retiré).   */
bool evq_pop(evq_event_t *e)
{
    if (nombre == 0u || e == 0) {           /* file vide ou pointeur nul      */
        return false;
    }
    *e   = file[tete];                      /* copie du premier élément       */
    tete = (tete + 1u) % EVQ_CAPACITE;      /* indice suivant, enroulement    */
    nombre--;                               /* un élément de moins            */
    return true;
}

uint32_t evq_count(void)        { return nombre; }
uint32_t evq_debordements(void) { return debordements; }
uint32_t evq_total(void)        { return total; }
