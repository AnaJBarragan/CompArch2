/* ---------------------------------------------------------------------------
 * fsm.c — machine à états du véhicule en C (CEG 3536)
 *         Base : corrigé du laboratoire 3 (à citer). Étendue pour le projet,
 *         partie I (E5, E6, E8, E9, E12) : mode, cause, plafond, drapeaux,
 *         état ARRÊT_ÉVÉNEMENT et priorités de la section 4.6.
 *
 * Principe : les événements de la file modifient des CONDITIONS (PIR haut,
 * feu, obstacle simulé, limitation, défaut, humidité). Après chaque
 * événement, evaluer() calcule la cause d'arrêt la plus prioritaire encore
 * active et applique la règle : en marche, toute cause arrête ; à l'arrêt,
 * seul un défaut impose ARRÊT_ÉVÉNEMENT ; en ARRÊT_ÉVÉNEMENT, une cause plus
 * prioritaire remplace celle qui est affichée. On ne sort de
 * ARRÊT_ÉVÉNEMENT que par Fn (ou Touch En si le TouchPad était le module
 * muet), quand plus aucune cause n'est active : retour en ARRÊT, jamais en
 * marche (aucune reprise automatique).
 *
 * Priorités (section 4.6) : E-Stop > défaut > obstacle > feu > fin >
 * limitation > consigne. Les valeurs CAUSE_x de lab4.h croissent avec la
 * priorité, d'où les comparaisons « c > cause ».
 *
 * Sorties : entree_etat (sens du moteur, bips d'entrée) et fsm_sorties
 * (rampe, DEL de la Nucleo, avertisseur) sont les seuls endroits qui
 * commandent le moteur, les DEL et l'avertisseur. La machine ne connaît ni le
 * bus ni les adresses : main.c traduit le matériel en événements.
 * ------------------------------------------------------------------------- */
#include "fsm.h"
#include "lab4.h"

/* variables déclarées extern volatile dans lab1.h / lab2.h */
volatile uint32_t etat;
volatile uint32_t touch_enabled;
volatile uint32_t compteur_transitions;
volatile uint32_t consigne_pct;
volatile uint32_t pwm_pct;

static uint32_t consigne_pot;       /* dernière position du potentiomètre (%) */
static uint32_t prochain_sens;      /* 0 : prochain départ en avant           */
static uint32_t rampe_compteur, clignote_compteur, clignote_phase, bip_compteur;
static etat_vehicule_t expose;

/* ---- projet, partie I ---- */
static uint32_t mode;               /* MODE_MANUEL ou MODE_AUTONOME            */
static uint32_t cause;              /* CAUSE_x de l'arrêt affiché              */
static uint32_t plafond_pct;        /* 10 à 100 (touches 1 à 9 et 0)           */
static uint32_t pir_haut;           /* signal PIR à 1 (EV_PIR)                 */
static uint32_t feu, obstacle_sim, limitation_sw, defaut_sim;  /* SW1, SW2, SW3, SW5 */
static uint32_t fin;                /* SW4 : fin de trajet signalée            */
static uint32_t hum_haute;          /* humidité au-delà du seuil (hystérésis)  */
static uint32_t modules_muets;      /* modules I2C muets (EV_DEFAUT)           */
static uint32_t defaut_tp;          /* le TouchPad a été muet : Touch En acquitte */

/* tables indexées par l'état (0 à 4) */
static const uint8_t etat_vers_del[5]  = { LED_ROUGE, LED_VERTE, LED_BLEUE, LED_ROUGE, LED_ROUGE };
static const uint8_t etat_vers_dir[5]  = { MOTOR_ARRET, MOTOR_AVANT, MOTOR_ARRIERE,
                                           MOTOR_ARRET, MOTOR_ARRET };
static const uint8_t etat_vers_bips[5] = { 1u, 1u, 2u, 0u, 1u };

static bool en_marche(void)
{
    return etat == ETAT_MARCHE_AVANT || etat == ETAT_MARCHE_ARRIERE;
}

