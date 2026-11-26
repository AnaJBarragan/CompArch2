/* ---------------------------------------------------------------------------
 * main.c — programme principal en C (CEG 3536)
 *          Base : corrigé du laboratoire 3 (à citer). Étendu pour le projet,
 *          partie I : bus I2C (répartiteur), extenseurs, LCD, TouchPad, PIR,
 *          capteur environnemental, télémétrie contrôlée (E1 à E12).
 *
 * Boucle cadencée par tick_1ms. À chaque tick, dans cet ordre :
 *   1. décompte des ticks manqués (ticks_perdus, E11) ;
 *   2. producteurs : boutons, E-Stop, encodeur, potentiomètre, télémétrie,
 *      PIR, capteur environnemental, surveillance des modules I2C ;
 *   3. commande série éventuelle (un caractère) ;
 *   4. consommateur : machine à états, lignes « E », page, « VERROU » ;
 *   5. fsm_sorties (rampe, DEL, avertisseur) ;
 *   6. UN SEUL travail lourd : trame « T », OU tableau de bord, OU capteur
 *      environnemental (SPI), OU une transaction I2C (répartiteur) ;
 *   7. telem_step (vidage non bloquant de la file d'émission).
 * Le point 6 borne la durée d'un passage : jamais deux travaux lourds dans le
 * même tick (mesure : boucle_max_cycles, DWT ; 4000 cycles = 1 ms).
 *
 * Commandes série : z, t, q, b, a (laboratoire 3) ; x banc de trame_xor8 ;
 * f autotest du tableau ; i nouveau balayage (étalé, une adresse par tick) ;
 * d LCD simulé absent pendant 2 s (E12) ; w mesure DWT d'une écriture de deux
 * octets, puis NACK sur une adresse libre (T1) ; p relecture de registres du
 * TouchPad (L4-A).
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab4.h"
#include "evq.h"
#include "evq_test.h"
#include "acq.h"
#include "adc_dma.h"
#include "telem.h"
#include "fsm.h"
#include "bench_l3a.h"
#include "i2c.h"
#include "ioexp.h"
#include "lcd.h"
#include "touchpad.h"
#include "l4a_defaut.h"
#include "spi.h"
#include "env.h"
#include "tableau.h"
#include "tableau_test.h"
#include "somme_test.h"

/* Pilote du TouchPad : celui de l'équipe, ou celui de L4-A (lab4.h) */
#if L4A_ESSAI
#define TP_INIT()       l4a_tp_init()
#define TP_LIRE(e)      l4a_tp_lire(e)
#else
#define TP_INIT()       tp_init()
#define TP_LIRE(e)      tp_lire(e)
#endif

/* modules I2C surveillés : un bit chacun dans modules_muets */
#define MOD_DEL         (1u << 0)
#define MOD_COMMUT      (1u << 1)
#define MOD_LCD         (1u << 2)
#define MOD_TP          (1u << 3)
#define I2C_ADR_LIBRE   0x10u   /* aucun module à cette adresse (T1)    */
#define SW_N            8u
#define SW_ECHANTILLONS (COMMUT_ANTIREBOND_MS / COMMUT_PERIODE_MS)   /* 3 */

/* tampon DMA : ADC_N_MOY échantillons par voie, entrelacés IN1, IN2, ... */
static volatile uint16_t tampon_adc[ADC_N_VOIES * ADC_N_MOY];
static acq_t acq_pot;      /* potentiomètre : 0..100 %                        */
static acq_t acq_temp;     /* température : dixièmes de degré Celsius         */

