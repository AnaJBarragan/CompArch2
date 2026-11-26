/* ---------------------------------------------------------------------------
 * rejeu.h — rejeu d'une trace de télémétrie dans les modules sans matériel
 *           (CEG 3536, projet final, composante L5-C) — ajout de l'équipe
 * Module SANS matériel : compilé sur Windows, Linux (hote/test_rejeu.c, qui
 * lit le CSV de telemetrie.py --extraire) et sur la cible (commande série j,
 * données compilées dans rejeu_donnees.h par hote/csv_vers_c.py).
 *
 * Chaque trame enregistrée fournit les ENTRÉES (état, sens, commande, mode,
 * plafond, température, vitesse) ; le rejeu recalcule les DÉCISIONS des
 * modules trajet, regul et surv, et les écrit sous forme de lignes de texte
 * identiques sur les trois environnements :
 *     k,ticks_ms,segment,consigne_rpm,commande_pct,code_defaut,niveau_therm
 * Une empreinte FNV-1a 32 bits de ces lignes résume le rejeu : deux
 * environnements qui prennent les mêmes décisions donnent la même empreinte.
 * ------------------------------------------------------------------------- */
#ifndef REJEU_H
#define REJEU_H

#include <stdint.h>
#include "projet.h"

typedef struct {
    uint32_t ticks_ms;
    uint16_t vitesse_rpm_x100;  /* colonne vitesse_rpm_x100 (au plus 40 000)  */
    int16_t  temp_c_x10;        /* colonne temp_c_x10                         */
    uint8_t  etat;              /* colonne etat (0 à 6)                       */
    uint8_t  sens;              /* colonne sens                               */
    uint8_t  pwm_pct;           /* colonne pwm_pct                            */
    uint8_t  plafond_pct;       /* colonne plafond_pct                        */
    uint8_t  mode;              /* colonne mode                               */
    uint8_t  segment;           /* colonne segment (pour la concordance)      */
} rejeu_trame_t;

typedef struct {
    uint32_t n;                 /* trames rejouées                            */
    uint32_t somme;             /* empreinte FNV-1a des lignes de décision    */
    uint32_t concordants;       /* trames où le segment recalculé = enregistré */
    uint32_t segment_final;
    uint32_t dist_x10_final;
    uint32_t defauts;           /* trames avec un code de surveillance != 0   */
} rejeu_bilan_t;

/* Reçoit chaque ligne de décision (terminée par '\n'), ou 0 pour n'écrire
 * aucune ligne (cible : seul le bilan est émis).                            */
typedef void (*rejeu_sortie_t)(const char *ligne);

/* Rejoue n trames sur le trajet s (n_seg segments). ATTENTION sur la cible :
 * trajet et surv sont des modules à état unique ; l'appelant les réinitialise
 * après le rejeu.                                                           */
void rejeu_executer(const rejeu_trame_t *t, uint32_t n, const segment_t *s, uint32_t n_seg,
                    rejeu_sortie_t sortie, rejeu_bilan_t *bilan);

#endif /* REJEU_H */
