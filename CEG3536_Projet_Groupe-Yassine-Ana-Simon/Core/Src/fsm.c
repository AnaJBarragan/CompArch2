/* ---------------------------------------------------------------------------
 * fsm.c — machine à états du véhicule (CEG 3536)
 *         Base : corrigé du laboratoire 3 (à citer), étendu à la partie I.
 *         Partie II (E15, L5-B) : la fonction de transition écrite à la main
 *         est REMPLACÉE par automate_transition, générée par outils/automate.py
 *         à partir de hote/automate/automate.json (jamais modifiée à la main).
 *
 * fsm.c garde trois responsabilités, et elles seules (outils/LISEZMOI.md) :
 *   1. TRADUIRE chaque événement de la file, avec ses gardes, en une entrée
 *      du modèle ; mémoriser les CONDITIONS durables (PIR haut, feu, obstacle
 *      simulé, défaut actif, arrêt thermique) ;
 *   2. APPELER automate_transition avec l'entrée la plus prioritaire parmi
 *      l'entrée de l'événement et les conditions actives (automate_choisir :
 *      ordre de la liste « entrees » = priorités de la section 4.6) ;
 *   3. APPLIQUER les sorties de l'état obtenu en un seul endroit : entrer()
 *      à chaque transition, fsm_sorties() à chaque tick.
 *
 * Correspondance modèle -> colonne <etat> de la télémétrie :
 *   INIT -> 5 ; PRET -> 0 (ARRÊT) ; MANUEL et AUTONOME -> 1 ou 2 selon le sens
 *   choisi au départ ; ARRET_EVENEMENT -> 4 ; ARRET_URGENCE -> 3 ; DEFAUT -> 6.
 * Variables bornées hors du modèle : mode (choisi en PRÊT par Next/Back),
 * sens (alterné à chaque départ, comme au laboratoire 3), cause affichée.
 *
 * Gardes (« cause disparue ») : l'entrée ACQUIT n'est produite que si la
 * sortie de l'état courant est permise — E-Stop relâché (ARRÊT_URGENCE),
 * aucune cause active (ARRÊT_ÉVÉNEMENT), aucun défaut actif (DÉFAUT). Un Fn
 * refusé ne produit donc aucune entrée. Après chaque transition, un second
 * pas avec les seules conditions (stabilisation) réapplique aussitôt une
 * cause encore active : ARRÊT_URGENCE acquitté avec un défaut présent mène à
 * PRÊT puis, dans le même appel, à DÉFAUT.
 * ------------------------------------------------------------------------- */
#include "fsm.h"
#include "projet.h"
#include "surv.h"           /* SURV_THERM_x (valeur de EV_THERMIQUE)          */
#include "essai.h"          /* essai_actif : colonne mode_essai               */
#include "automate_gen.h"   /* code GÉNÉRÉ : ne pas modifier                  */

#define BIT(x)  (1u << (uint32_t)(x))

/* variables déclarées extern volatile dans lab1.h / lab2.h */
volatile uint32_t etat;
volatile uint32_t touch_enabled;
volatile uint32_t compteur_transitions;
volatile uint32_t consigne_pct;     /* commande visée, rejointe par la rampe   */
volatile uint32_t pwm_pct;

static auto_etat_t    modele;       /* état de l'automate généré               */
static auto_sorties_t sorties;      /* sorties de cet état (Moore)             */
static uint32_t consigne_pot;       /* dernière position du potentiomètre (%)  */
static uint32_t prochain_sens;      /* 0 : prochain départ en avant            */
static uint32_t sens_marche;        /* ETAT_MARCHE_AVANT ou ETAT_MARCHE_ARRIERE */
static uint32_t rampe_compteur, clignote_compteur, clignote_phase, bip_compteur;
static etat_vehicule_t expose;