static ioexp_t      x_del, x_commut;        /* deux instances du même objet   */
static env_mesure_t mesure_env;             /* dernière mesure (et ta_c10)    */
static int32_t      derniere_consigne = -1;
static uint32_t     trame_due;              /* EV_TELEM consommé              */
static uint32_t     pir_haut;               /* signal PIR vu à 1              */
static uint32_t     tp_echecs;              /* erreurs consécutives du TouchPad */
static uint32_t     modules_muets;          /* MOD_x                          */
static uint8_t      sw_valide;              /* bit à 1 = commutateur enfoncé  */
static uint8_t      sw_compte[SW_N];        /* échantillons différents de suite */
static uint32_t     scan_actif, scan_n;     /* balayage étalé (commande i)    */
static uint8_t      scan_adr, scan_trouvees[8];
static uint32_t     t_commut, t_tp_surv, t_del_essai, t_tableau, t_env;

/* variables à observer au débogueur (Live Expressions) */
volatile uint32_t compteur_pas;             /* passages dans la boucle        */
volatile uint32_t ticks_perdus;             /* E11 : doit rester à 0          */
volatile uint32_t touches_refusees;         /* E4                             */
volatile uint32_t boucle_max_cycles;        /* pire passage (cycles DWT)      */
volatile uint16_t tp_etat;                  /* 12 électrodes : relevé de la table */

void SystemInit(void)
{
    /* horloge MSI 4 MHz de la réinitialisation : rien à faire (HORLOGE_HZ_C) */
}

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void reponse_u32(const char *etiquette, uint32_t v)
{
    char ligne[48];
    char *p = ligne;
    while (*etiquette) *p++ = *etiquette++;
    p = telem_fmt_u32(p, v);
    *p++ = '\r'; *p++ = '\n'; *p = '\0';
    telem_put(ligne);
}

/* ---- consommateur ------------------------------------------------------- */
static bool journalise(evq_type_t type)     /* E10 : types qui ont une ligne E */
{
    return type == EV_ESTOP || type == EV_TOUCHE || type == EV_PIR
        || type == EV_COMMUT || type == EV_DEFAUT;
}

static void consommer(const evq_event_t *e)
{
    switch (e->type) {
    case EV_TELEM:
        trame_due = 1u;                         /* formatée au point 6         */
        return;
    case EV_TOUCHE:
        if (!touch_enabled) {                   /* garde Touch En (E4)         */
            touches_refusees++;
            tableau_verrou();
            telem_evenement(e);
            return;
        }
        break;
    case EV_COMMUT:
        if (e->valeur == SW_PAGE) {             /* SW8 : présentation seulement */
            tableau_page_basculer();
            telem_evenement(e);
            return;
        }
        break;
    default:
        break;
    }
    fsm_traiter(e);
    if (journalise(e->type)) {
        telem_evenement(e);                     /* après le traitement : CCR1 = 0 déjà écrit */
    }
}

/* E-Stop et défauts : jamais perdus, même file pleine (E12) — traités sur-
 * le-champ si la file refuse l'événement.                                  */
static void pousser_garanti(evq_type_t type, uint32_t valeur, uint32_t t)
{
    if (!evq_push(type, valeur, t)) {
        evq_event_t e = { type, valeur, t };
        consommer(&e);
    }
}

/* ---- surveillance des modules I2C (E12) ----------------------------------
 * Trois erreurs consécutives : module muet -> EV_DEFAUT (valeur = adresse).
 * Première réussite ensuite : module rétabli -> EV_DEFAUT | DEFAUT_RETABLI. */
static void surveiller(uint32_t bit, uint32_t echecs, uint8_t adr7, uint32_t t)
{
    uint32_t tp = (bit == MOD_TP) ? DEFAUT_TP : 0u;

    if (!(modules_muets & bit) && echecs >= I2C_ERREURS_DEFAUT) {
        modules_muets |= bit;
        pousser_garanti(EV_DEFAUT, adr7 | tp, t);
    } else if ((modules_muets & bit) && echecs == 0u) {
        modules_muets &= ~bit;
        pousser_garanti(EV_DEFAUT, adr7 | tp | DEFAUT_RETABLI, t);
    }
}

