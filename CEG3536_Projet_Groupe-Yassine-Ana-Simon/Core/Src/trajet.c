/* ---------------------------------------------------------------------------
 * trajet.c — suivi d'un trajet défini en distance
 *            (CEG 3536, projet final, E16) — complété par l'équipe
 * Module sans matériel, testé sur hôte : make trajet dans hote/.
 *
 * Représentation : la distance parcourue est gardée en FRONTS d'encodeur
 * (entier de 32 bits, 105 tours = 78 540 fronts), plafonnée à la distance
 * totale. Les dixièmes de tour ne sont calculés qu'à la lecture :
 * dist_x10 = fronts x 10 / 748 (troncature ; aucun débordement avant
 * 429 millions de fronts). Ainsi 748 appels d'un front valent exactement un
 * tour : rien n'est perdu par arrondi.
 * Segment courant : le premier dont la borne de fin (somme cumulée des
 * longueurs, en fronts) DÉPASSE la distance parcourue. Aucune notion de temps.
 * ------------------------------------------------------------------------- */
#include "trajet.h"

/* Trajet de référence de l'énoncé (section 4.8) : 105 tours — FOURNI        */
const segment_t TRAJET_REF[TRAJET_REF_N] = {
    { 20u, 150u },      /* S1 : démarrage et établissement de la vitesse     */
    { 40u, 250u },      /* S2 : accélération ; arrêt au feu rouge et reprise  */
    { 15u, 100u },      /* S3 : ralentissement marqué                        */
    { 30u, 200u },      /* S4 : nouvelle accélération, puis fin de trajet    */
};

/* Second trajet de l'équipe (énoncé 4.8) : 25 tours, environ 10 s. Il met à
 * l'épreuve les deux bornes de la plage de consigne (80 et 300 tr/min) et un
 * segment court, franchi en quelques fenêtres de 100 ms.                    */
const segment_t TRAJET_EQUIPE[TRAJET_EQUIPE_N] = {
    { 8u,  120u },
    { 10u, 300u },
    { 2u,   80u },
    { 5u,  200u },
};

static const segment_t *segments;   /* trajet courant                        */
static uint32_t nb_segments;
static uint32_t fronts;             /* distance parcourue, en fronts         */
static uint32_t fronts_total;       /* distance totale, en fronts            */
static uint32_t tours_total;        /* distance totale, en tours             */

void trajet_init(const segment_t *s, uint32_t n)
{
    segments = s;
    nb_segments = (s != 0) ? n : 0u;
    tours_total = 0u;
    for (uint32_t i = 0u; i < nb_segments; i++) {
        tours_total += segments[i].tours;
    }
    fronts_total = tours_total * FRONTS_PAR_TOUR;
    trajet_reset();
}

void trajet_reset(void)
{
    fronts = 0u;
}

void trajet_avancer(uint32_t f)
{
    uint32_t reste = fronts_total - fronts;     /* fronts <= fronts_total     */

    fronts += (f > reste) ? reste : f;          /* plafonné, sans débordement */
}

bool trajet_fini(void)
{
    return fronts >= fronts_total;              /* vrai aussi pour n = 0      */
}

/* indice (0 .. n-1) du segment courant ; n-1 quand le trajet est fini */
static uint32_t indice(void)
{
    uint32_t borne = 0u;

    for (uint32_t i = 0u; i < nb_segments; i++) {
        borne += (uint32_t)segments[i].tours * FRONTS_PAR_TOUR;
        if (fronts < borne) {
            return i;
        }
    }
    return nb_segments - 1u;
}

uint32_t trajet_consigne_rpm(void)
{
    if (trajet_fini()) {
        return 0u;
    }
    return segments[indice()].consigne_rpm;
}

uint32_t trajet_segment(void)
{
    if (nb_segments == 0u) {
        return 0u;
    }
    return indice() + 1u;
}

uint32_t trajet_nb_segments(void)
{
    return nb_segments;
}

uint32_t trajet_dist_x10(void)
{
    return fronts * 10u / FRONTS_PAR_TOUR;
}

uint32_t trajet_total_tours(void)
{
    return tours_total;
}