/* ---- variables bornées et conditions (parties I et II) ---- */
static uint32_t mode;               /* MODE_MANUEL ou MODE_AUTONOME            */
static uint32_t cause;              /* CAUSE_x affichée                        */
static uint32_t plafond_pct;        /* 10 à 100 (touches 1 à 9 et 0)           */
static uint32_t pir_haut;           /* signal PIR à 1 (EV_PIR)                 */
static uint32_t feu, obstacle_sim, limitation_sw, defaut_sim;  /* SW1, SW2, SW3, SW5 */
static uint32_t fin;                /* fin de trajet signalée (Y4, « FIN »)    */
static uint32_t hum_haute;          /* humidité au-delà du seuil (hystérésis)  */
static uint32_t modules_muets;      /* modules I2C muets (EV_DEFAUT, partie I) */
static uint32_t defaut_tp;          /* le TouchPad a été muet : Touch En acquitte */
static uint32_t surv_actif;         /* défaut de sens ou d'encodeur présent    */
static uint32_t surv_code_recu;     /* DEFAUT_SENS ou DEFAUT_ENCODEUR          */
static uint32_t therm;              /* SURV_THERM_x (EV_THERMIQUE)             */
static uint32_t code_init;          /* DEFAUT_INIT si INIT a échoué            */
static uint32_t code_defaut;        /* motif affiché en DÉFAUT                 */
static uint32_t auto_pwm, auto_rpm, auto_segment, auto_dist;   /* fsm_auto   */

/* bips à l'entrée, indexés par la colonne <etat> (0 ARRÊT .. 6 DÉFAUT) */
static const uint8_t etat_vers_bips[7] = { 1u, 1u, 2u, 0u, 1u, 0u, 1u };

static bool autonome(void)
{
    return mode == MODE_AUTONOME;
}

bool fsm_en_marche(void)
{
    return modele == AUTO_ETAT_MANUEL || modele == AUTO_ETAT_AUTONOME;
}

static bool defaut_actif(void)
{
    return modules_muets != 0u || (autonome() && defaut_sim) || surv_actif;
}

static bool limitation(void)
{
    return (autonome() && limitation_sw) || hum_haute || therm != SURV_THERM_NORMAL;
}

/* Conditions durables, traduites en entrées du modèle (bit i = entrée i) */
static uint32_t conditions(void)
{
    uint32_t m = 0u;

    if (defaut_actif())                           m |= BIT(AUTO_ENTREE_DEFAUT);
    if (therm == SURV_THERM_ARRET)                m |= BIT(AUTO_ENTREE_THERMIQUE);
    if (pir_haut || (autonome() && obstacle_sim)) m |= BIT(AUTO_ENTREE_OBSTACLE);
    if (autonome() && feu)                        m |= BIT(AUTO_ENTREE_FEU);
    return m;
}

/* Cause d'arrêt sur événement la plus prioritaire encore active */
static uint32_t cause_active(void)
{
    if (therm == SURV_THERM_ARRET)                return CAUSE_THERMIQUE;
    if (pir_haut || (autonome() && obstacle_sim)) return CAUSE_OBSTACLE;
    if (autonome() && feu)                        return CAUSE_FEU;
    return CAUSE_AUCUNE;
}

/* Motif du défaut présent (colonne <code_defaut>) */
static uint32_t code_courant(void)
{
    if (surv_actif)                  return surv_code_recu;
    if (modules_muets != 0u)         return DEFAUT_I2C;
    if (autonome() && defaut_sim)    return DEFAUT_SIMULE;
    return code_init;
}

/* Garde de l'entrée ACQUIT : la sortie de l'état courant est-elle permise ? */
static bool acquit_possible(void)
{
    switch (modele) {
    case AUTO_ETAT_ARRET_URGENCE:   return button_raw(BTN_ESTOP) == 0u;
    case AUTO_ETAT_ARRET_EVENEMENT: return conditions() == 0u;
    case AUTO_ETAT_DEFAUT:          return !defaut_actif();
    default:                        return false;
    }
}

uint32_t fsm_plafond_pct(void)
{
    uint32_t p = plafond_pct;

    if (limitation() && p > LIMITATION_PCT) {
        p = LIMITATION_PCT;
    }
    return p;
}

/* Commande PWM visée (rejointe par la rampe) : 0 si le moteur n'est pas
 * autorisé ; régulateur en AUTONOME ; potentiomètre plafonné en MANUEL.     */
static uint32_t commande_visee(void)
{
    uint32_t c, p;

    if (!sorties.moteur) {
        return 0u;
    }
    if (sorties.regulation) {
        return auto_pwm;                    /* déjà plafonné par main.c        */
    }
    c = consigne_pot;
    p = fsm_plafond_pct();
    return (c > p) ? p : c;
}

static uint32_t code_etat(void)
{
    switch (modele) {
    case AUTO_ETAT_INIT:            return ETAT_INIT;
    case AUTO_ETAT_MANUEL:
    case AUTO_ETAT_AUTONOME:        return sens_marche;
    case AUTO_ETAT_ARRET_EVENEMENT: return ETAT_ARRET_EVENEMENT;
    case AUTO_ETAT_ARRET_URGENCE:   return ETAT_ARRET_URGENCE;
    case AUTO_ETAT_DEFAUT:          return ETAT_DEFAUT;
    default:                        return ETAT_ARRET;      /* PRET */
    }
}