static bool autonome(void)
{
    return mode == MODE_AUTONOME;
}

/* Événements actifs, indépendamment de l'état (DEL de l'extenseur). Les
 * commutateurs ne comptent qu'en mode AUTONOME (E8).                       */
static uint32_t calculer_drapeaux(void)
{
    uint32_t d = 0u;

    if ((autonome() && limitation_sw) || hum_haute)   d |= DRAPEAU_LIMITATION;
    if (touch_enabled)                                d |= DRAPEAU_TOUCH;
    if (fin)                                          d |= DRAPEAU_FIN;
    if (autonome() && feu)                            d |= DRAPEAU_FEU;
    if (pir_haut || (autonome() && obstacle_sim))     d |= DRAPEAU_OBSTACLE;
    if (modules_muets != 0u || (autonome() && defaut_sim)) d |= DRAPEAU_DEFAUT;
    return d;
}

/* Cause d'arrêt la plus prioritaire encore active (CAUSE_AUCUNE sinon). */
static uint32_t cause_active(void)
{
    uint32_t d = calculer_drapeaux();

    if (d & DRAPEAU_DEFAUT)   return CAUSE_DEFAUT;
    if (d & DRAPEAU_OBSTACLE) return CAUSE_OBSTACLE;
    if (d & DRAPEAU_FEU)      return CAUSE_FEU;
    return CAUSE_AUCUNE;
}

/* Consigne appliquée = min(consigne demandée, plafond, limitation) (E5, E8). */
static uint32_t consigne_appliquee(void)
{
    uint32_t c = autonome() ? CONSIGNE_AUTO_PCT : consigne_pot;

    if (c > plafond_pct) {
        c = plafond_pct;
    }
    if ((calculer_drapeaux() & DRAPEAU_LIMITATION) && c > LIMITATION_PCT) {
        c = LIMITATION_PCT;
    }
    return c;
}

static void entree_etat(uint32_t nouvel_etat)
{
    etat = nouvel_etat;
    compteur_transitions++;
    pwm_pct = 0u;                               /* la rampe repart de 0        */
    rampe_compteur = 0u;
    (void)motor_set_dir(etat_vers_dir[nouvel_etat]);  /* ARRET : CCR1 = 0, STB = 0 */
    consigne_pct = en_marche() ? consigne_appliquee() : 0u;
    buzzer_bip(etat_vers_bips[nouvel_etat]);
}

static void entree_evenement(uint32_t c)
{
    cause = c;
    bip_compteur = 0u;
    entree_etat(ETAT_ARRET_EVENEMENT);
}

static void effacer_simules(void)               /* changement de mode          */
{
    feu = obstacle_sim = limitation_sw = defaut_sim = fin = 0u;
}

/* Applique les priorités après chaque événement (voir l'en-tête). */
static void evaluer(void)
{
    uint32_t c = cause_active();

    if (en_marche()) {
        if (c != CAUSE_AUCUNE) {
            entree_evenement(c);
        }
    } else if (etat == ETAT_ARRET) {
        if (c == CAUSE_DEFAUT) {
            entree_evenement(c);
        }
    } else if (etat == ETAT_ARRET_EVENEMENT) {
        if (c > cause) {
            cause = c;                          /* la plus prioritaire s'affiche */
        }
    }
    /* ARRÊT_URGENCE : rien, seul l'acquittement en sort */
}

/* Sortie de ARRÊT_URGENCE ou de ARRÊT_ÉVÉNEMENT, si les conditions le
 * permettent ; retourne true si l'acquittement est accepté.                */
static bool acquitter(void)
{
    if (etat == ETAT_ARRET_URGENCE) {
        if (button_raw(BTN_ESTOP) != 0u) {
            return false;                       /* E-Stop encore enfoncé       */
        }
        buzzer_continu = 0u;
    } else if (etat == ETAT_ARRET_EVENEMENT) {
        uint32_t c = cause_active();
        if (c != CAUSE_AUCUNE) {
            cause = c;                          /* refus : la cause restante s'affiche */
            return false;
        }
        defaut_tp = 0u;
    } else {
        return false;
    }
    cause = CAUSE_AUCUNE;
    entree_etat(ETAT_ARRET);                    /* bip court : touche acceptée */
    return true;
}