/* ---- demandeurs du bus -------------------------------------------------- */
static void lire_touchpad(uint32_t t)
{
    uint16_t nouvel;
    int32_t  k;

    if (!TP_LIRE(&nouvel)) {
        tp_echecs++;
        return;
    }
    tp_echecs = 0u;
    k = tp_touche(tp_etat, nouvel);             /* un nouvel appui, seul       */
    tp_etat = nouvel;
    if (k >= 0) {
        evq_push(EV_TOUCHE, (uint32_t)k, t);
    }
}

/* Anti-rebond par compteur : un changement n'est validé qu'après
 * SW_ECHANTILLONS lectures de suite (3 x 10 ms = 20 à 30 ms) ; seul le
 * passage à « enfoncé » produit EV_COMMUT (valeur 1 à 8).                  */
static void lire_commutateurs(uint32_t t)
{
    uint8_t brut;
    uint8_t enfonce;

    if (!ioexp_lire(&x_commut, &brut)) {
        return;
    }
    enfonce = (uint8_t)~brut;                   /* niveau bas = enfoncé        */
    for (uint32_t i = 0u; i < SW_N; i++) {
        uint8_t b = (uint8_t)(1u << i);
        if ((enfonce ^ sw_valide) & b) {
            if (++sw_compte[i] >= SW_ECHANTILLONS) {
                sw_compte[i] = 0u;
                sw_valide ^= b;
                if (sw_valide & b) {
                    evq_push(EV_COMMUT, i + 1u, t);
                }
            }
        } else {
            sw_compte[i] = 0u;
        }
    }
}

/* Répartiteur : au plus UNE transaction par appel, par ordre de priorité
 * (énoncé 4.7) ; un module muet n'est réessayé que toutes les 100 ms.      */
static void bus_step(uint32_t t)
{
    uint8_t del = (uint8_t)~tableau_image_del();     /* niveau bas allume   */
    uint32_t p_commut = (ioexp_echecs(&x_commut) >= I2C_ERREURS_DEFAUT)
                      ? I2C_REESSAI_MS : COMMUT_PERIODE_MS;

    if (thi_flag) {                                         /* 1. TouchPad   */
        thi_flag = 0u;
        lire_touchpad(t);
    } else if ((t - t_commut) >= p_commut) {                /* 2. commutateurs */
        t_commut = t;
        lire_commutateurs(t);
    } else if (ioexp_differe(&x_del, del)
               && (ioexp_echecs(&x_del) < I2C_ERREURS_DEFAUT
                   || (t - t_del_essai) >= I2C_REESSAI_MS)) {   /* 3. DEL    */
        t_del_essai = t;
        (void)ioexp_ecrire(&x_del, del);
    } else if (scan_actif) {                                /* balayage « i » */
        if (i2c_sonder(scan_adr)) {
            if (scan_n < sizeof scan_trouvees) {
                scan_trouvees[scan_n] = scan_adr;
            }
            scan_n++;
        }
        if (++scan_adr > 0x77u) {
            scan_actif = 0u;
            telem_balayage(scan_trouvees, scan_n < 8u ? scan_n : 8u);
        }
    } else if ((t - t_tp_surv) >= TP_SURV_MS) {             /* surveillance TP */
        t_tp_surv = t;
        lire_touchpad(t);
    } else {                                                /* 4-5. LCD      */
        lcd_step();                             /* couleur, puis un caractère  */
    }
}

/* ---- commandes série ----------------------------------------------------
 * Retourne true pour un banc d'essai bloquant (plusieurs ms par nature) :
 * le tick suivant n'est alors pas compté dans ticks_perdus.               */
