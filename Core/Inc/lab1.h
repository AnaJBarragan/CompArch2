/* ---------------------------------------------------------------------------
 * lab1.h — interface C des routines assembleur du laboratoire 1
 *          (CEG 3536, automne 2026). Utilisé à partir du laboratoire 3.
 * Toutes les routines respectent l'AAPCS : paramètres R0-R3, retour R0.
 * ------------------------------------------------------------------------- */
#ifndef LAB1_H
#define LAB1_H

#include <stdint.h>

/* Identifiants de DEL (led_set) */
#define LED_AUCUNE  0u
#define LED_ROUGE   1u
#define LED_VERTE   2u
#define LED_BLEUE   3u

/* Identifiants de bouton (button_raw, button_pressed) */
#define BTN_USER    0u
#define BTN_ESTOP   1u
#define BTN_TOUCH   2u

/* États de la machine */
#define ETAT_ARRET            0u
#define ETAT_MARCHE_AVANT     1u
#define ETAT_MARCHE_ARRIERE   2u
#define ETAT_ARRET_URGENCE    3u

void     gpio_init(void);
void     led_set(uint32_t del);
uint32_t button_raw(uint32_t id);
uint32_t button_pressed(uint32_t id);
void     delay_ms(uint32_t ms);
void     estop_init(void);
void     fsm_init(void);
void     fsm_step(void);

/* Variables partagées (définies en .bss dans les fichiers .s) */
extern volatile uint32_t etat;
extern volatile uint32_t estop_flag;
extern volatile uint32_t touch_enabled;
extern volatile uint32_t compteur_transitions;

#endif /* LAB1_H */
