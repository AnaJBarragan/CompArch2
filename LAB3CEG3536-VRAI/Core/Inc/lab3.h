/* ---------------------------------------------------------------------------
 * lab3.h — interface du laboratoire 3 (CEG 3536, automne 2026)
 * Routine assembleur nouvelle, types communs et constantes de la couche C.
 * Inclut lab2.h (et donc lab1.h) : toutes les routines assembleur des
 * laboratoires 1 et 2 restent appelables depuis le C.
 * ------------------------------------------------------------------------- */
#ifndef LAB3_H
#define LAB3_H

#include <stdint.h>
#include <stdbool.h>
#include "lab2.h"

/* ---- Routine assembleur du laboratoire 3 (vitesse.s) -------------------- */
/* Vitesse de l'arbre de sortie en tr/min, format Q16.16, pour une fenêtre de
 * FENETRE_VITESSE_MS = 250 ms : tr/min = fronts x 240 / 748 = fronts x k / 65536
 * avec k = round(240 x 65536 / 748) = 21028 (erreur relative +0,002 %).       */
#define RPM_Q16_K           21028u
uint32_t enc_fronts_vers_rpm_q16(uint32_t fronts);

/* ---- Constantes de la couche C ----------------------------------------- */
#define HORLOGE_HZ_C        4000000u /* MSI après réinitialisation (SystemInit vide) */
#define FENETRE_VITESSE_MS  250u    /* propre au lab. 3 (lab. 2 : 500 ms)       */
#define RAMPE_PAS_MS        10u
#define CLIGNOTEMENT_DEMI_MS 250u
#define TELEM_PERIODE_MS    200u    /* télémétrie à 5 Hz (E10)                  */
#define TELEM_BAUDS         115200u
#define ADC_N_VOIES         2u      /* IN1 = potentiomètre, IN2 = température   */
#define ADC_N_MOY           16u     /* échantillons par voie dans le tampon DMA */
#define ADC_VREF_MV         3300u
#define POT_ZONE_MORTE_PCT  3       /* E5 : < 3 % -> 0, > 97 % -> 100           */
#define CONSIGNE_HYSTERESIS 2       /* E5 : la consigne ne change que de > 2 pts */

/* Étalonnage de la température (E6). Capteur LMT86 (annexe D du guide) :
 * pente -10,9 mV/°C, environ 2 100 mV à 0 °C et 1 777 mV à 30 °C (fiche
 * technique), soit environ 1 831 mV à 25 °C. Le point de référence doit être
 * relevé par l'équipe (commande série a, thermomètre de référence) : une
 * lecture très éloignée (par exemple 800 mV) indique une entrée flottante
 * (S2 en position Ext sans circuit sur J5).                                   */
#define TEMP_PENTE_UV_PAR_C (-10900)   /* microvolts par degré                 */
#define TEMP_MV_REF         1831       /* mV lus à TEMP_C10_REF (fiche LMT86)  */
#define TEMP_C10_REF        250        /* 25,0 °C                              */

/* ---- État du véhicule exposé à la télémétrie (fsm_etat) ----------------- */
typedef struct {
    uint32_t etat;              /* ETAT_x (lab1.h)                              */
    uint32_t sens;              /* SENS_x mesuré par l'encodeur                 */
    uint32_t consigne_pct;      /* consigne appliquée (0 en ARRÊT)              */
    uint32_t pwm_pct;           /* rapport cyclique courant (rampe)             */
    uint32_t vitesse_rpm_q16;   /* dernière vitesse (Q16.16)                    */
    uint32_t transitions;       /* nombre de changements d'état                 */
} etat_vehicule_t;

#endif /* LAB3_H */
