/* ---------------------------------------------------------------------------
 * main.c — programme principal en C (CEG 3536, laboratoire 3)
 *          (remplace main.s du laboratoire 2)
 *
 * Initialise la couche assembleur (laboratoires 1 et 2), puis les pilotes C
 * (ADC/DMA, LPUART1, DWT), et exécute une boucle cadencée par tick_1ms (E1) :
 *   production d'événements (boutons, E-Stop, encodeur, consigne, télémétrie)
 *   -> consommation par la machine à états -> sorties -> émission série.
 * Commandes série facultatives (un caractère) : z = remise à zéro des
 * compteurs ; t = 40 événements de test (saturation de la file, E12) ;
 * q = autotest de la file (E11) ; b = banc de comparaison L3-A (en ARRÊT) ;
 * a = lectures brutes de l'ADC.
 * ------------------------------------------------------------------------- */
#include "stm32l552xx.h"
#include "lab3.h"
#include "evq.h"
#include "evq_test.h"
#include "acq.h"
#include "adc_dma.h"
#include "telem.h"
#include "fsm.h"
#include "bench_l3a.h"

/* tampon DMA : ADC_N_MOY échantillons par voie, entrelacés IN1, IN2, ... */
static volatile uint16_t tampon_adc[ADC_N_VOIES * ADC_N_MOY];
static acq_t acq_pot;      /* potentiomètre : 0..100 %                        */
static acq_t acq_temp;     /* température : dixièmes de degré Celsius         */

volatile uint32_t compteur_pas;          /* passages dans la boucle (watch)    */

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

int main(void)
{
    uint32_t fenetre = 0u, periode_telem = 0u;
    int32_t  derniere_consigne = -1;
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
    /* potentiomètre : % = mv x 100 / 3300 (version 1 : sans filtre)        */
    acq_init(&acq_pot, &tampon_adc[0], ADC_N_VOIES, ADC_N_MOY, 100, (int32_t)ADC_VREF_MV, 0);
    /* température (dixièmes de °C) : t10 = mv x (-10000) / 10900 + C
     * avec C = TEMP_C10_REF + round(TEMP_MV_REF x 10000 / 10900) (relation linéaire
     * étalonnée, E6) ; TEMP_MV_REF et TEMP_C10_REF relevés à l'étalonnage.   */
    acq_init(&acq_temp, &tampon_adc[1], ADC_N_VOIES, ADC_N_MOY,
             -10000, -TEMP_PENTE_UV_PAR_C,
             TEMP_C10_REF + (TEMP_MV_REF * 10000 - TEMP_PENTE_UV_PAR_C / 2)
                            / (-TEMP_PENTE_UV_PAR_C));      /* arrondi, = 1930 */
    telem_init(TELEM_BAUDS);
    fsm_init();
    telem_entete();

    for (;;) {
        if (tick_1ms == 0u) {
            continue;                                   /* attente du tick    */
        }
        tick_1ms = 0u;
        compteur_pas++;
        uint32_t t = ticks_ms;

        /* --- producteurs d'événements --- */
        if (button_pressed(BTN_USER))  { evq_push(EV_USER, 0u, t); }
        if (button_pressed(BTN_TOUCH)) { evq_push(EV_TOUCH, 0u, t); }
        if (estop_flag) {
            estop_flag = 0u;
            motor_stop_now();                           /* redondant avec l'ISR */
            evq_push(EV_ESTOP, 0u, t);
        }
        /* E3 : fin de fenêtre de l'encodeur toutes les 250 ms. encoder_read
         * (encoder.s) retourne |fronts| depuis la lecture précédente et écrit
         * le sens mesuré dans sens_mesure.                                   */
        if (++fenetre >= FENETRE_VITESSE_MS) {
            fenetre = 0u;
            uint32_t fronts = encoder_read(&sens_mesure);
            evq_push(EV_VITESSE, fronts, t);
        }
        /* E5 : un événement seulement quand la consigne lue change          */
        int32_t c = acq_valeur(&acq_pot);
        if (c != derniere_consigne) {
            derniere_consigne = c;
            evq_push(EV_CONSIGNE, (uint32_t)c, t);
        }
        if (++periode_telem >= TELEM_PERIODE_MS) {
            periode_telem = 0u;
            evq_push(EV_TELEM, 0u, t);
        }
        adc_dma_step();

        /* --- commandes série facultatives --- */
        int rx = telem_rx();
        if (rx == 'z') {
            evq_init();
            derniere_consigne = -1;
            telem_put("Z,ok\r\n");
        } else if (rx == 't') {
            for (uint32_t i = 0u; i < 40u; i++) { evq_push(EV_TEST, i, t); }
        } else if (rx == 'q') {
            reponse_u32("Q,echecs=", evq_autotest());
        } else if (rx == 'b') {
            if (etat == ETAT_ARRET) {
                reponse_u32("B,ecarts=", bench_l3a());
                reponse_u32("B,cycles_asm=", bench_cycles_asm);
                reponse_u32("B,cycles_c=", bench_cycles_c);
            } else {
                telem_put("B,refuse (ARRET requis)\r\n");
            }
        } else if (rx == 'a') {
            reponse_u32("A,pot_brut=", acq_brut(&acq_pot));
            reponse_u32("A,pot_mv=", acq_mv(&acq_pot));
            reponse_u32("A,temp_brut=", acq_brut(&acq_temp));
            reponse_u32("A,temp_mv=", acq_mv(&acq_temp));
            reponse_u32("A,tours_dma=", adc_dma_tours());
        }

        /* --- consommateur : machine à états --- */
        while (evq_pop(&e)) {
            if (e.type == EV_TELEM) {
                telem_trame(fsm_etat(), acq_valeur(&acq_temp), evq_total(), evq_debordements());
            } else {
                fsm_traiter(&e);
                if (e.type == EV_ESTOP) {
                    telem_evenement("ESTOP", e.t_ms);   /* E9 : consigné    */
                }
            }
        }
        fsm_sorties();
        telem_step();
    }
}
