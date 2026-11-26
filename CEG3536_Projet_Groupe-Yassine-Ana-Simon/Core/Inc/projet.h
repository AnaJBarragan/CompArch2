/* ---------------------------------------------------------------------------
 * projet.h — interface de la partie II du projet final (CEG 3536, automne 2026)
 * Routines assembleur nouvelles (sections critiques), constantes, codes et
 * types communs de la partie II. Inclut lab4.h (et donc lab3.h, lab2.h et
 * lab1.h) : toute la couche des laboratoires 1 à 4 reste appelable.
 * Aucun en-tête CMSIS ici : ce fichier est aussi compilé sur hôte.
 *
 * IMPORTANT : etat_vehicule_t (lab3.h) reçoit cinq champs de plus, ajoutés
 * À LA FIN de la structure, après ceux du laboratoire 4 (voir LISEZMOI.md) :
 *     uint32_t consigne_rpm;   consigne de vitesse en mode AUTONOME (tr/min)
 *     uint32_t segment;        segment courant du trajet (1..n), 0 hors trajet
 *     uint32_t dist_x10;       distance parcourue dans le trajet, 0,1 tour
 *     uint32_t code_defaut;    DEFAUT_x
 *     uint32_t mode_essai;     1 si le mode essai est actif
 * ------------------------------------------------------------------------- */
#ifndef PROJET_H
#define PROJET_H

#include <stdint.h>
#include <stdbool.h>
#include "lab4.h"

/* ---- Routines assembleur de la partie II (crit.s, E13) ------------------ */
/* crit_entrer : retourne l'état courant de PRIMASK (0 ou 1), puis masque les
 * interruptions. crit_sortir : restaure l'état retourné par crit_entrer.
 * Routines feuilles, R0 seulement, imbricables, appelables d'un gestionnaire.
 *     uint32_t p = crit_entrer();
 *     ... quelques instructions sur les indices de la file ...
 *     crit_sortir(p);
 * Sur hôte, hote/crit_hote.c fournit des équivalents C (couche d'adaptation). */
uint32_t crit_entrer(void);
void     crit_sortir(uint32_t etat);

#define CRIT_MAX_CYCLES     200u    /* durée maximale de masquage (50 µs à 4 MHz) */

/* Mesure de la durée de masquage, sans appel de fonction dans la section :
 *     uint32_t p = crit_entrer();
 *     uint32_t t0 = CRIT_CYCLES();
 *     ... section critique ...
 *     CRIT_MESURER(t0);
 *     crit_sortir(p);
 * crit_max_cycles est défini dans crit_test.c (cible) et crit_hote.c (hôte).
 * Le compteur DWT est mis en marche par dwt_init() dans main.c (laboratoire 3). */
extern volatile uint32_t crit_max_cycles;
#if defined(__arm__)
#define CRIT_CYCLES()       (*(volatile uint32_t *)0xE0001004u)     /* DWT->CYCCNT */
#else
#define CRIT_CYCLES()       0u                                      /* hôte        */
#endif
#define CRIT_MESURER(t0)    do { uint32_t d_ = CRIT_CYCLES() - (t0); \
                                 if (d_ > crit_max_cycles) { crit_max_cycles = d_; } } while (0)

/* ---- États et causes ajoutés (suite de lab1.h et de lab4.h) ------------- */
#define ETAT_INIT               5u      /* vérification des ressources (E22)      */
#define ETAT_DEFAUT             6u      /* remplace ARRÊT_ÉVÉNEMENT / CAUSE_DEFAUT */
#define CAUSE_THERMIQUE         5u      /* ARRÊT_ÉVÉNEMENT sur le second seuil    */

/* codes de la colonne <code_defaut> de la télémétrie */
#define DEFAUT_AUCUN            0u
#define DEFAUT_INIT             1u      /* ressource absente au démarrage (E22)   */
#define DEFAUT_I2C              2u      /* module muet (E12)                      */
#define DEFAUT_SENS             3u      /* sens mesuré opposé au sens commandé    */
#define DEFAUT_ENCODEUR         4u      /* encodeur muet                          */
#define DEFAUT_SIMULE           5u      /* SW5                                    */

/* drapeaux ajoutés à ceux de lab4.h (bits 0 à 5) */
#define DRAPEAU_THERMIQUE       (1u << 6)   /* arrêt thermique actif -> R3 clignotante */
#define DRAPEAU_ESSAI           (1u << 7)   /* mode essai actif -> « * » sur le LCD     */
#define DRAPEAU_FILE            (1u << 8)   /* débordement récent -> « FILE » (1 s)     */

/* commutateurs libres de la partie I, page du LCD ajoutée */
#define SW_TRAJET               6u      /* remise à zéro du trajet, à l'arrêt     */
#define SW_ESSAI                7u      /* bascule du mode essai                  */
#define PAGE_TRAJET             3u      /* troisième page (SW8)                   */
#define FILE_MESSAGE_MS         1000u   /* durée du message « FILE » (E14)        */

