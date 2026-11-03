/* ---------------------------------------------------------------------------
 * evq.c — file circulaire d'événements (CEG 3536, laboratoire 3, E7)
 *         — SQUELETTE À COMPLÉTER (jalon J2)
 * Tableau fixe de EVQ_CAPACITE éléments, indice de tête (lecture), indice de
 * queue (écriture), compteur d'éléments. Débordement : push refusé et compté,
 * la file n'est jamais corrompue. Aucun accès matériel : ce fichier est
 * compilé tel quel sur hôte (hote/Makefile) et sur la cible.
 * Vérification : hote/test_evq.c (9 cas) et commande série q sur la cible.
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
   /* file[0].type = EV_AUCUN;   */        /* à retirer une fois push/pop écrites */
}





/* -----------------------E7------------------------*/


/* À COMPLÉTER : si la file est pleine, debordements++ et retour false ;
 * sinon écrire type, valeur, t_ms en file[queue], avancer queue modulo
 * EVQ_CAPACITE (enroulement), nombre++, total++, retour true.              */
bool evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms)
{
	 bool evq_push(evq_type_t type, uint32_t valeur,
	 uint32_t t_ms)
	   {
	       if (nombre >= EVQ_CAPACITE) {       /* queue full = refuse            */
	           debordements++;                 /* compte le débordement             */
	           return false;
	       }

	       file[queue].type   = type;          /* save type                     */
	       file[queue].valeur = valeur;        /*save value                 */
	       file[queue].t_ms   = t_ms;       /*save horodatage*/
	       queue = (queue + 1u) % EVQ_CAPACITE;
	                                           /* enroulement au bout du tableau    */
	       nombre++;                           /* un élément ++             */
	       total++;                            /* compte ce push accepté            */
	       return true;                        /*sucess*/
	   }


	/*
    (void)type; (void)valeur; (void)t_ms;
    debordements++;                    /* squelette : refuse tout             */ /*
    return false;
    */
}






/* À COMPLÉTER : si nombre == 0 ou e == 0, retour false ; sinon copier
 * file[tete] dans *e, avancer tete modulo EVQ_CAPACITE, nombre--, true.     */
bool evq_pop(evq_event_t *e)
{

	 bool evq_pop(evq_event_t *e)
	   {
	       if (nombre == 0u) {                 /* empty queue = nothing        */
	           return false;                   /* fail*/
	       }
	       if (e == 0) {                       /* pointer == null */
	           return false;
	       }

	       *e   = file[tete];                  /* copie premier elem */
	       tete = (tete + 1u) % EVQ_CAPACITE;  /* next index   */

	       nombre--;                           /*elem --            */
	       return true;
	   }
	/*
    (void)e;
    return false;
    */
}

uint32_t evq_count(void)        { return nombre; }
uint32_t evq_debordements(void) { return debordements; }
uint32_t evq_total(void)        { return total; }