/* ---- 3. application des sorties à l'entrée d'un état ------------------- */
static void entrer(auto_etat_t suivant, auto_entree_t x)
{
    auto_etat_t ancien = modele;

    modele = suivant;
    compteur_transitions++;
    pwm_pct = 0u;                               /* la rampe repart de 0        */
    rampe_compteur = 0u;

    if (ancien == AUTO_ETAT_ARRET_URGENCE) {
        buzzer_continu = 0u;                    /* urgence acquittée           */
    }
    if (ancien == AUTO_ETAT_DEFAUT) {
        code_defaut = DEFAUT_AUCUN;             /* défaut acquitté             */
        code_init = DEFAUT_AUCUN;
        defaut_tp = 0u;
    }

    if (sorties.moteur) {                       /* MANUEL ou AUTONOME, depuis PRÊT */
        sens_marche = prochain_sens ? ETAT_MARCHE_ARRIERE : ETAT_MARCHE_AVANT;
        prochain_sens ^= 1u;
        fin = 0u;                               /* « FIN » s'efface au départ  */
        cause = CAUSE_AUCUNE;
        (void)motor_set_dir(sens_marche == ETAT_MARCHE_AVANT ? MOTOR_AVANT : MOTOR_ARRIERE);
    } else {
        (void)motor_set_dir(MOTOR_ARRET);       /* CCR1 = 0, STB = 0           */
    }

    switch (suivant) {
    case AUTO_ETAT_PRET:
        cause = CAUSE_AUCUNE;
        if (x == AUTO_ENTREE_FIN) {             /* fin de trajet (E16)         */
            fin = 1u;
            cause = CAUSE_FIN;
        }
        break;
    case AUTO_ETAT_ARRET_EVENEMENT:
        cause = cause_active();
        bip_compteur = 0u;
        break;
    case AUTO_ETAT_ARRET_URGENCE:
        motor_stop_now();                       /* redondant avec l'ISR        */
        buzzer_continu = 1u;
        clignote_compteur = 0u;
        clignote_phase = 1u;
        cause = CAUSE_AUCUNE;
        break;
    case AUTO_ETAT_DEFAUT:
        cause = CAUSE_DEFAUT;
        code_defaut = code_courant();
        break;
    default:
        break;
    }
    etat = code_etat();
    consigne_pct = commande_visee();
    buzzer_bip(etat_vers_bips[etat]);
}

/* ---- 2. un pas de l'automate généré ------------------------------------- */
static void pas(uint32_t masque)
{
    auto_entree_t  x = automate_choisir(masque);    /* la plus prioritaire */
    auto_sorties_t s;
    auto_etat_t    suivant;

    if (x == AUTO_N_ENTREES) {
        return;                                     /* aucune entrée       */
    }
    suivant = automate_transition(modele, x, &s);
    if (suivant != modele) {
        sorties = s;
        entrer(suivant, x);
    }
}

void fsm_init(void)
{
    etat_vehicule_t zero = { 0u };

    expose = zero;
    expose.sens = SENS_IMMOBILE;
    touch_enabled = 0u;
    consigne_pot = 0u;
    prochain_sens = 0u;
    sens_marche = ETAT_MARCHE_AVANT;
    clignote_compteur = clignote_phase = bip_compteur = rampe_compteur = 0u;
    mode = MODE_MANUEL;
    cause = CAUSE_AUCUNE;
    plafond_pct = 100u;                         /* valeur après réinitialisation (E5) */
    pir_haut = feu = obstacle_sim = limitation_sw = defaut_sim = fin = hum_haute = 0u;
    modules_muets = defaut_tp = surv_actif = surv_code_recu = 0u;
    therm = SURV_THERM_NORMAL;
    code_init = code_defaut = DEFAUT_AUCUN;
    auto_pwm = auto_rpm = auto_segment = auto_dist = 0u;

    modele = AUTO_ETAT_INITIAL;                 /* INIT (E22)                  */
    automate_sorties(modele, &sorties);
    (void)motor_set_dir(MOTOR_ARRET);
    etat = code_etat();
    pwm_pct = 0u;
    consigne_pct = 0u;
    compteur_transitions = 0u;                  /* fsm_init ne compte pas      */
    fsm_sorties();
}