/* ---- Distance, vitesse et régulation (E16, E17) ------------------------- */
#define FRONTS_PAR_TOUR         748u    /* deux fronts de E_A, arbre de sortie    */
#define VITESSE_MAX_RPM         340u    /* 100 % en mode AUTONOME                 */
#define CONSIGNE_RPM_MIN        80u
#define CONSIGNE_RPM_MAX        300u
#define REGUL_PERIODE_MS        100u    /* fenêtre de mesure et pas du régulateur */
/* tr/min (Q16.16) pour une fenêtre de 100 ms : fronts x 600 / 748, soit
 * k = round(600 x 65536 / 748) = 52 569 (21 028 pour les 250 ms du lab. 3). */
#define RPM_Q16_K_100MS         52569u
/* Choix de l'équipe : la routine assembleur du laboratoire 3 (constante
 * 21 028, fenêtre de 250 ms) est gardée telle quelle, et son résultat est
 * multiplié par 5 / 2 (21 028 x 5 / 2 = 52 570, écart +0,002 %). Le banc de
 * la routine (commande b) reste valable sans modification.                 */
#define RPM_Q16_100MS(fronts)   (enc_fronts_vers_rpm_q16(fronts) * 5u / 2u)
#define REGUL_KP_Q8             26      /* 0,10 point de PWM par tr/min : point de */
#define REGUL_KI_Q8             26      /* départ, à ajuster sur le banc (E17)     */
#define REGUL_TOLERANCE_PCT     10u     /* erreur admise 2 s après un changement   */

/* ---- Surveillances (E18, E19) ------------------------------------------- */
#define SURV_SENS_MS            500u    /* sens opposé pendant plus de 500 ms     */
#define SURV_MUET_MS            1000u   /* aucun front pendant plus de 1 s ...    */
#define SURV_MUET_PWM_PCT       30u     /* ... au-delà de 30 % de commande        */
#define TEMP_LIMITE_C10         400     /* limitation à partir de 40,0 °C         */
#define TEMP_LIMITE_FIN_C10     370     /* levée à 37,0 °C                        */
#define TEMP_ARRET_C10          500     /* arrêt à partir de 50,0 °C              */
#define TEMP_ARRET_FIN_C10      450     /* sortie possible à 45,0 °C              */

/* ---- Mode essai (E20) : valeurs injectées -------------------------------- */
#define ESSAI_TEMP_1_C10        420     /* commande « 1 »                         */
#define ESSAI_TEMP_2_C10        520     /* commande « 2 »                         */
#define ESSAI_TEMP_3_C10        250     /* commande « 3 »                         */
#define ESSAI_HUM_HAUTE_X10     800u    /* commande « h » (bascule)               */
#define ESSAI_HUM_BASSE_X10     400u
#define ESSAI_RAFALE_N          40u     /* commande « e » : événements en rafale  */

/* ---- Trajet (E16) -------------------------------------------------------- */
typedef struct {
    uint16_t tours;             /* longueur du segment, tours de l'arbre de sortie */
    uint16_t consigne_rpm;      /* consigne de vitesse du segment, tr/min          */
} segment_t;

#define TRAJET_REF_N            4u
extern const segment_t TRAJET_REF[TRAJET_REF_N];    /* défini dans trajet.c       */

/* ---- Types d'événements à ajouter À LA FIN de evq_type_t (evq.h) ---------
 *     EV_THERMIQUE,   13 : niveau thermique changé ; valeur = SURV_THERM_x
 *     EV_FIN,         14 : fin du dernier segment du trajet
 *     EV_ESSAI        15 : mode essai ou injection ; valeur = caractère reçu
 * EV_DEFAUT (12) est réutilisé : valeur = adresse I2C (module muet ou rétabli)
 * ou 0x100 + DEFAUT_x pour les défauts de la partie II.                       */
#define EV_DEFAUT_CODE_BASE     0x100u

/* ---- Ajouts de l'équipe (partie II) -------------------------------------- */
/* INIT (E22) : durée de la phase de vérification, puis plages jugées
 * plausibles pour la tension du capteur de température (LMT86 : environ
 * -20 à +55 °C) et identifiant attendu du capteur environnemental.          */
#define INIT_DUREE_MS           1000u
#define INIT_TEMP_MV_MIN        1200u
#define INIT_TEMP_MV_MAX        2400u
#define INIT_ENV_ID             0x61u
#define INIT_N_ADRESSES         5u

/* Trajet parcouru en mode AUTONOME : 0 = TRAJET_REF (démonstration),
 * 1 = TRAJET_EQUIPE (essais de l'équipe, trajet.h).                         */
#ifndef TRAJET_EQUIPE_ESSAI
#define TRAJET_EQUIPE_ESSAI     0
#endif

/* Numéros des types d'événements utilisés par l'assembleur (estop.s,
 * exti_l4.s) : doivent rester égaux à EV_ESTOP et EV_PIR de evq.h. Les
 * mêmes valeurs sont reprises dans registres_lab4.inc (EV_NUM_ESTOP, ...). */
#define EV_NUM_ESTOP            3u
#define EV_NUM_PIR              9u

#endif /* PROJET_H */
