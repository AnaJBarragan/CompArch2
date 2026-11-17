/* ---------------------------------------------------------------------------
 * lab4.h — interface du laboratoire 4 (CEG 3536, automne 2026)
 * Routines assembleur nouvelles, adresses du bus I2C, constantes et types
 * communs de la couche C. Inclut lab3.h (et donc lab2.h et lab1.h) : toute la
 * couche des laboratoires 1 à 3 reste appelable.
 *
 * IMPORTANT : etat_vehicule_t (lab3.h) doit recevoir quatre champs de plus,
 * ajoutés À LA FIN de la structure (voir LISEZMOI.md, « Modifications ») :
 *     uint32_t mode;          MODE_x
 *     uint32_t cause;         CAUSE_x
 *     uint32_t plafond_pct;   vitesse maximale fixée au TouchPad (10..100)
 *     uint32_t drapeaux;      DRAPEAU_x (événements actifs, pour les DEL)
 * ------------------------------------------------------------------------- */
#ifndef LAB4_H
#define LAB4_H

#include <stdint.h>
#include <stdbool.h>
#include "lab3.h"

/* ---- Routines assembleur du laboratoire 4 ------------------------------ */
/* somme.s : OU exclusif des n octets (0 si n = 0). Routine feuille (E10).   */
uint32_t trame_xor8(const uint8_t *octets, uint32_t n);
/* exti_l4.s : configuration des lignes EXTI9 (PIR, PB9) et EXTI6 (THI, PB6)
 * et gestionnaires. Les gestionnaires ne font que lever un drapeau (E6, E4). */
void exti_l4_init(void);

extern volatile uint32_t pir_flag;      /* 1 = mouvement détecté, à consommer  */
extern volatile uint32_t pir_t_ms;      /* ticks_ms au moment du front retenu  */
extern volatile uint32_t thi_flag;      /* 1 = demande du TouchPad, à consommer */

/* ---- Bus I2C : adresses sur 7 bits (énoncé, section 2) ------------------ */
#define I2C_ADR_RVB         0x2Du   /* rétroéclairage du LCD (octets 0x5A / 0x5B) */
#define I2C_ADR_DEL         0x38u   /* extenseur des DEL (octets 0x70 / 0x71)     */
#define I2C_ADR_COMMUT      0x39u   /* extenseur des commutateurs (octet 0x73)    */
#define I2C_ADR_LCD         0x3Eu   /* pilote des caractères (octets 0x7C / 0x7D) */
#define I2C_ADR_TOUCHPAD    0x5Au   /* contrôleur tactile (octets 0xB4 / 0xB5)    */

/* ---- Constantes du laboratoire 4 --------------------------------------- */
#define I2C_DELAI_MS            2u      /* échéance de chaque attente (E1)       */
#define I2C_ERREURS_DEFAUT      3u      /* erreurs consécutives -> cause DÉFAUT  */
#define PIR_FILTRE_MS           50u     /* fronts PIR rapprochés ignorés (E6)    */
#define COMMUT_PERIODE_MS       10u     /* scrutation des commutateurs (E2)      */
#define COMMUT_ANTIREBOND_MS    30u     /* fenêtre exigée : 20 à 50 ms           */
#define ENV_PERIODE_MS          1000u   /* une mesure par seconde (E7)           */
#define CONSIGNE_AUTO_PCT       60u     /* consigne demandée en mode AUTONOME    */
#define LIMITATION_PCT          30u     /* plafond de la limitation de vitesse   */
#define HUM_SEUIL_PCT           70u     /* seuil d'humidité de la limitation     */
#define HUM_HYSTERESIS_PCT      5u
#define VERROU_MS               1000u   /* durée du message « VERROU » (E4)      */
#define BIP_OBSTACLE_MS         500u    /* un bip toutes les 500 ms sur obstacle */
#define LCD_DEMARRAGE_MS        50u     /* délai après la mise sous tension      */
#define ABSENCE_SIMULEE_MS      2000u   /* durée de la commande série « d » (E12) */

/* ---- État, mode, cause, drapeaux --------------------------------------- */
#define ETAT_ARRET_EVENEMENT    4u      /* suit ETAT_ARRET_URGENCE (3) de lab1.h */

#define MODE_MANUEL             0u
#define MODE_AUTONOME           1u

/* causes, par priorité croissante (énoncé, section 4.6) */
#define CAUSE_AUCUNE            0u
#define CAUSE_FIN               1u      /* fin de trajet : affichée en ARRÊT     */
#define CAUSE_FEU               2u
#define CAUSE_OBSTACLE          3u
#define CAUSE_DEFAUT            4u

/* drapeaux : événements actifs, indépendamment de l'état courant */
#define DRAPEAU_LIMITATION      (1u << 0)   /* SW3 ou humidité          -> Y1 */
#define DRAPEAU_TOUCH           (1u << 1)   /* TouchPad autorisé        -> Y3 */
#define DRAPEAU_FIN             (1u << 2)   /* fin de trajet (SW4)      -> Y4 */
#define DRAPEAU_FEU             (1u << 3)   /* feu rouge (SW1)          -> R1 */
#define DRAPEAU_OBSTACLE        (1u << 4)   /* PIR haut ou SW2          -> R2 */
#define DRAPEAU_DEFAUT          (1u << 5)   /* SW5 ou module muet       -> R3 */

/* image logique des huit DEL (bit à 1 = DEL allumée ; l'extenseur reçoit le
 * complément : un niveau bas allume la DEL). P0..P3 = Y1..Y4, P4..P7 = R1..R4. */
#define DEL_Y1  (1u << 0)   /* limitation active        */
#define DEL_Y2  (1u << 1)   /* mode AUTONOME            */
#define DEL_Y3  (1u << 2)   /* TouchPad autorisé        */
#define DEL_Y4  (1u << 3)   /* fin de trajet            */
#define DEL_R1  (1u << 4)   /* feu rouge                */
#define DEL_R2  (1u << 5)   /* obstacle (PIR ou simulé) */
#define DEL_R3  (1u << 6)   /* défaut                   */
#define DEL_R4  (1u << 7)   /* arrêt d'urgence          */

/* commutateurs (valeur de EV_COMMUT : 1 à 8) */
#define SW_FEU          1u
#define SW_OBSTACLE     2u
#define SW_LIMITATION   3u
#define SW_FIN          4u
#define SW_DEFAUT       5u
#define SW_PAGE         8u

/* touches (valeur de EV_TOUCHE) : 0 à 9 = chiffres */
#define TOUCHE_FN       10
#define TOUCHE_NEXT     11

/* pages du LCD */
#define PAGE_CONDUITE   0u
#define PAGE_CAPTEURS   1u
#define PAGE_VERROU     2u      /* page de conduite avec « VERROU » en ligne 1 */

/* ---- Mesure environnementale exposée au tableau de bord ---------------- */
typedef struct {
    int32_t  t_c10;         /* température du capteur environnemental, 0,1 °C  */
    uint32_t h_x10;         /* humidité relative, 0,1 %                        */
    uint32_t p_hpa_x10;     /* pression, 0,1 hPa                               */
    int32_t  ta_c10;        /* température analogique (acq_temp, lab. 3), 0,1 °C :
                               recopiée par main.c, pas par env.c              */
    uint32_t valide;        /* 0 tant qu'aucune mesure n'a abouti              */
} env_mesure_t;

/* ---- Composante L4-A : variante attribuée à l'équipe (0 à 3) ------------ */
#ifndef L4A_VARIANTE
#define L4A_VARIANTE 0
#endif

#endif /* LAB4_H */
