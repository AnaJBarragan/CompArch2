/* ---------------------------------------------------------------------------
 * touchpad.c — contrôleur tactile (CEG 3536, laboratoire 4, E4)
 *              — SQUELETTE À COMPLÉTER
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

/* Table électrode -> touche : À RELEVER (toucher chaque touche et noter le bit
 * qui monte dans l'état lu ; consigner la table dans JOURNAL.md).
 * Valeurs : 0 à 9, TOUCHE_FN, TOUCHE_NEXT ; -1 = électrode sans touche connue. */
const int8_t tp_table[TP_N_ELECTRODES] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

bool tp_init(void)
{
    /* À COMPLÉTER : étapes 1 à 5 ; retourner false dès qu'une écriture échoue */
    return false;
}

bool tp_lire(uint16_t *etat12)
{
    /* À COMPLÉTER */
    (void)etat12;
    return false;
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
