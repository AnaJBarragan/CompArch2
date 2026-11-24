/* ---------------------------------------------------------------------------
 * touchpad.c — contrôleur tactile (CEG 3536, laboratoire 4, E4)
 *              — complété par l'équipe
 *
 * Repères (énoncé, 4.7 ; registres du contrôleur MPR121, À CONFIRMER dans la
 * fiche technique liée en annexe D du guide) :
 *   1. réinitialisation logicielle : 0x80 <- 0x63 ;
 *   2. mode arrêt : 0x5E <- 0x00 (obligatoire avant toute configuration) ;
 *   3. filtres de la ligne de base : registres 0x2B à 0x40 (valeurs de la note
 *      d'application) ;
 *   4. seuils de l'électrode i : toucher 0x41 + 2i, relâchement 0x42 + 2i
 *      (valeurs de départ 12 et 6) ;
 *   5. mise en service des 12 électrodes : 0x5E <- 0x8C, EN DERNIER.
 * Chaque écriture de registre est une transaction de deux octets de données
 * {registre, valeur}. tp_init est appelée avant la boucle principale : elle
 * peut enchaîner ses transactions (une quarantaine, environ 15 ms).
 * État des touches : registres 0x00 (électrodes 0 à 7) et 0x01 (électrodes 8
 * à 11), lus en UNE transaction (i2c_ecrire_lire) ; ne garder que 12 bits.
 * Vérification au débogueur : relire 0x5E et deux seuils après tp_init.
 *
 * Si le gestionnaire EXTI6 est écrit en C, il se place ici :
 *     void EXTI6_IRQHandler(void) { thi_flag = 1u; <effacer RPR1 et FPR1> }
 * (il n'est alors pas défini dans exti_l4.s).
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "i2c.h"
#include "touchpad.h"

/* Registres du contrôleur (MPR121, fiche technique et note AN3944) */
#define TP_REG_ETAT         0x00u   /* 0x00 : électrodes 0-7 ; 0x01 : 8-11   */
#define TP_REG_FILTRES      0x2Bu   /* 0x2B à 0x35 : filtres de la ligne de base */
#define TP_REG_SEUILS       0x41u   /* toucher 0x41 + 2i, relâchement 0x42 + 2i */
#define TP_REG_REBOND       0x5Bu
#define TP_REG_CONFIG1      0x5Cu   /* FFI, courant de charge (CDC)           */
#define TP_REG_CONFIG2      0x5Du   /* temps de charge (CDT), SFI, ESI        */
#define TP_REG_ECR          0x5Eu   /* configuration des électrodes (marche)  */
#define TP_REG_RAZ          0x80u
#define TP_SEUIL_TOUCHER    12u
#define TP_SEUIL_RELACHE    6u

/* Table électrode -> touche, RELEVÉE sur la carte le 17 nov. 2026 : avec une
 * table identité, les touches 1 à 9, 0, Shift (Fn) et Next/Back, touchées
 * dans cet ordre, ont donné les codes 1 à 9, 0, 10 et 11 dans les lignes
 * « E,…,8,<code> ». L'électrode i correspond donc à la touche i ; l'électrode
 * 10 est la touche Shift, qui joue le rôle de Fn (acquittement).
 * Valeurs : 0 à 9, TOUCHE_FN, TOUCHE_NEXT ; -1 = électrode sans touche.      */
const int8_t tp_table[TP_N_ELECTRODES] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, TOUCHE_FN, TOUCHE_NEXT
};

/* Filtres de la ligne de base, 0x2B à 0x35 (valeurs de la note AN3944) :
 * MHD, NHD, NCL, FDL pour les variations montantes, descendantes, puis
 * pendant un toucher.                                                        */
static const uint8_t tp_filtres[] = {
    0x01u, 0x01u, 0x00u, 0x00u,     /* 0x2B à 0x2E : montée                 */
    0x01u, 0x01u, 0xFFu, 0x02u,     /* 0x2F à 0x32 : descente               */
    0x00u, 0x00u, 0x00u             /* 0x33 à 0x35 : électrode touchée      */
};

static bool reg_ecrire(uint8_t reg, uint8_t valeur)
{
    uint8_t d[2] = { reg, valeur };

    return i2c_ecrire(I2C_ADR_TOUCHPAD, d, 2u) == I2C_OK;
}

/* Appelée avant la boucle principale : enchaîne ses ~40 transactions. Les
 * registres de configuration ne sont acceptés qu'en mode arrêt (ECR = 0) :
 * la mise en service (ECR) vient EN DERNIER.                                */
bool tp_init(void)
{
    if (!reg_ecrire(TP_REG_RAZ, 0x63u)) {           /* 1. réinitialisation   */
        return false;
    }
    if (!reg_ecrire(TP_REG_ECR, 0x00u)) {           /* 2. mode arrêt         */
        return false;
    }
    for (uint32_t i = 0u; i < sizeof tp_filtres; i++) {          /* 3. filtres */
        if (!reg_ecrire((uint8_t)(TP_REG_FILTRES + i), tp_filtres[i])) {
            return false;
        }
    }
    for (uint32_t i = 0u; i < TP_N_ELECTRODES; i++) {            /* 4. seuils  */
        if (!reg_ecrire((uint8_t)(TP_REG_SEUILS + 2u * i), TP_SEUIL_TOUCHER)
            || !reg_ecrire((uint8_t)(TP_REG_SEUILS + 2u * i + 1u), TP_SEUIL_RELACHE)) {
            return false;
        }
    }
    if (!reg_ecrire(TP_REG_REBOND, 0x11u)              /* 1 échantillon        */
        || !reg_ecrire(TP_REG_CONFIG1, 0x10u)          /* FFI = 6, CDC = 16 µA */
        || !reg_ecrire(TP_REG_CONFIG2, 0x20u)) {       /* CDT = 0,5 µs, ESI 1 ms */
        return false;
    }
    /* 5. mise en service : CL = 10 (ligne de base initialisée par la première
     * mesure), ELE = 1100 (12 électrodes)                                   */
    return reg_ecrire(TP_REG_ECR, 0x8Cu);
}

/* État des 12 électrodes en UNE transaction : adresse + registre 0x00, puis
 * redémarrage et lecture de 0x00 et 0x01 (5 octets sur le bus avec les deux
 * adresses). Cette lecture retire la demande THI.                           */
bool tp_lire(uint16_t *etat12)
{
    uint8_t reg = TP_REG_ETAT;
    uint8_t d[2];

    if (i2c_ecrire_lire(I2C_ADR_TOUCHPAD, &reg, 1u, d, 2u) != I2C_OK) {
        return false;
    }
    *etat12 = (uint16_t)(((uint32_t)d[0] | ((uint32_t)d[1] << 8)) & 0x0FFFu);
    return true;
}

/* [FOURNI] un seul nouvel appui, et aucune autre électrode touchée */
int32_t tp_touche(uint16_t avant, uint16_t apres)
{
    uint32_t a = (uint32_t)apres & 0x0FFFu;
    uint32_t nouveaux = a & ~(uint32_t)avant;

    if (nouveaux == 0u || (a & (a - 1u)) != 0u) {
        return -1;                      /* rien de nouveau, ou plusieurs touches */
    }
    for (uint32_t i = 0u; i < TP_N_ELECTRODES; i++) {
        if (nouveaux == (1u << i)) {
            return tp_table[i];
        }
    }
    return -1;
}
