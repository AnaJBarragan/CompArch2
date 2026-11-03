#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""telemetrie.py — vérification des trames de télémétrie (CEG 3536, lab. 3, E10)

Usage :
  python telemetrie.py COM8 [--duree 30] [--sauver journal.csv]   lecture du port série
  python telemetrie.py journal.csv                                 analyse d'un fichier

Vérifie : en-tête H, format des trames T (10 colonnes), cadence 5 Hz +/- 10 %,
plages (etat 0..3, sens 0..2, consigne et pwm 0..100, vitesse 0..40000 (x100),
temperature -200..1000 (x10)), trames tronquées, débordements de la file.
Dépendance : pyserial (pip install pyserial) pour la lecture du port.
"""
import sys, time, argparse, statistics

COLONNES = ["ticks_ms", "etat", "sens", "consigne_pct", "pwm_pct",
            "temp_c_x10", "vitesse_rpm_x100", "nb_evenements", "evq_debordements"]
PLAGES = {"etat": (0, 3), "sens": (0, 2), "consigne_pct": (0, 100), "pwm_pct": (0, 100),
          "vitesse_rpm_x100": (0, 40000), "temp_c_x10": (-200, 1000)}


def lire_port(port, duree, bauds=115200):
    import serial
    lignes = []
    with serial.Serial(port, bauds, timeout=1) as s:
        s.reset_input_buffer()
        fin = time.time() + duree
        while time.time() < fin:
            l = s.readline()
            if l:
                lignes.append(l.decode("ascii", "replace"))
    return lignes


def analyser(lignes):
    trames, autres, erreurs = [], 0, []
    entete = any(l.startswith("H,") for l in lignes)
    for i, l in enumerate(lignes):
        l = l.rstrip("\r\n")
        if not l or l.startswith("H,"):
            continue
        if not l.startswith("T,"):
            autres += 1
            continue
        champs = l.split(",")[1:]
        if len(champs) != len(COLONNES):
            erreurs.append(f"ligne {i + 1} : {len(champs)} colonnes au lieu de {len(COLONNES)} : {l!r}")
            continue
        try:
            t = dict(zip(COLONNES, map(int, champs)))
        except ValueError:
            erreurs.append(f"ligne {i + 1} : champ non entier : {l!r}")
            continue
        for k, (a, b) in PLAGES.items():
            if not a <= t[k] <= b:
                erreurs.append(f"ligne {i + 1} : {k} = {t[k]} hors de [{a}, {b}]")
        trames.append(t)

    print(f"lignes lues : {len(lignes)} ; trames T valides : {len(trames)} ; autres lignes : {autres}")
    print(f"en-tete H : {'present' if entete else 'ABSENT'}")
    if len(trames) >= 3:
        dt = [b["ticks_ms"] - a["ticks_ms"] for a, b in zip(trames, trames[1:])]
        moy = statistics.mean(dt)
        duree_s = (trames[-1]["ticks_ms"] - trames[0]["ticks_ms"]) / 1000.0
        freq = (len(trames) - 1) / duree_s if duree_s > 0 else 0.0
        print(f"periode (ticks_ms) : moyenne {moy:.1f} ms, min {min(dt)}, max {max(dt)} ; cadence {freq:.2f} Hz "
              f"({'OK' if 4.5 <= freq <= 5.5 else 'HORS TOLERANCE'})")
        for k in COLONNES[1:]:
            v = [t[k] for t in trames]
            print(f"  {k:18s} min {min(v):7d}  max {max(v):7d}  dernier {v[-1]:7d}")
        v = [t["vitesse_rpm_x100"] / 100 for t in trames]
        tc = [t["temp_c_x10"] / 10 for t in trames]
        print(f"  vitesse {min(v):.2f} .. {max(v):.2f} tr/min ; temperature {min(tc):.1f} .. {max(tc):.1f} C")
        if trames[-1]["evq_debordements"]:
            print(f"  debordements de la file signales : {trames[-1]['evq_debordements']}")
    for e in erreurs[:20]:
        print("ERREUR :", e)
    if len(erreurs) > 20:
        print(f"... {len(erreurs) - 20} autres erreurs")
    print("RESULTAT :", "OK" if not erreurs and entete and len(trames) >= 3 else "ECHEC")
    return 0 if not erreurs else 1


def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", help="port serie (COM8, /dev/ttyACM0) ou fichier journal")
    p.add_argument("--duree", type=float, default=30.0, help="secondes de lecture du port")
    p.add_argument("--sauver", help="fichier ou ecrire les lignes lues")
    a = p.parse_args()
    if a.source.upper().startswith("COM") or a.source.startswith("/dev/"):
        lignes = lire_port(a.source, a.duree)
        if a.sauver:
            with open(a.sauver, "w", encoding="ascii", errors="replace") as f:
                f.writelines(lignes)
    else:
        with open(a.source, encoding="ascii", errors="replace") as f:
            lignes = f.readlines()
    sys.exit(analyser(lignes))


if __name__ == "__main__":
    main()
