/* ---------------------------------------------------------------------------
 * banc.h — petit banc d'essai commun aux programmes hôte de la partie II
 *          (CEG 3536, projet final) — FOURNI
 * VERIFIER(condition, "texte") compte un cas et affiche son résultat ;
 * banc_bilan("nom") affiche le total et retourne le code de sortie du main.
 * ------------------------------------------------------------------------- */
#ifndef BANC_H
#define BANC_H

#include <stdio.h>
#include <limits.h>

static unsigned banc_cas, banc_echecs;

#define VERIFIER(condition, texte) do {                                     \
        banc_cas++;                                                         \
        if (condition) {                                                    \
            printf("  cas %2u : reussi  %s\n", banc_cas, texte);            \
        } else {                                                            \
            banc_echecs++;                                                  \
            printf("  cas %2u : ECHEC   %s\n", banc_cas, texte);            \
        }                                                                   \
    } while (0)

static int banc_bilan(const char *nom)
{
    printf("%s : %u cas, %u echec(s) : %s  (int = %u bits, long = %u bits, pointeur = %u bits)\n",
           nom, banc_cas, banc_echecs, banc_echecs ? "ECHEC" : "OK",
           (unsigned)(sizeof(int) * CHAR_BIT), (unsigned)(sizeof(long) * CHAR_BIT),
           (unsigned)(sizeof(void *) * CHAR_BIT));
    return banc_echecs ? 1 : 0;
}

#endif /* BANC_H */