void fsm_init(void)
{
    touch_enabled = 0u;
    consigne_pot = 0u;
    prochain_sens = 0u;
    clignote_compteur = clignote_phase = bip_compteur = 0u;
    expose.vitesse_rpm_q16 = 0u;
    expose.sens = SENS_IMMOBILE;
    mode = MODE_MANUEL;
    cause = CAUSE_AUCUNE;
    plafond_pct = 100u;                         /* valeur après réinitialisation (E5) */
    pir_haut = hum_haute = modules_muets = defaut_tp = 0u;
    effacer_simules();
    (void)motor_set_dir(MOTOR_ARRET);
    etat = ETAT_ARRET;
    pwm_pct = 0u;
    consigne_pct = 0u;
    compteur_transitions = 0u;                  /* fsm_init ne compte pas      */
    fsm_sorties();
}

static void traiter_touche(uint32_t k)          /* TouchPad autorisé (main.c)  */
{
    if (k <= 9u) {                              /* vitesse maximale (E5)       */
        plafond_pct = (k == 0u) ? 100u : 10u * k;
        buzzer_bip(1u);
    } else if (k == (uint32_t)TOUCHE_FN) {      /* acquittement (E6, E8, E9)   */
        (void)acquitter();
    } else if (k == (uint32_t)TOUCHE_NEXT) {    /* mode : en ARRÊT seulement   */
        if (etat == ETAT_ARRET) {
            mode = autonome() ? MODE_MANUEL : MODE_AUTONOME;
            effacer_simules();
            cause = CAUSE_AUCUNE;               /* efface aussi « FIN »        */
            buzzer_bip(1u);
        }
    }
}

static void traiter_commutateur(uint32_t sw)
{
    if (!autonome()) {
        return;                                 /* MANUEL : SW1 à SW5 ignorés  */
    }
    switch (sw) {
    case SW_FEU:        feu ^= 1u;           break;
    case SW_OBSTACLE:   obstacle_sim ^= 1u;  break;
    case SW_LIMITATION: limitation_sw ^= 1u; break;
    case SW_DEFAUT:     defaut_sim ^= 1u;    break;
    case SW_FIN:                                /* en marche seulement         */
        if (en_marche()) {
            fin = 1u;
            cause = CAUSE_FIN;
            entree_etat(ETAT_ARRET);            /* sans acquittement, bip court */
        }
        break;
    default:                                    /* SW6, SW7 libres ; SW8 : main.c */
        break;
    }
}

