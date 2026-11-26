#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""csv_vers_c.py — traces CSV de telemetrie.py vers un en-tête C pour la cible
(CEG 3536, projet final, composante L5-C) — ajout de l'équipe

Usage (depuis hote/) :
  python csv_vers_c.py preuves/s3_autonome.csv preuves/s5_defauts.csv \\
         --sortie ../Core/Inc/rejeu_donnees.h

Écrit, pour chaque CSV, un tableau const rejeu_trame_t (en mémoire flash) et
la table REJEU_SCENARIOS que la commande série j parcourt. Recompiler et
programmer la carte, puis envoyer j : chaque ligne J,... donne la même
empreinte que test_rejeu sur Windows et sur Linux si les décisions sont
identiques. Bibliothèque standard seulement.
"""
import argparse, csv, os, re

CHAMPS = [("ticks_ms", "ticks_ms"), ("vitesse_rpm_x100", "vitesse_rpm_x100"),
          ("temp_c_x10", "temp_c_x10"), ("etat", "etat"), ("sens", "sens"),
          ("pwm_pct", "pwm_pct"), ("plafond_pct", "plafond_pct"), ("mode", "mode"),
          ("segment", "segment")]


def ident(chemin):
    base = os.path.splitext(os.path.basename(chemin))[0]
    return re.sub(r"[^A-Za-z0-9_]", "_", base).upper()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("csv", nargs="+")
    p.add_argument("--sortie", default="../Core/Inc/rejeu_donnees.h")
    p.add_argument("--max", type=int, default=600, help="trames au plus par scenario")
    a = p.parse_args()

    L = ["/* rejeu_donnees.h - GENERE par hote/csv_vers_c.py a partir de :",
         *[f" *   {os.path.basename(c)}" for c in a.csv],
         " * Ne pas modifier a la main : regenerer apres chaque nouvelle trace.",
         " * Inclus seulement par main.c (commande serie j, composante L5-C). */",
         "#ifndef REJEU_DONNEES_H", "#define REJEU_DONNEES_H", "", '#include "rejeu.h"', ""]
    noms = []
    for chemin in a.csv:
        with open(chemin, newline="", encoding="ascii") as f:
            lignes = list(csv.DictReader(f))[: a.max]
        nom = ident(chemin)
        noms.append((nom, os.path.basename(chemin), len(lignes)))
        L.append(f"static const rejeu_trame_t REJEU_{nom}[{len(lignes)}] = {{")
        for r in lignes:
            L.append("    { " + ", ".join(f"{int(r[c])}" for c, _ in CHAMPS) + " },")
        L.append("};")
        L.append("")
    L.append("typedef struct { const char *nom; const rejeu_trame_t *t; uint32_t n; } rejeu_scenario_t;")
    L.append("static const rejeu_scenario_t REJEU_SCENARIOS[] = {")
    for nom, fichier, n in noms:
        L.append(f'    {{ "{fichier}", REJEU_{nom}, {n}u }},')
    L.append("};")
    L.append("#define REJEU_N_SCENARIOS (sizeof REJEU_SCENARIOS / sizeof REJEU_SCENARIOS[0])")
    L.append("")
    L.append("#endif /* REJEU_DONNEES_H */")
    with open(a.sortie, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    for nom, fichier, n in noms:
        print(f"{fichier} : {n} trames -> REJEU_{nom}")
    print(f"ecrit : {a.sortie}")


if __name__ == "__main__":
    main()
