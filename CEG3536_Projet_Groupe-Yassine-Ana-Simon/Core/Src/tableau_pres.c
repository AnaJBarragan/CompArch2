/* ---------------------------------------------------------------------------
 * tableau_pres.c — présentation du tableau de bord (CEG 3536, laboratoire 4,
 *                  E3, E4, E8 ; partie II, E14, E16, E19) — complété par l'équipe
 * Relie l'état du véhicule (fsm_etat) au LCD, au rétroéclairage et aux huit
 * DEL. Ne lance AUCUNE transaction : lcd_ligne et lcd_couleur ne font que
 * mémoriser la demande, et l'image des DEL est remise au répartiteur du bus
 * de main.c par tableau_image_del (énoncé 4.7, « Répartition des
 * transactions »).
 * Séparé de tableau.c pour que le formatage reste compilable sur hôte.
 * Partie II : troisième page (trajet), message « FILE » pendant 1 s après un
 * débordement de la file, R3 clignotante en arrêt thermique, rétroéclairage
 * clignotant aussi en DÉFAUT.
 * ------------------------------------------------------------------------- */
#include "projet.h"
#include "fsm.h"
#include "lcd.h"
#include "evq.h"
#include "trajet.h"
#include "tableau.h"

#define RVB_FORT    0xFFu
#define RVB_ETEINT  0x00u

static uint32_t     page;           /* PAGE_CONDUITE, PAGE_CAPTEURS, PAGE_TRAJET */
static uint32_t     verrou_actif, verrou_debut;
static uint32_t     file_actif, file_debut, debordements_vus;
static env_mesure_t mesure;
static uint8_t      image_del;

void tableau_init(void)
{
    page = PAGE_CONDUITE;
    verrou_actif = 0u;
    file_actif = 0u;
    debordements_vus = evq_debordements();
    mesure.valide = 0u;
    mesure.ta_c10 = 0;
    image_del = 0u;
}

/* SW8 : conduite -> capteurs -> trajet -> conduite (trois pages, partie II) */
void tableau_page_basculer(void)
{
    if (page == PAGE_CONDUITE) {
        page = PAGE_CAPTEURS;
    } else if (page == PAGE_CAPTEURS) {
        page = PAGE_TRAJET;
    } else {
        page = PAGE_CONDUITE;
    }
}

/* [FOURNI] */
void tableau_verrou(void)
{
    verrou_actif = 1u;
    verrou_debut = ticks_ms;
}

/* [FOURNI] */
void tableau_env(const env_mesure_t *m)
{
    mesure = *m;
}

/* [FOURNI] */
uint8_t tableau_image_del(void)
{
    return image_del;
}

void tableau_step(void)
{
    etat_vehicule_t v = *fsm_etat();        /* copie : champs de présentation */
    char l1[17], l2[17];
    uint32_t d = evq_debordements();
    uint32_t phase_eteinte = ((ticks_ms / CLIGNOTEMENT_DEMI_MS) & 1u) != 0u;   /* 2 Hz */

    /* [FOURNI] fin du message « VERROU » */
    if (verrou_actif && (ticks_ms - verrou_debut) >= VERROU_MS) {
        verrou_actif = 0u;
    }
    /* message « FILE » (E14) : un nouveau débordement le relance pour 1 s.
     * « > » et non « != » : la commande z remet le compteur à zéro.        */
    if (d > debordements_vus) {
        file_actif = 1u;
        file_debut = ticks_ms;
    }
    debordements_vus = d;
    if (file_actif && (ticks_ms - file_debut) >= FILE_MESSAGE_MS) {
        file_actif = 0u;
    }
    if (file_actif) {
        v.drapeaux |= DRAPEAU_FILE;
    }
    v.nb_segments = trajet_nb_segments();   /* page de trajet (E16)           */
    v.total_tours = trajet_total_tours();

    /* [FOURNI] texte : formaté à chaque appel, mais lcd ne réécrit que les
     * caractères qui changent                                                 */
    tableau_formater(&v, &mesure, verrou_actif ? PAGE_VERROU : page, l1, l2);
    lcd_ligne(0u, l1);
    lcd_ligne(1u, l2);

    /* DEL : R3 clignote à 2 Hz en arrêt thermique, sauf si un défaut la tient
     * allumée (E19)                                                          */
    image_del = tableau_del(&v);
    if ((v.drapeaux & DRAPEAU_THERMIQUE) && !(v.drapeaux & DRAPEAU_DEFAUT) && phase_eteinte) {
        image_del &= (uint8_t)~DEL_R3;
    }

    /* Rétroéclairage (E3) : couleur de la DEL active de la Nucleo (vert en
     * MARCHE_AVANT, bleu en MARCHE_ARRIÈRE, rouge sinon) ; clignotement à
     * 2 Hz en ARRÊT_URGENCE, en ARRÊT_ÉVÉNEMENT et en DÉFAUT (section 4.6).
     * lcd_couleur ne fait que mémoriser : lcd_step n'envoie que les
     * composantes qui changent.                                              */
    {
        uint32_t clignote = (v.etat == ETAT_ARRET_URGENCE || v.etat == ETAT_ARRET_EVENEMENT
                             || v.etat == ETAT_DEFAUT);
        uint8_t  fort = (clignote && phase_eteinte) ? RVB_ETEINT : RVB_FORT;

        if (v.etat == ETAT_MARCHE_AVANT) {
            lcd_couleur(RVB_ETEINT, fort, RVB_ETEINT);
        } else if (v.etat == ETAT_MARCHE_ARRIERE) {
            lcd_couleur(RVB_ETEINT, RVB_ETEINT, fort);
        } else {
            lcd_couleur(fort, RVB_ETEINT, RVB_ETEINT);
        }
    }
}