void fsm_traiter(const evq_event_t *e)
{
    switch (e->type) {
    case EV_ESTOP:
        motor_stop_now();                       /* redondant avec l'ISR        */
        pwm_pct = 0u;
        consigne_pct = 0u;
        buzzer_continu = 1u;
        clignote_compteur = 0u;
        clignote_phase = 1u;
        cause = CAUSE_AUCUNE;
        if (etat != ETAT_ARRET_URGENCE) {
            etat = ETAT_ARRET_URGENCE;
            compteur_transitions++;
        }
        break;

    case EV_USER:
        if (etat == ETAT_ARRET) {
            uint32_t c;
            fin = 0u;                           /* « FIN » s'efface au départ  */
            cause = CAUSE_AUCUNE;
            c = cause_active();
            if (c != CAUSE_AUCUNE) {
                entree_evenement(c);            /* départ refusé : on montre pourquoi */
            } else {
                entree_etat(prochain_sens ? ETAT_MARCHE_ARRIERE : ETAT_MARCHE_AVANT);
                prochain_sens ^= 1u;
            }
        } else if (en_marche()) {
            entree_etat(ETAT_ARRET);
        }
        /* ARRÊT_URGENCE, ARRÊT_ÉVÉNEMENT : User ignoré */
        break;

    case EV_TOUCH:                              /* bouton Touch En             */
        if (etat == ETAT_ARRET_URGENCE) {
            (void)acquitter();                  /* secours du laboratoire 3    */
        } else if (etat == ETAT_ARRET_EVENEMENT && cause == CAUSE_DEFAUT && defaut_tp) {
            (void)acquitter();                  /* E12 : TouchPad muet         */
        } else {
            touch_enabled ^= 1u;                /* garde du TouchPad (Y3)      */
            buzzer_bip(1u);
        }
        break;

    case EV_TOUCHE:
        traiter_touche(e->valeur);
        break;

    case EV_COMMUT:
        traiter_commutateur(e->valeur);
        break;

    case EV_PIR:                                /* valeur != 0 : signal haut   */
        pir_haut = (e->valeur != 0u) ? 1u : 0u;
        break;

    case EV_ENV:                                /* valeur = humidité, 0,1 %    */
        if (e->valeur > HUM_SEUIL_PCT * 10u) {
            hum_haute = 1u;
        } else if (e->valeur < (HUM_SEUIL_PCT - HUM_HYSTERESIS_PCT) * 10u) {
            hum_haute = 0u;
        }
        break;

    case EV_DEFAUT:                             /* module I2C muet ou rétabli  */
        if (e->valeur & DEFAUT_RETABLI) {
            if (modules_muets > 0u) {
                modules_muets--;
            }
        } else {
            modules_muets++;
            if (e->valeur & DEFAUT_TP) {
                defaut_tp = 1u;
            }
        }
        break;

    case EV_CONSIGNE:
        consigne_pot = (e->valeur > 100u) ? 100u : e->valeur;
        break;

    case EV_VITESSE:
        expose.vitesse_rpm_q16 = enc_fronts_vers_rpm_q16(e->valeur);  /* E3 */
        expose.sens = sens_mesure;
        break;

    default:
        break;
    }

    evaluer();                                  /* priorités (section 4.6)     */
    if (en_marche()) {
        consigne_pct = consigne_appliquee();    /* la rampe la rejoint         */
    }
}

static void rampe(void)
{
    if (!en_marche()) {
        return;
    }
    if (++rampe_compteur < RAMPE_PAS_MS) {
        return;
    }
    rampe_compteur = 0u;
    if (pwm_pct < consigne_pct) {
        pwm_pct++;
    } else if (pwm_pct > consigne_pct) {
        pwm_pct--;
    } else {
        return;
    }
    (void)motor_set_pwm(pwm_pct);
}

void fsm_sorties(void)
{
    rampe();
    if (etat == ETAT_ARRET_URGENCE) {
        if (++clignote_compteur >= CLIGNOTEMENT_DEMI_MS) {
            clignote_compteur = 0u;
            clignote_phase ^= 1u;
        }
        led_set(clignote_phase ? LED_ROUGE : LED_AUCUNE);
    } else if (etat <= ETAT_ARRET_EVENEMENT) {
        led_set(etat_vers_del[etat]);
    }
    /* obstacle : un bip de 100 ms toutes les 500 ms (E6) */
    if (etat == ETAT_ARRET_EVENEMENT && cause == CAUSE_OBSTACLE) {
        if (bip_compteur == 0u) {
            buzzer_bip(1u);
        }
        if (++bip_compteur >= BIP_OBSTACLE_MS) {
            bip_compteur = 0u;
        }
    }
    buzzer_step();
}

const etat_vehicule_t *fsm_etat(void)
{
    expose.etat = etat;
    expose.consigne_pct = consigne_pct;
    expose.pwm_pct = pwm_pct;
    expose.transitions = compteur_transitions;
    expose.mode = mode;
    expose.cause = cause;
    expose.plafond_pct = plafond_pct;
    expose.drapeaux = calculer_drapeaux();
    return &expose;
}
