/* ---------------------------------------------------------------------------
 * test_fsm.c — essai hôte de la machine à états (équipe, E9, E15 à E19)
 * fsm.c est compilé avec le code GÉNÉRÉ (automate_gen.c) et le module essai ;
 * les routines assembleur sont remplacées par des bouchons qui notent CCR1,
 * STB, la DEL et les bips. Scénarios : T5, T6, T8, T13, T15, T16, E12, E22.
 *   make fsm   (ou)
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_fsm test_fsm.c \
 *       ../Core/Src/fsm.c ../Core/Src/automate_gen.c ../Core/Src/essai.c
 * Le code généré doit d'abord être produit dans Core/ :
 *   python ../outils/automate.py generer automate/automate.json --inc ../Core/Inc --src ../Core/Src
 * ------------------------------------------------------------------------- */
#include <stdio.h>
#include "fsm.h"
#include "projet.h"
#include "surv.h"
#include "essai.h"
#include "automate_gen.h"

/* ---- bouchons de la couche assembleur ---- */
volatile uint32_t estop_flag, buzzer_continu, sens_mesure, ticks_ms, tick_1ms;
uint32_t ccr1 = 0, stb = 0, estop_enfonce = 0, bips = 0, del = 0;
uint32_t motor_set_dir(uint32_t d){ if(d==0){ccr1=0;stb=0;return 0;} if(ccr1) return 1; stb=1; return 0; }
uint32_t motor_set_pwm(uint32_t p){ ccr1 = p*2; return ccr1; }
void motor_stop_now(void){ ccr1=0; stb=0; }
void buzzer_bip(uint32_t n){ bips += n; }
void buzzer_step(void){}
void led_set(uint32_t d){ del = d; }
uint32_t button_raw(uint32_t id){ return id==BTN_ESTOP ? estop_enfonce : 0; }
uint32_t enc_fronts_vers_rpm_q16(uint32_t f){ return f * 21028u; }

static int echecs=0, n=0;
static void ev(evq_type_t t, uint32_t v){ evq_event_t e={t,v,0}; fsm_traiter(&e); }
static void ticks(int k){ while(k--) fsm_sorties(); }
static const etat_vehicule_t *V(void){ return fsm_etat(); }
#define VERIF(cond, txt) do{ n++; if(!(cond)){ echecs++; printf("ECHEC %s (etat=%u cause=%u code=%u consigne=%u ccr1=%u stb=%u)\n", txt, (unsigned)V()->etat,(unsigned)V()->cause,(unsigned)V()->code_defaut,(unsigned)V()->consigne_pct,(unsigned)ccr1,(unsigned)stb);} else printf("  ok   %s\n", txt); }while(0)

