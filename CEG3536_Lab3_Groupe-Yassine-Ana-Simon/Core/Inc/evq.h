/* ---------------------------------------------------------------------------
 * evq.h — file circulaire d'événements (CEG 3536, laboratoire 3, E7)
 * Module indépendant du matériel : aucun en-tête CMSIS, aucun registre.
 * Le même source est compilé sur la cible STM32 et sur hôte (hote/).
 * Un seul producteur et un seul consommateur sont supposés s'exécuter dans le
 * même contexte (boucle principale) : aucune section critique (laboratoire 5).
 * ------------------------------------------------------------------------- */
#ifndef EVQ_H
#define EVQ_H

#include <stdint.h>
#include <stdbool.h>

#ifndef EVQ_CAPACITE
#define EVQ_CAPACITE 32u
#endif

typedef enum {
    EV_AUCUN = 0,
    EV_USER,        /* appui validé sur User                                   */
    EV_TOUCH,       /* appui validé sur Touch En                               */
    EV_ESTOP,       /* drapeau E-Stop consommé                                 */
    EV_VITESSE,     /* fin de fenêtre de l'encodeur ; valeur = fronts          */
    EV_CONSIGNE,    /* nouvelle position du potentiomètre ; valeur = %         */
    EV_TELEM,       /* période de télémétrie écoulée                           */
    EV_TEST         /* événement de test (saturation volontaire, E12)          */
} evq_type_t;

typedef struct {
    evq_type_t type;
    uint32_t   valeur;
    uint32_t   t_ms;    /* horodatage (ticks_ms) fourni par le producteur       */
} evq_event_t;

void     evq_init(void);
bool     evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms); /* false si pleine */
bool     evq_pop(evq_event_t *e);                                   /* false si vide   */
uint32_t evq_count(void);                                           /* éléments présents */
uint32_t evq_debordements(void);                                    /* push refusés    */
uint32_t evq_total(void);                                           /* push acceptés   */

#endif /* EVQ_H */
