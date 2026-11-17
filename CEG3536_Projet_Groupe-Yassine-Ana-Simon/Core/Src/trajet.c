/* ---------------------------------------------------------------------------
 * trajet.c — suivi d'un trajet défini en distance
 *            (CEG 3536, projet final, E16) — SQUELETTE À COMPLÉTER
 * Module sans matériel, testé sur hôte : make trajet dans hote/.
 *
 * Indications :
 *  - conserver la distance en FRONTS (entier de 32 bits : 105 tours valent
 *    78 540 fronts), et n'en déduire les dixièmes de tour qu'à l'affichage :
 *    dist_x10 = fronts x 10 / 748 ;
 *  - calculer une fois, dans trajet_init, la distance totale et, si l'on
 *    veut, la borne de fin de chaque segment en fronts ;
 *  - le segment courant est le premier dont la borne de fin dépasse la
 *    distance parcourue ; aucune notion de temps n'intervient.
 * ------------------------------------------------------------------------- */
#include "trajet.h"

/* Trajet de référence de l'énoncé (section 4.8) : 105 tours — FOURNI        */
const segment_t TRAJET_REF[TRAJET_REF_N] = {
    { 20u, 150u },      /* S1 : démarrage et établissement de la vitesse     */
    { 40u, 250u },      /* S2 : accélération ; arrêt au feu rouge et reprise  */
    { 15u, 100u },      /* S3 : ralentissement marqué                        */
    { 30u, 200u },      /* S4 : nouvelle accélération, puis fin de trajet    */
};

static const segment_t *segments;   /* trajet courant                        */
static uint32_t nb_segments;
/* À COMPLÉTER : distance parcourue en fronts, distance totale, ...          */

void trajet_init(const segment_t *s, uint32_t n)
{
    segments = s;
    nb_segments = n;
    /* À COMPLÉTER */
    trajet_reset();
}

void trajet_reset(void)
{
    /* À COMPLÉTER */
}

void trajet_avancer(uint32_t fronts)
{
    /* À COMPLÉTER */
    (void)fronts;
}

uint32_t trajet_consigne_rpm(void)
{
    /* À COMPLÉTER */
    return 0u;
}

uint32_t trajet_segment(void)
{
    /* À COMPLÉTER */
    return 0u;
}

uint32_t trajet_nb_segments(void)
{
    return nb_segments;
}

uint32_t trajet_dist_x10(void)
{
    /* À COMPLÉTER */
    return 0u;
}

uint32_t trajet_total_tours(void)
{
    /* À COMPLÉTER */
    (void)segments;
    return 0u;
}

bool trajet_fini(void)
{
    /* À COMPLÉTER */
    return false;
}