static bool traiter_commande(int rx, uint32_t t)
{
    switch (rx) {
    case 'z':
        evq_init();
        derniere_consigne = -1;
        ticks_perdus = 0u;
        touches_refusees = 0u;
        boucle_max_cycles = 0u;
        telem_put("Z,ok\r\n");
        return false;
    case 't':
        for (uint32_t i = 0u; i < 40u; i++) { evq_push(EV_TEST, i, t); }
        return false;
    case 'q':
        reponse_u32("Q,echecs=", evq_autotest());
        return true;
    case 'b':
        if (etat == ETAT_ARRET) {
            reponse_u32("B,ecarts=", bench_l3a());
            reponse_u32("B,cycles_asm=", bench_cycles_asm);
            reponse_u32("B,cycles_c=", bench_cycles_c);
        } else {
            telem_put("B,refuse (ARRET requis)\r\n");
        }
        return true;
    case 'a':
        reponse_u32("A,pot_brut=", acq_brut(&acq_pot));
        reponse_u32("A,pot_mv=", acq_mv(&acq_pot));
        reponse_u32("A,temp_brut=", acq_brut(&acq_temp));
        reponse_u32("A,temp_mv=", acq_mv(&acq_temp));
        reponse_u32("A,tours_dma=", adc_dma_tours());
        reponse_u32("A,env_id=", env_identifiant());
        reponse_u32("A,boucle_max_cycles=", boucle_max_cycles);
        return true;
    case 'x':
        reponse_u32("X,echecs=", somme_autotest());
        return true;
    case 'f':
        reponse_u32("F,echecs=", tableau_autotest());
        return true;
    case 'i':
        scan_actif = 1u;
        scan_adr = 0x08u;
        scan_n = 0u;
        return false;
    case 'd':
        i2c_simuler_absence(I2C_ADR_LCD, ABSENCE_SIMULEE_MS);
        telem_put("D,ok\r\n");
        return false;
    case 'w': {                 /* T1 : débit du bus, NACK compté, bus libre  */
        static const uint8_t cmd[2] = { 0x80u, 0x0Cu };   /* LCD : afficheur allumé,
                                                             commande sans effet visible */
        uint32_t c0 = DWT->CYCCNT;
        uint32_t s = (uint32_t)i2c_ecrire(I2C_ADR_LCD, cmd, 2u);   /* 3 octets sur le bus */
        uint32_t cycles = DWT->CYCCNT - c0;
        reponse_u32("W,statut=", s);                        /* 0 = I2C_OK       */
        reponse_u32("W,cycles=", cycles);
        reponse_u32("W,us=", cycles / (HORLOGE_HZ_C / 1000000u));
        /* Débit réel : 2 puis 4 octets de données (deux commandes du LCD sans
         * effet visible). La partie logicielle est la même dans les deux cas :
         * la différence est le temps de 2 octets sur le bus, soit 18 bits.
         * Minimum de 8 essais : écarte ceux qu'une interruption a allongés.  */
        {
            static const uint8_t cmd4[4] = { 0x80u, 0x0Cu, 0x80u, 0x0Cu };
            uint32_t min2 = 0xFFFFFFFFu, min4 = 0xFFFFFFFFu;
            for (uint32_t k = 0u; k < 8u; k++) {
                uint32_t a = DWT->CYCCNT;
                (void)i2c_ecrire(I2C_ADR_LCD, cmd, 2u);
                uint32_t b = DWT->CYCCNT;
                (void)i2c_ecrire(I2C_ADR_LCD, cmd4, 4u);
                uint32_t c = DWT->CYCCNT;
                if (b - a < min2) min2 = b - a;
                if (c - b < min4) min4 = c - b;
            }
            reponse_u32("W,cycles_2octets=", min2);
            reponse_u32("W,cycles_4octets=", min4);
            if (min4 > min2) {
                /* f = 18 bits x HORLOGE_HZ / (min4 - min2) ; en dixièmes de kHz */
                reponse_u32("W,khz_x10=", (18u * (HORLOGE_HZ_C / 100u)) / (min4 - min2));
            }
        }
        reponse_u32("W,nack=", (uint32_t)i2c_ecrire(I2C_ADR_LIBRE, cmd, 1u)); /* 1 = I2C_NACK */
        reponse_u32("W,busy=", (I2C2->ISR & I2C_ISR_BUSY) ? 1u : 0u);       /* 0 attendu */
        reponse_u32("W,i2c_erreurs=", i2c_erreurs());
        return true;
    }
    case 'p': {                 /* L4-A : relecture de registres du TouchPad   */
        static const uint8_t regs[] = { 0x5Eu, 0x41u, 0x42u, 0x5Bu, 0x5Cu, 0x5Du };
        for (uint32_t i = 0u; i < sizeof regs; i++) {
            uint8_t r = regs[i], v = 0u;
            uint32_t s = (uint32_t)i2c_ecrire_lire(I2C_ADR_TOUCHPAD, &r, 1u, &v, 1u);
            reponse_u32("P,registre=", r);
            reponse_u32("P,valeur=", v);
            reponse_u32("P,statut=", s);                    /* 0 = I2C_OK       */
        }
        reponse_u32("P,tp_etat=", tp_etat);
        return true;
    }
    default:
        return false;
    }
}

