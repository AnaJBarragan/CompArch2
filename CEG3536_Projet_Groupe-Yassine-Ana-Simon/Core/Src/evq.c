/* ---------------------------------------------------------------------------
 * evq.c — file circulaire d'événements (CEG 3536, laboratoire 3) — CORRIGÉ
 * Tableau fixe de EVQ_CAPACITE éléments, indices tête (lecture) et queue
 * (écriture), compteur d'éléments. Débordement : push refusé et compté, la
 * file n'est jamais corrompue (E7).
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

bool evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms)
{
    if (nombre >= EVQ_CAPACITE) {
        debordements++;
        return false;
    }
    file[queue].type   = type;
    file[queue].valeur = valeur;
    file[queue].t_ms   = t_ms;
    queue = (queue + 1u) % EVQ_CAPACITE;   /* enroulement de l'index           */
    nombre++;
    total++;
    return true;
}

bool evq_pop(evq_event_t *e)
{
    if (nombre == 0u || e == 0) {
        return false;
    }
    *e = file[tete];
    tete = (tete + 1u) % EVQ_CAPACITE;
    nombre--;
    return true;
}

uint32_t evq_count(void)        { return nombre; }
uint32_t evq_debordements(void) { return debordements; }
uint32_t evq_total(void)        { return total; }
