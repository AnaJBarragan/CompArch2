/* ---------------------------------------------------------------------------
 * test_fsm.c — essai hôte de la machine à états (équipe, facultatif, E9)
 * Les routines assembleur sont remplacées par des bouchons qui notent CCR1,
 * STB, la DEL et les bips. Scénarios : T5, T6, T8, E9, E12.
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_fsm test_fsm.c ../Core/Src/fsm.c
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include "fsm.h"
#include "lab4.h"
volatile uint32_t estop_flag, buzzer_continu, sens_mesure, ticks_ms, tick_1ms;
uint32_t ccr1 = 0, stb = 0, estop_enfonce = 0, bips = 0, del = 0;
uint32_t motor_set_dir(uint32_t d){ if(d==0){ccr1=0;stb=0;return 0;} if(ccr1) return 1; stb=1; return 0; }
uint32_t motor_set_pwm(uint32_t p){ ccr1 = p*2; return ccr1; }
void motor_stop_now(void){ ccr1=0; stb=0; }
void buzzer_bip(uint32_t n){ bips += n; }
void buzzer_step(void){}
void led_set(uint32_t d){ del = d; }
uint32_t button_raw(uint32_t id){ return id==BTN_ESTOP ? estop_enfonce : 0; }
uint32_t enc_fronts_vers_rpm_q16(uint32_t f){ return f<<16; }
static int echecs=0, n=0;
static void ev(evq_type_t t, uint32_t v){ evq_event_t e={t,v,0}; fsm_traiter(&e); }
static void ticks(int k){ while(k--) fsm_sorties(); }
#define VERIF(cond, txt) do{ n++; if(!(cond)){ echecs++; printf("ECHEC %s (etat=%u cause=%u consigne=%u ccr1=%u stb=%u)\n", txt, (unsigned)fsm_etat()->etat,(unsigned)fsm_etat()->cause,(unsigned)fsm_etat()->consigne_pct,(unsigned)ccr1,(unsigned)stb);} }while(0)
int main(void){
  const etat_vehicule_t *v;
  fsm_init(); v=fsm_etat();
  VERIF(v->etat==ETAT_ARRET && v->plafond_pct==100 && v->mode==MODE_MANUEL, "init");
  /* T5 : plafond en marche */
  ev(EV_TOUCH,0); VERIF(touch_enabled==1, "Touch En autorise");
  ev(EV_CONSIGNE,100); ev(EV_USER,0); v=fsm_etat();
  VERIF(v->etat==ETAT_MARCHE_AVANT && v->consigne_pct==100 && stb==1, "depart 100%");
  ticks(2000); VERIF(fsm_etat()->pwm_pct==100, "rampe a 100");
  ev(EV_TOUCHE,5); VERIF(fsm_etat()->consigne_pct==50, "touche 5 -> 50");
  ticks(300); VERIF(fsm_etat()->pwm_pct==70, "rampe descend (1 pt/10 ms)");
  ev(EV_TOUCHE,9); VERIF(fsm_etat()->consigne_pct==90, "touche 9 -> 90");
  ev(EV_TOUCHE,0); VERIF(fsm_etat()->consigne_pct==100 && fsm_etat()->plafond_pct==100, "touche 0 -> 100");
  ev(EV_TOUCHE,TOUCHE_NEXT); VERIF(fsm_etat()->mode==MODE_MANUEL, "Next refuse en marche");
  /* T6 : PIR en marche */
  ev(EV_PIR,1234); v=fsm_etat();
  VERIF(v->etat==ETAT_ARRET_EVENEMENT && v->cause==CAUSE_OBSTACLE && ccr1==0 && stb==0, "PIR -> OBSTACLE, CCR1=0 STB=0");
  ticks(1); VERIF((v->drapeaux & DRAPEAU_OBSTACLE) && del==LED_ROUGE, "R2 et DEL rouge");
  bips=0; ticks(1000); VERIF(bips==2, "bips repetes 500 ms");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT, "Fn refuse PIR haut");
  ev(EV_PIR,0); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT, "PIR bas : pas de reprise auto");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET, "Fn -> ARRET");
  ev(EV_USER,0); VERIF(fsm_etat()->etat==ETAT_MARCHE_ARRIERE, "nouvel appui User (sens alterne)");
  ev(EV_USER,0);
  /* PIR en ARRET : pas de changement d'etat, R2 suit */
  ev(EV_PIR,99); VERIF(fsm_etat()->etat==ETAT_ARRET && (fsm_etat()->drapeaux&DRAPEAU_OBSTACLE), "PIR en ARRET : R2 seulement");
  ev(EV_PIR,0);
  /* T8 : mode autonome */
  ev(EV_TOUCHE,TOUCHE_NEXT); VERIF(fsm_etat()->mode==MODE_AUTONOME, "Next en ARRET -> AUT");
  ev(EV_CONSIGNE,15); ev(EV_USER,0); VERIF(fsm_etat()->consigne_pct==CONSIGNE_AUTO_PCT, "AUT : consigne 60, pot sans effet");
  ev(EV_COMMUT,SW_LIMITATION); VERIF(fsm_etat()->consigne_pct==30 && (fsm_etat()->drapeaux&DRAPEAU_LIMITATION), "SW3 limitation 30");
  ev(EV_COMMUT,SW_FEU); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT && fsm_etat()->cause==CAUSE_FEU, "SW1 feu");
  ev(EV_COMMUT,SW_OBSTACLE); VERIF(fsm_etat()->cause==CAUSE_OBSTACLE, "SW2 : obstacle plus prioritaire affiche");
  ev(EV_COMMUT,SW_OBSTACLE); ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT && fsm_etat()->cause==CAUSE_FEU, "Fn refuse : feu encore actif, affiche FEU");
  ev(EV_COMMUT,SW_FEU); ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET, "feu eteint + Fn -> ARRET");
  ev(EV_USER,0); ev(EV_COMMUT,SW_FIN); VERIF(fsm_etat()->etat==ETAT_ARRET && fsm_etat()->cause==CAUSE_FIN && (fsm_etat()->drapeaux&DRAPEAU_FIN), "SW4 fin sans acquittement");
  ev(EV_USER,0); VERIF(fsm_etat()->cause==CAUSE_AUCUNE && !(fsm_etat()->drapeaux&DRAPEAU_FIN) && fsm_etat()->etat==ETAT_MARCHE_AVANT, "fin effacee au depart");
  ev(EV_COMMUT,SW_DEFAUT); VERIF(fsm_etat()->cause==CAUSE_DEFAUT, "SW5 defaut simule");
  ev(EV_COMMUT,SW_DEFAUT); ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET, "defaut leve + Fn");
  ev(EV_COMMUT,SW_LIMITATION);
  /* mode MANUEL : commutateurs ignores */
  ev(EV_TOUCHE,TOUCHE_NEXT); ev(EV_COMMUT,SW_FEU); ev(EV_USER,0);
  VERIF(fsm_etat()->etat==ETAT_MARCHE_ARRIERE && fsm_etat()->drapeaux==DRAPEAU_TOUCH, "MAN : SW1 ignore");
  /* humidite : hysteresis */
  ev(EV_CONSIGNE,80); ev(EV_ENV,710); VERIF(fsm_etat()->consigne_pct==30, "humidite > 70 % : limitation");
  ev(EV_ENV,660); VERIF(fsm_etat()->consigne_pct==30, "hysteresis : 66 % garde la limitation");
  ev(EV_ENV,640); VERIF(fsm_etat()->consigne_pct==80, "< 65 % : levee");
  /* E-Stop prioritaire, acquittement Fn seulement relache */
  estop_enfonce=1; ev(EV_ESTOP,0); VERIF(fsm_etat()->etat==ETAT_ARRET_URGENCE && ccr1==0, "E-Stop");
  ev(EV_PIR,5); VERIF(fsm_etat()->etat==ETAT_ARRET_URGENCE, "PIR n'efface pas l'urgence");
  ev(EV_PIR,0); ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET_URGENCE, "Fn refuse E-Stop enfonce");
  estop_enfonce=0; ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET && buzzer_continu==0, "Fn acquitte l'urgence");
  /* E12 : module muet (LCD) puis retabli */
  ev(EV_DEFAUT,0x3E); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT && fsm_etat()->cause==CAUSE_DEFAUT, "LCD muet -> DEFAUT meme a l'arret");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT, "Fn refuse tant que muet");
  ev(EV_DEFAUT,0x3E|DEFAUT_RETABLI); ev(EV_TOUCHE,TOUCHE_FN); VERIF(fsm_etat()->etat==ETAT_ARRET, "retabli + Fn");
  /* TouchPad muet : Touch En acquitte */
  ev(EV_DEFAUT,0x5A|DEFAUT_TP); ev(EV_DEFAUT,0x5A|DEFAUT_TP|DEFAUT_RETABLI);
  ev(EV_TOUCH,0); VERIF(fsm_etat()->etat==ETAT_ARRET && touch_enabled==1, "TP muet puis retabli : Touch En acquitte");
  /* garde : User refuse vers un obstacle */
  ev(EV_PIR,77); ev(EV_USER,0); VERIF(fsm_etat()->etat==ETAT_ARRET_EVENEMENT && stb==0, "depart refuse PIR haut");
  printf("%d verifications, %d echec(s)\n", n, echecs);
  return echecs != 0;
}