void fsm_fin_init(uint32_t code)
{
    if (code != DEFAUT_AUCUN) {
        code_init = code;
        pas(conditions() | BIT(AUTO_ENTREE_DEFAUT));
    } else {
        pas(conditions() | BIT(AUTO_ENTREE_INIT_OK));
    }
    pas(conditions());                          /* stabilisation               */
}

void fsm_auto(uint32_t pwm, uint32_t consigne_rpm, uint32_t segment, uint32_t dist_x10)
{
    auto_pwm = pwm;
    auto_rpm = consigne_rpm;
    auto_segment = segment;
    auto_dist = dist_x10;
    if (fsm_en_marche() && sorties.regulation) {
        consigne_pct = auto_pwm;                /* la rampe la rejoint         */
    }
}

/* ---- 1. traduction des événements --------------------------------------- */
static void effacer_simules(void)               /* changement de mode          */
{
    feu = obstacle_sim = limitation_sw = defaut_sim = fin = 0u;
}

static auto_entree_t traiter_touche(uint32_t k) /* TouchPad autorisé (main.c)  */
{
    if (k <= 9u) {                              /* vitesse maximale (E5)       */
        plafond_pct = (k == 0u) ? 100u : 10u * k;
        buzzer_bip(1u);
    } else if (k == (uint32_t)TOUCHE_FN) {      /* acquittement                */
        if (acquit_possible()) {
            return AUTO_ENTREE_ACQUIT;
        }
        if (modele == AUTO_ETAT_ARRET_EVENEMENT && cause_active() != CAUSE_AUCUNE) {
            cause = cause_active();             /* refus : la cause restante s'affiche */
        }
    } else if (k == (uint32_t)TOUCHE_NEXT) {    /* mode : en PRÊT seulement    */
        if (modele == AUTO_ETAT_PRET) {
            mode = autonome() ? MODE_MANUEL : MODE_AUTONOME;
            effacer_simules();
            cause = CAUSE_AUCUNE;               /* efface aussi « FIN »        */
            buzzer_bip(1u);
        }
    }
    return AUTO_N_ENTREES;
}

static auto_entree_t traiter_commutateur(uint32_t sw)
{
    if (!autonome()) {
        return AUTO_N_ENTREES;                  /* MANUEL : SW1 à SW5 ignorés  */
    }
    switch (sw) {
    case SW_FEU:        feu ^= 1u;           break;
    case SW_OBSTACLE:   obstacle_sim ^= 1u;  break;
    case SW_LIMITATION: limitation_sw ^= 1u; break;
    case SW_DEFAUT:     defaut_sim ^= 1u;    break;
    case SW_FIN:        return AUTO_ENTREE_FIN;     /* sans effet hors AUTONOME */
    default:            break;                  /* SW6, SW7, SW8 : main.c      */
    }
    return AUTO_N_ENTREES;
}

/* EV_DEFAUT : partie I, adresse du module (| DEFAUT_TP) muet ou rétabli ;
 * partie II, EV_DEFAUT_CODE_BASE + DEFAUT_SENS ou DEFAUT_ENCODEUR, avec
 * DEFAUT_RETABLI quand le défaut a disparu (main.c).                         */
static void traiter_defaut(uint32_t v)
{
    if (v & EV_DEFAUT_CODE_BASE) {
        uint32_t c = v & 0xFFu;
        if (c == DEFAUT_SENS || c == DEFAUT_ENCODEUR) {
            if (v & DEFAUT_RETABLI) {
                surv_actif = 0u;
            } else {
                surv_actif = 1u;
                surv_code_recu = c;
            }
        }
    } else if (v & DEFAUT_RETABLI) {
        if (modules_muets > 0u) {
            modules_muets--;
        }
    } else {
        modules_muets++;
        if (v & DEFAUT_TP) {
            defaut_tp = 1u;
        }
    }
}

