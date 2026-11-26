/* ---------------------------------------------------------------------------
 * tableau_pres.c — présentation du tableau de bord (CEG 3536, laboratoire 4,
 *                  E3, E4, E8) — complété par l'équipe
 * Relie l'état du véhicule (fsm_etat) au LCD, au rétroéclairage et aux huit
 * DEL. Ne lance AUCUNE transaction : lcd_ligne et lcd_couleur ne font que
 * mémoriser la demande, et l'image des DEL est remise au répartiteur du bus
 * de main.c par tableau_image_del (énoncé 4.7, « Répartition des
 * transactions »).
 * Séparé de tableau.c pour que le formatage reste compilable sur hôte.
 * ------------------------------------------------------------------------- */
#include "lab4.h"
#include "fsm.h"
#include "lcd.h"
#include "tableau.h"

#define RVB_FORT    0xFFu
#define RVB_ETEINT  0x00u

static uint32_t     page;           /* PAGE_CONDUITE ou PAGE_CAPTEURS          */
static uint32_t     verrou_actif, verrou_debut;
static env_mesure_t mesure;
static uint8_t      image_del;

/* [FOURNI] */
void tableau_init(void)
{
    page = PAGE_CONDUITE;
    verrou_actif = 0u;
    mesure.valide = 0u;
    mesure.ta_c10 = 0;
    image_del = 0u;
}

/* [FOURNI] */
void tableau_page_basculer(void)
{
    page = (page == PAGE_CONDUITE) ? PAGE_CAPTEURS : PAGE_CONDUITE;
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
    const etat_vehicule_t *v = fsm_etat();
    char l1[17], l2[17];

    /* [FOURNI] fin du message « VERROU » */
    if (verrou_actif && (ticks_ms - verrou_debut) >= VERROU_MS) {
        verrou_actif = 0u;
    }

    /* [FOURNI] texte : formaté à chaque tick, mais lcd ne réécrit que les
     * caractères qui changent                                                 */
    tableau_formater(v, &mesure, verrou_actif ? PAGE_VERROU : page, l1, l2);
    lcd_ligne(0u, l1);
    lcd_ligne(1u, l2);

    /* [FOURNI] DEL */
    image_del = tableau_del(v);

    /* Rétroéclairage (E3) : couleur de la DEL active de la Nucleo (même
     * correspondance que fsm_sorties : vert en MARCHE_AVANT, bleu en
     * MARCHE_ARRIÈRE, rouge sinon) ; clignotement à 2 Hz en ARRÊT_URGENCE et
     * en ARRÊT_ÉVÉNEMENT : allumé pendant les demi-périodes paires de
     * CLIGNOTEMENT_DEMI_MS, éteint pendant les impaires. lcd_couleur ne fait
     * que mémoriser : lcd_step n'envoie que les composantes qui changent.    */
    {
        uint32_t clignote = (v->etat == ETAT_ARRET_URGENCE || v->etat == ETAT_ARRET_EVENEMENT);
        uint32_t allume = !clignote || ((ticks_ms / CLIGNOTEMENT_DEMI_MS) & 1u) == 0u;
        uint8_t  fort = allume ? RVB_FORT : RVB_ETEINT;

        if (v->etat == ETAT_MARCHE_AVANT) {
            lcd_couleur(RVB_ETEINT, fort, RVB_ETEINT);
        } else if (v->etat == ETAT_MARCHE_ARRIERE) {
            lcd_couleur(RVB_ETEINT, RVB_ETEINT, fort);
        } else {
            lcd_couleur(fort, RVB_ETEINT, RVB_ETEINT);
        }
    }
}
