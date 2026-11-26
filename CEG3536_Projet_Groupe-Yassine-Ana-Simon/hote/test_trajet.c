/* ---------------------------------------------------------------------------
 * test_trajet.c — essai hôte du suivi de trajet (CEG 3536, projet final, E16)
 *                 FOURNI ; cas à ajouter par l'équipe
 * Compile sur Windows (MinGW/MSYS2) et Linux : make trajet dans ce dossier, ou
 *   gcc -std=gnu11 -Wall -Wextra -I../Core/Inc -o test_trajet test_trajet.c \
 *       ../Core/Src/trajet.c
 * Le même trajet.c est compilé dans le projet STM32.
 * ------------------------------------------------------------------------- */
#include "banc.h"
#include "trajet.h"

#define TOUR FRONTS_PAR_TOUR

int main(void)
{
    static const segment_t court[2] = { { 3u, 90u }, { 2u, 120u } };

    printf("trajet : trajet de reference, %u segments\n", (unsigned)TRAJET_REF_N);
    trajet_init(TRAJET_REF, TRAJET_REF_N);
    VERIFIER(trajet_segment() == 1u && trajet_consigne_rpm() == 150u && trajet_dist_x10() == 0u
             && !trajet_fini(), "depart : segment 1, 150 tr/min, distance nulle");
    VERIFIER(trajet_total_tours() == 105u && trajet_nb_segments() == 4u, "total de 105 tours, 4 segments");

    trajet_avancer(10u * TOUR);
    VERIFIER(trajet_dist_x10() == 100u && trajet_segment() == 1u, "10 tours : 100 dixiemes, segment 1");

    trajet_avancer(10u * TOUR - 1u);
    VERIFIER(trajet_segment() == 1u && trajet_dist_x10() == 199u, "un front avant 20 tours : encore le segment 1");

    trajet_avancer(1u);
    VERIFIER(trajet_segment() == 2u && trajet_consigne_rpm() == 250u && trajet_dist_x10() == 200u,
             "20 tours : segment 2, 250 tr/min");

    for (unsigned i = 0u; i < 1000u; i++) {
        trajet_avancer(0u);
    }
    VERIFIER(trajet_segment() == 2u && trajet_dist_x10() == 200u, "1000 appels sans front : rien ne change (pas de temps)");

    for (unsigned i = 0u; i < TOUR; i++) {
        trajet_avancer(1u);
    }
    VERIFIER(trajet_dist_x10() == 210u, "748 appels d'un front : un tour, sans perte");

    trajet_avancer(60u * TOUR);
    VERIFIER(trajet_segment() == 4u && trajet_consigne_rpm() == 200u && trajet_dist_x10() == 810u,
             "un seul appel franchit deux limites : segment 4");

    trajet_avancer(24u * TOUR - 1u);
    VERIFIER(!trajet_fini() && trajet_dist_x10() == 1049u, "un front avant 105 tours : trajet non fini");

    trajet_avancer(1u);
    VERIFIER(trajet_fini() && trajet_consigne_rpm() == 0u && trajet_segment() == 4u
             && trajet_dist_x10() == 1050u, "105 tours : fini, consigne nulle");

    trajet_avancer(5u * TOUR);
    VERIFIER(trajet_fini() && trajet_dist_x10() == 1050u, "apres la fin : distance plafonnee a 105 tours");

    trajet_reset();
    VERIFIER(!trajet_fini() && trajet_segment() == 1u && trajet_dist_x10() == 0u
             && trajet_consigne_rpm() == 150u, "remise a zero : retour au depart");

    trajet_init(court, 2u);
    trajet_avancer(3u * TOUR);
    VERIFIER(trajet_total_tours() == 5u && trajet_segment() == 2u && trajet_consigne_rpm() == 120u,
             "second trajet (3 + 2 tours) : segment 2 apres 3 tours");

    /* À COMPLÉTER : au moins deux cas de l'équipe (trajet de l'équipe, reprise
     * après un arrêt, trajet d'un seul segment, ...)                         */

    return banc_bilan("test_trajet");
}