void fsm_traiter(const evq_event_t *e)
{
    auto_entree_t entree = AUTO_N_ENTREES;

    switch (e->type) {
    case EV_ESTOP:
        entree = AUTO_ENTREE_ESTOP;
        break;
    case EV_USER:                               /* traduit selon le mode       */
        entree = autonome() ? AUTO_ENTREE_USER_AUT : AUTO_ENTREE_USER_MAN;
        break;
    case EV_TOUCH:                              /* bouton Touch En             */
        if (modele == AUTO_ETAT_ARRET_URGENCE
            || (modele == AUTO_ETAT_DEFAUT && defaut_tp)) {
            if (acquit_possible()) {            /* secours (lab. 3) ; E12      */
                entree = AUTO_ENTREE_ACQUIT;
            }
        } else {
            touch_enabled ^= 1u;                /* garde du TouchPad (Y3)      */
            buzzer_bip(1u);
        }
        break;
    case EV_TOUCHE:
        entree = traiter_touche(e->valeur);
        break;
    case EV_COMMUT:
        entree = traiter_commutateur(e->valeur);
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
    case EV_DEFAUT:
        traiter_defaut(e->valeur);
        break;
    case EV_THERMIQUE:                          /* valeur = SURV_THERM_x       */
        therm = (e->valeur <= SURV_THERM_ARRET) ? e->valeur : SURV_THERM_ARRET;
        break;
    case EV_FIN:                                /* trajet_fini (main.c)        */
        entree = AUTO_ENTREE_FIN;
        break;
    case EV_CONSIGNE:
        consigne_pot = (e->valeur > 100u) ? 100u : e->valeur;
        break;
    case EV_VITESSE:                            /* fenêtre de 100 ms (E17)     */
        expose.vitesse_rpm_q16 = RPM_Q16_100MS(e->valeur);
        expose.sens = sens_mesure;
        break;
    default:                                    /* EV_ESSAI, EV_TEST, ...      */
        break;
    }

    pas(conditions() | ((entree < AUTO_N_ENTREES) ? BIT(entree) : 0u));
    pas(conditions());                          /* stabilisation               */

    if (modele == AUTO_ETAT_ARRET_EVENEMENT && cause_active() > cause) {
        cause = cause_active();                 /* la plus prioritaire s'affiche */
    }
    if (fsm_en_marche()) {
        consigne_pct = commande_visee();        /* la rampe la rejoint         */
    }
}

static void rampe(void)
{
    if (!fsm_en_marche()) {
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
    switch (sorties.voyant) {                   /* DEL de la Nucleo            */
    case 1u:                                    /* couleur du sens             */
        led_set(sens_marche == ETAT_MARCHE_AVANT ? LED_VERTE : LED_BLEUE);
        break;
    case 2u:                                    /* rouge clignotante (urgence) */
        if (++clignote_compteur >= CLIGNOTEMENT_DEMI_MS) {
            clignote_compteur = 0u;
            clignote_phase ^= 1u;
        }
        led_set(clignote_phase ? LED_ROUGE : LED_AUCUNE);
        break;
    default:                                    /* rouge fixe                  */
        led_set(LED_ROUGE);
        break;
    }
    /* obstacle ou arrêt thermique : un bip de 100 ms toutes les 500 ms */
    if (modele == AUTO_ETAT_ARRET_EVENEMENT
        && (cause == CAUSE_OBSTACLE || cause == CAUSE_THERMIQUE)) {
        if (bip_compteur == 0u) {
            buzzer_bip(1u);
        }
        if (++bip_compteur >= BIP_OBSTACLE_MS) {
            bip_compteur = 0u;
        }
    }
    buzzer_step();
}

static uint32_t calculer_drapeaux(void)
{
    uint32_t d = 0u;

    if (limitation())                                d |= DRAPEAU_LIMITATION;
    if (touch_enabled)                               d |= DRAPEAU_TOUCH;
    if (fin)                                         d |= DRAPEAU_FIN;
    if (autonome() && feu)                           d |= DRAPEAU_FEU;
    if (pir_haut || (autonome() && obstacle_sim))    d |= DRAPEAU_OBSTACLE;
    if (defaut_actif() || modele == AUTO_ETAT_DEFAUT) d |= DRAPEAU_DEFAUT;
    if (therm == SURV_THERM_ARRET)                   d |= DRAPEAU_THERMIQUE;
    if (essai_actif())                               d |= DRAPEAU_ESSAI;
    return d;
}

const etat_vehicule_t *fsm_etat(void)
{
    bool regule = fsm_en_marche() && sorties.regulation;

    expose.etat = etat;
    expose.consigne_pct = regule ? (auto_rpm * 100u) / VITESSE_MAX_RPM : consigne_pct;
    expose.pwm_pct = pwm_pct;
    expose.transitions = compteur_transitions;
    expose.mode = mode;
    expose.cause = cause;
    expose.plafond_pct = plafond_pct;
    expose.drapeaux = calculer_drapeaux();
    expose.consigne_rpm = regule ? auto_rpm : 0u;
    expose.segment = autonome() ? auto_segment : 0u;
    expose.dist_x10 = auto_dist;
    expose.code_defaut = code_defaut;
    expose.mode_essai = essai_actif() ? 1u : 0u;
    return &expose;
}
