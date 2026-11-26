/* ---------------------------------------------------------------------------
 * evq.h — file circulaire d'événements (CEG 3536, laboratoire 3, E7)
 * Module indépendant du matériel : aucun en-tête CMSIS, aucun registre.
 * Le même source est compilé sur la cible STM32 et sur hôte (hote/).
 * Projet, partie II : plusieurs producteurs (gestionnaires d'interruption et
 * boucle principale), un seul consommateur (boucle principale). Indices et
 * compteurs protégés par crit_entrer / crit_sortir (crit.s ; crit_hote.c sur
 * hôte). Débordement : le plus ancien événement ordinaire est écrasé ; un
 * EV_ESTOP ou un EV_DEFAUT n'est jamais perdu (voir evq.c).
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
    EV_TEST,        /* 7 : événement de test (saturation volontaire)           */
    /* --- projet final, partie I --- */
    EV_TOUCHE,      /* 8 : touche du TouchPad ; valeur = 0..9, 10 Fn, 11 Next  */
    EV_PIR,         /* 9 : mouvement détecté (pir_flag consommé)               */
    EV_COMMUT,      /* 10 : appui validé sur un commutateur ; valeur = 1..8    */
    EV_ENV,         /* 11 : nouvelle mesure ; valeur = humidité en 0,1 %       */
    EV_DEFAUT,      /* 12 : adresse I2C du module muet ou rétabli ;            */
                    /*      partie II : aussi 0x100 + DEFAUT_x                 */
    /* --- projet final, partie II --- */
    EV_THERMIQUE,   /* 13 : niveau thermique changé ; valeur = SURV_THERM_x    */
    EV_FIN,         /* 14 : fin du dernier segment du trajet                   */
    EV_ESSAI        /* 15 : mode essai ou injection ; valeur = caractère reçu  */
} evq_type_t;

typedef struct {
    evq_type_t type;
    uint32_t   valeur;
    uint32_t   t_ms;    /* horodatage (ticks_ms) fourni par le producteur       */
} evq_event_t;

void     evq_init(void);
bool     evq_push(evq_type_t type, uint32_t valeur, uint32_t t_ms); /* false si débordement */
bool     evq_pop(evq_event_t *e);                                   /* false si vide   */
uint32_t evq_count(void);                                           /* éléments présents */
uint32_t evq_debordements(void);                                    /* événements perdus */
uint32_t evq_max(void);                                             /* occupation maximale (E14) */
uint32_t evq_total(void);                                           /* push acceptés   */

#endif /* EVQ_H */