int main(void)
{
    uint32_t fenetre = 0u, periode_telem = 0u, t_prec;
    uint32_t banc = 0u;
    uint8_t  adresses[8];
    uint32_t n;
    evq_event_t e;

    /* couche assembleur (sorties en état sûr avant les interruptions) */
    gpio_init();
    buzzer_init();
    motor_init();
    encoder_init();
    estop_init();
    systick_init(1000u);
    /* couche C */
    dwt_init();
    evq_init();
    adc_dma_init(tampon_adc, ADC_N_MOY);
    acq_init(&acq_pot, &tampon_adc[0], ADC_N_VOIES, ADC_N_MOY, 100, (int32_t)ADC_VREF_MV, 0);
    acq_config_filtre(&acq_pot, CONSIGNE_HYSTERESIS, POT_ZONE_MORTE_PCT, 0, 100);
    acq_init(&acq_temp, &tampon_adc[1], ADC_N_VOIES, ADC_N_MOY,
             -10000, -TEMP_PENTE_UV_PAR_C,
             TEMP_C10_REF + (TEMP_MV_REF * 10000) / (-TEMP_PENTE_UV_PAR_C));
    telem_init(TELEM_BAUDS);
    fsm_init();
    telem_entete();

    /* projet, partie I : bus I2C et balayage (ligne « I », E1) */
    i2c_init();
    n = i2c_scan(adresses, 8u);
    telem_balayage(adresses, n < 8u ? n : 8u);
    /* extenseurs : DEL éteintes ; commutateurs en entrée (0xFF une fois) */
    ioexp_init(&x_del, I2C_ADR_DEL);
    ioexp_init(&x_commut, I2C_ADR_COMMUT);
    (void)ioexp_ecrire(&x_del, 0xFFu);
    (void)ioexp_ecrire(&x_commut, 0xFFu);
    /* TouchPad : configuration complète (avant la boucle : ~40 transactions) */
    tp_echecs = TP_INIT() ? 0u : I2C_ERREURS_DEFAUT;
    lcd_init();
    tableau_init();
    /* SPI et capteur environnemental (identifiant, coefficients) */
    spi_init();
    (void)env_init();
    mesure_env.valide = 0u;
    mesure_env.ta_c10 = acq_valeur(&acq_temp);
    /* EXTI6 (THI) et EXTI9 (PIR) en dernier */
    exti_l4_init();
    pir_flag = 0u;
    thi_flag = 1u;                  /* première lecture du TouchPad : retire
                                       une demande déjà en attente           */
    t_commut = t_tp_surv = t_del_essai = t_tableau = t_env = ticks_ms;
    t_prec = ticks_ms;

    for (;;) {
        if (tick_1ms == 0u) {
            continue;                                   /* attente du tick    */
        }
        tick_1ms = 0u;
        uint32_t c0 = DWT->CYCCNT;
        compteur_pas++;
        uint32_t t = ticks_ms;

        /* --- 1. ticks manqués (E11) --- */
        if (!banc && (t - t_prec) > 1u) {
            ticks_perdus += t - t_prec - 1u;
        }
        t_prec = t;

        /* --- 2. producteurs d'événements --- */
        if (button_pressed(BTN_USER))  { evq_push(EV_USER, 0u, t); }
        if (button_pressed(BTN_TOUCH)) { evq_push(EV_TOUCH, 0u, t); }
        if (estop_flag) {
            estop_flag = 0u;
            motor_stop_now();                           /* redondant avec l'ISR */
            pousser_garanti(EV_ESTOP, 0u, t);
        }
        if (++fenetre >= FENETRE_VITESSE_MS) {
            fenetre = 0u;
            uint32_t fronts = encoder_read(&sens_mesure);
            evq_push(EV_VITESSE, fronts, t);
        }
        int32_t c = acq_valeur(&acq_pot);
        if (c != derniere_consigne) {
            derniere_consigne = c;
            evq_push(EV_CONSIGNE, (uint32_t)c, t);
        }
        if (++periode_telem >= TELEM_PERIODE_MS) {
            periode_telem = 0u;
            evq_push(EV_TELEM, 0u, t);
        }
        /* PIR : front montant retenu par EXTI9 (valeur = instant de l'IRQ),
         * puis retour du signal à 0 (lu sur PB9)                             */
        if (pir_flag) {
            pir_flag = 0u;
            pir_haut = 1u;
            evq_push(EV_PIR, (pir_t_ms != 0u) ? pir_t_ms : 1u, t);
        } else if (pir_haut && (GPIOB->IDR & GPIO_IDR_ID9) == 0u) {
            pir_haut = 0u;
            evq_push(EV_PIR, 0u, t);
        }
        /* capteur environnemental : nouvelle mesure (E7) */
        if (env_mesure(&mesure_env)) {
            mesure_env.ta_c10 = acq_valeur(&acq_temp);
            tableau_env(&mesure_env);
            evq_push(EV_ENV, mesure_env.h_x10, t);
        }
        /* modules I2C muets ou rétablis (E12) */
        surveiller(MOD_DEL, ioexp_echecs(&x_del), I2C_ADR_DEL, t);
        surveiller(MOD_COMMUT, ioexp_echecs(&x_commut), I2C_ADR_COMMUT, t);
        surveiller(MOD_LCD, lcd_echecs(), I2C_ADR_LCD, t);
        surveiller(MOD_TP, tp_echecs, I2C_ADR_TOUCHPAD, t);
        adc_dma_step();

        /* --- 3. commandes série --- */
        int rx = telem_rx();
        banc = (rx >= 0) ? traiter_commande(rx, t) : false;

        /* --- 4. consommateur --- */
        while (evq_pop(&e)) {
            consommer(&e);
        }
        /* --- 5. sorties --- */
        fsm_sorties();

        /* --- 6. un seul travail lourd par tick --- */
        if (trame_due) {
            trame_due = 0u;
            telem_trame(fsm_etat(), acq_valeur(&acq_temp), evq_total(), evq_debordements(),
                        &mesure_env, i2c_erreurs(), ticks_perdus);
        } else if ((t - t_tableau) >= TABLEAU_PERIODE_MS) {
            t_tableau = t;
            tableau_step();
        } else if ((t - t_env) >= ENV_SCRUTATION_MS) {
            t_env = t;
            env_step();
        } else {
            bus_step(t);
        }
        /* --- 7. émission --- */
        telem_step();

        if (!banc) {
            uint32_t dc = DWT->CYCCNT - c0;
            if (dc > boucle_max_cycles) {
                boucle_max_cycles = dc;
            }
        }
    }
}