int main(void){
  printf("fsm : automate genere, somme de la table %08X\n", (unsigned)AUTO_SOMME);
  essai_init();
  fsm_init();
  VERIF(V()->etat==ETAT_INIT && ccr1==0 && stb==0 && V()->plafond_pct==100, "E22 : INIT au demarrage, moteur coupe");
  ev(EV_USER,0); VERIF(V()->etat==ETAT_INIT && stb==0, "User ignore pendant INIT");
  fsm_fin_init(DEFAUT_AUCUN); VERIF(V()->etat==ETAT_ARRET, "ressources conformes : INIT -> PRET (ARRET)");

  /* T5 : plafond en marche (MANUEL) */
  ev(EV_TOUCH,0); VERIF(touch_enabled==1, "Touch En autorise");
  ev(EV_CONSIGNE,100); ev(EV_USER,0);
  VERIF(V()->etat==ETAT_MARCHE_AVANT && V()->consigne_pct==100 && stb==1, "depart 100 % en MANUEL");
  ticks(2000); VERIF(V()->pwm_pct==100, "rampe a 100");
  ev(EV_TOUCHE,5); VERIF(V()->consigne_pct==50, "touche 5 -> 50");
  ticks(300); VERIF(V()->pwm_pct==70, "rampe descend (1 pt/10 ms)");
  ev(EV_TOUCHE,0); VERIF(V()->consigne_pct==100, "touche 0 -> 100");
  ev(EV_TOUCHE,TOUCHE_NEXT); VERIF(V()->mode==MODE_MANUEL, "Next refuse en marche");

  /* T6 : PIR en marche */
  ev(EV_PIR,1234);
  VERIF(V()->etat==ETAT_ARRET_EVENEMENT && V()->cause==CAUSE_OBSTACLE && ccr1==0 && stb==0, "PIR -> OBSTACLE, CCR1=0 STB=0");
  ticks(1); VERIF((V()->drapeaux & DRAPEAU_OBSTACLE) && del==LED_ROUGE, "R2 et DEL rouge");
  bips=0; ticks(1000); VERIF(bips==2, "bips repetes toutes les 500 ms");
  ev(EV_USER,0); VERIF(V()->etat==ETAT_ARRET_EVENEMENT, "User ignore : aucune reprise");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET_EVENEMENT, "Fn refuse, PIR encore haut");
  ev(EV_PIR,0); VERIF(V()->etat==ETAT_ARRET_EVENEMENT, "PIR bas : pas de reprise automatique");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET, "Fn -> PRET");
  ev(EV_USER,0); VERIF(V()->etat==ETAT_MARCHE_ARRIERE, "nouvel appui User : sens alterne");
  ev(EV_USER,0);
  ev(EV_PIR,99); ev(EV_USER,0);
  VERIF(V()->etat==ETAT_ARRET && stb==0 && (V()->drapeaux&DRAPEAU_OBSTACLE), "PIR haut en PRET : R2, depart refuse");
  ev(EV_PIR,0);

  /* T8, T13 : mode AUTONOME, trajet regule */
  ev(EV_TOUCHE,TOUCHE_NEXT); VERIF(V()->mode==MODE_AUTONOME, "Next en PRET -> AUTONOME");
  ev(EV_USER,0);
  fsm_auto(45u, 150u, 1u, 0u);
  VERIF(V()->etat==ETAT_MARCHE_AVANT && V()->consigne_rpm==150 && V()->consigne_pct==44
        && V()->segment==1, "AUTONOME : consigne du trajet 150 tr/min (44 %), segment 1");
  ticks(1000); VERIF(V()->pwm_pct==45, "la rampe rejoint la commande du regulateur");
  ev(EV_COMMUT,SW_LIMITATION); VERIF(fsm_plafond_pct()==LIMITATION_PCT && (V()->drapeaux&DRAPEAU_LIMITATION), "SW3 : plafond effectif 30 %");
  ev(EV_COMMUT,SW_LIMITATION);
  ev(EV_COMMUT,SW_FEU); VERIF(V()->etat==ETAT_ARRET_EVENEMENT && V()->cause==CAUSE_FEU && V()->consigne_rpm==0, "SW1 : feu rouge");
  ev(EV_COMMUT,SW_OBSTACLE); VERIF(V()->cause==CAUSE_OBSTACLE, "SW2 : obstacle plus prioritaire affiche");
  ev(EV_COMMUT,SW_OBSTACLE); ev(EV_TOUCHE,TOUCHE_FN);
  VERIF(V()->etat==ETAT_ARRET_EVENEMENT && V()->cause==CAUSE_FEU, "Fn refuse : feu encore allume, FEU affiche");
  ev(EV_COMMUT,SW_FEU); ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET, "feu eteint + Fn -> PRET");
  ev(EV_USER,0); VERIF(V()->etat==ETAT_MARCHE_ARRIERE, "reprise du trajet (User)");
  ev(EV_FIN,105);
  VERIF(V()->etat==ETAT_ARRET && V()->cause==CAUSE_FIN && (V()->drapeaux&DRAPEAU_FIN) && stb==0, "EV_FIN : PRET avec FIN, sans acquittement");
  ev(EV_USER,0); VERIF(V()->cause==CAUSE_AUCUNE && !(V()->drapeaux&DRAPEAU_FIN), "FIN effacee au depart suivant");
  ev(EV_USER,0);

  /* E18 : defaut simule (SW5) puis encodeur muet */
  ev(EV_COMMUT,SW_DEFAUT); VERIF(V()->etat==ETAT_DEFAUT && V()->code_defaut==DEFAUT_SIMULE, "SW5 a l'arret : DEFAUT, code 5");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_DEFAUT, "Fn refuse : SW5 encore actif");
  ev(EV_COMMUT,SW_DEFAUT); ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET && V()->code_defaut==0, "SW5 relache + Fn -> PRET");
  ev(EV_USER,0);
  ev(EV_DEFAUT, EV_DEFAUT_CODE_BASE + DEFAUT_ENCODEUR);
  VERIF(V()->etat==ETAT_DEFAUT && V()->code_defaut==DEFAUT_ENCODEUR && ccr1==0 && stb==0 && (V()->drapeaux&DRAPEAU_DEFAUT), "encodeur muet en marche : DEFAUT, code 4, moteur coupe");
  ev(EV_COMMUT,SW_OBSTACLE); VERIF(V()->etat==ETAT_DEFAUT, "un obstacle ne masque pas le defaut (I8)");
  ev(EV_COMMUT,SW_OBSTACLE);
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_DEFAUT, "Fn refuse : defaut encore present (injection active)");
  ev(EV_DEFAUT, (EV_DEFAUT_CODE_BASE + DEFAUT_ENCODEUR) | DEFAUT_RETABLI);
  VERIF(V()->etat==ETAT_DEFAUT, "defaut disparu : on reste en DEFAUT jusqu'a Fn");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET, "Fn -> PRET, aucun redemarrage automatique");

  /* E19 : protection thermique */
  ev(EV_THERMIQUE, SURV_THERM_LIMITATION); VERIF(fsm_plafond_pct()==LIMITATION_PCT && V()->etat==ETAT_ARRET, "40 C : limitation, pas d'arret");
  ev(EV_THERMIQUE, SURV_THERM_ARRET);
  VERIF(V()->etat==ETAT_ARRET_EVENEMENT && V()->cause==CAUSE_THERMIQUE && (V()->drapeaux&DRAPEAU_THERMIQUE), "50 C : ARRET_EVENEMENT, cause THERMIQUE");
  bips=0; ticks(1000); VERIF(bips==2, "bips repetes en arret thermique");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET_EVENEMENT, "Fn refuse au-dessus de 45 C");
  ev(EV_THERMIQUE, SURV_THERM_NORMAL); ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET, "retour a 25 C + Fn -> PRET");

  /* E-Stop et priorites */
  ev(EV_USER,0);
  estop_enfonce=1; ev(EV_ESTOP,0);
  VERIF(V()->etat==ETAT_ARRET_URGENCE && ccr1==0 && buzzer_continu==1, "E-Stop en marche : URGENCE, son continu");
  ev(EV_DEFAUT, I2C_ADR_LCD); VERIF(V()->etat==ETAT_ARRET_URGENCE, "le defaut ne remplace pas l'urgence (I7)");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET_URGENCE, "Fn refuse : E-Stop enfonce");
  estop_enfonce=0; ev(EV_TOUCHE,TOUCHE_FN);
  VERIF(V()->etat==ETAT_DEFAUT && V()->code_defaut==DEFAUT_I2C && buzzer_continu==0, "Fn : URGENCE -> PRET -> DEFAUT (stabilisation, module muet)");
  ev(EV_DEFAUT, I2C_ADR_LCD | DEFAUT_RETABLI); ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET, "module retabli + Fn -> PRET");

  /* E12 : TouchPad muet, Touch En acquitte */
  ev(EV_DEFAUT, I2C_ADR_TOUCHPAD|DEFAUT_TP); VERIF(V()->etat==ETAT_DEFAUT, "TouchPad muet : DEFAUT");
  ev(EV_DEFAUT, I2C_ADR_TOUCHPAD|DEFAUT_TP|DEFAUT_RETABLI); ev(EV_TOUCH,0);
  VERIF(V()->etat==ETAT_ARRET && touch_enabled==1, "TouchPad retabli : Touch En acquitte (sans basculer la garde)");

  /* E20 : mode essai signale */
  (void)essai_commande('m'); VERIF(V()->mode_essai==1 && (V()->drapeaux&DRAPEAU_ESSAI), "mode essai : colonne mode_essai et drapeau");
  (void)essai_commande('m'); VERIF(V()->mode_essai==0, "sortie du mode essai");

  /* E22 : echec de INIT, puis E-Stop pendant INIT */
  fsm_init(); fsm_fin_init(DEFAUT_INIT);
  VERIF(V()->etat==ETAT_DEFAUT && V()->code_defaut==DEFAUT_INIT, "ressource absente : INIT -> DEFAUT, code 1");
  ev(EV_TOUCHE,TOUCHE_FN); VERIF(V()->etat==ETAT_ARRET && V()->code_defaut==0, "Fn : DEFAUT d'initialisation acquitte");
  fsm_init(); estop_enfonce=1; ev(EV_ESTOP,0); VERIF(V()->etat==ETAT_ARRET_URGENCE, "E-Stop pendant INIT : URGENCE (I4)");
  fsm_fin_init(DEFAUT_AUCUN); VERIF(V()->etat==ETAT_ARRET_URGENCE, "fin de INIT sans effet en URGENCE");
  estop_enfonce=0;

  printf("test_fsm : %d verifications, %d echec(s) : %s\n", n, echecs, echecs ? "ECHEC" : "OK");
  return echecs != 0;
}
