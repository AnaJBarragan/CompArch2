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

    /* ---- cas de l'équipe ---- */
    trajet_init(TRAJET_REF, TRAJET_REF_N);
    trajet_avancer(30u * TOUR);                 /* arrêt au feu rouge pendant S2 */
    for (unsigned i = 0u; i < 50u; i++) {
        trajet_avancer(0u);                     /* véhicule arrêté : 5 s sans front */
    }
    trajet_avancer(5u * TOUR);                  /* reprise après l'acquittement  */
    VERIFIER(trajet_segment() == 2u && trajet_dist_x10() == 350u,
             "reprise apres un arret : la distance repart de la position atteinte");

    trajet_avancer(25u * TOUR);
    VERIFIER(trajet_segment() == 3u && trajet_consigne_rpm() == 100u && trajet_dist_x10() == 600u,
             "60 tours : segment 3 (100 tr/min), limite S2/S3 de T13");
    trajet_avancer(15u * TOUR);
    VERIFIER(trajet_segment() == 4u && trajet_dist_x10() == 750u, "75 tours : segment 4, limite S3/S4 de T13");

    {
        static const segment_t seul[1] = { { 4u, 180u } };
        trajet_init(seul, 1u);
        trajet_avancer(4u * TOUR - 1u);
        VERIFIER(trajet_nb_segments() == 1u && trajet_segment() == 1u && !trajet_fini()
                 && trajet_consigne_rpm() == 180u, "trajet d'un seul segment : 1/1 jusqu'a la fin");
        trajet_avancer(0xFFFFFFFFu);
        VERIFIER(trajet_fini() && trajet_dist_x10() == 40u,
                 "avance enorme (2^32 - 1 fronts) : plafonnee, sans debordement");
    }

    trajet_init(TRAJET_EQUIPE, TRAJET_EQUIPE_N);
    VERIFIER(trajet_total_tours() == 25u && trajet_consigne_rpm() == 120u,
             "trajet de l'equipe : 25 tours, depart a 120 tr/min");
    trajet_avancer(19u * TOUR);
    VERIFIER(trajet_segment() == 3u && trajet_consigne_rpm() == 80u,
             "trajet de l'equipe : segment court de 2 tours a 80 tr/min");

    trajet_init(court, 0u);
    VERIFIER(trajet_fini() && trajet_segment() == 0u && trajet_consigne_rpm() == 0u
             && trajet_total_tours() == 0u, "trajet vide (n = 0) : fini d'emblee, aucune consigne");

    return banc_bilan("test_trajet");
}
