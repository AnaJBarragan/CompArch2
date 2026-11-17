#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""telemetrie.py — vérification de la télémétrie (CEG 3536, projet final, E10, E21)

Usage :
  python telemetrie.py COM8 [--duree 30] [--sauver s3.log]    lecture du port série
  python telemetrie.py s3.log [--extraire s3.csv]             analyse d'un fichier

Lignes reconnues (toutes terminées par *HH puis \\r\\n, HH = OU exclusif, en
hexadécimal majuscule, de tous les caractères qui précèdent l'astérisque) :
  T,<24 colonnes>*HH        trame périodique (5 Hz) : les 17 colonnes de la
                            partie I, puis mode_essai, segment, dist_x10,
                            consigne_rpm, code_defaut, evq_max, crit_max_cycles
  E,<ticks_ms>,<type>,<valeur>*HH    événement consommé
  I,<n>,<adr>,...*HH        balayage du bus : n adresses en hexadécimal
  H,...                     en-tête (somme de contrôle facultative)
Les trames de 17 colonnes (partie I) sont acceptées, avec un avis.
Les autres lignes (réponses aux commandes série) sont comptées, sans plus.

Vérifie : sommes de contrôle, nombre de colonnes, champs entiers, plages,
cadence 5 Hz +/- 10 %, adresses du balayage, ticks_perdus = 0,
crit_max_cycles <= 200. Résume le trajet (distance à chaque changement de
segment) et l'erreur de vitesse 2 s après chaque changement de consigne (E17).

--sauver   enregistre les lignes lues telles quelles : une trace par scénario
           (S1 à S5), preuve du dossier.
--extraire écrit les trames T valides dans un fichier CSV avec une ligne
           d'en-tête : c'est l'entrée du rejeu sur Windows, Linux et STM32
           (composante L5-C).
Dépendance : pyserial (pip install pyserial) pour la lecture du port.
"""
import sys, time, argparse, statistics, re
from functools import reduce

COLONNES_I = ["ticks_ms", "etat", "sens", "consigne_pct", "pwm_pct",
              "temp_c_x10", "vitesse_rpm_x100", "nb_evenements", "evq_debordements",
              "mode", "cause", "plafond_pct", "env_t_x10", "env_h_x10", "env_p_x10",
              "i2c_erreurs", "ticks_perdus"]
COLONNES_II = ["mode_essai", "segment", "dist_x10", "consigne_rpm", "code_defaut",
               "evq_max", "crit_max_cycles"]
COLONNES = COLONNES_I + COLONNES_II
PLAGES = {"etat": (0, 6), "sens": (0, 2), "consigne_pct": (0, 100), "pwm_pct": (0, 100),
          "vitesse_rpm_x100": (0, 40000), "temp_c_x10": (-200, 1000),
          "mode": (0, 1), "cause": (0, 5), "plafond_pct": (10, 100),
          "env_t_x10": (-400, 850), "env_h_x10": (0, 1000), "env_p_x10": (3000, 11000),
          "mode_essai": (0, 1), "segment": (0, 16), "dist_x10": (0, 100000),
          "consigne_rpm": (0, 340), "code_defaut": (0, 5), "evq_max": (0, 64),
          "crit_max_cycles": (0, 200)}
COLONNES_ENV = ("env_t_x10", "env_h_x10", "env_p_x10")
ADRESSES_ATTENDUES = [0x2D, 0x38, 0x39, 0x3E, 0x5A]
ETATS = ["ARRET", "MARCHE_AVANT", "MARCHE_ARRIERE", "ARRET_URGENCE", "ARRET_EVENEMENT", "INIT", "DEFAUT"]
DEFAUTS = ["aucun", "INIT", "I2C", "SENS", "ENCODEUR", "SIMULE"]
TOLERANCE_PCT = 10
DELAI_REGUL_MS = 2000
SOMME = re.compile(r"^(.*)\*([0-9A-F]{2})$")


def xor8(texte):
    return reduce(lambda a, b: a ^ b, texte.encode("ascii", "replace"), 0)


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


def separer(ligne, no, erreurs, obligatoire=True):
    """Retourne le corps de la ligne si la somme de contrôle est valide, sinon None."""
    m = SOMME.match(ligne)
    if not m:
        if obligatoire:
            erreurs.append(f"ligne {no} : somme de controle absente ou mal formee : {ligne!r}")
            return None
        return ligne
    corps, hh = m.group(1), int(m.group(2), 16)
    if xor8(corps) != hh:
        erreurs.append(f"ligne {no} : somme {hh:02X} recue, {xor8(corps):02X} calculee : {ligne!r}")
        return None
    return corps


def resume_trajet(trames):
    """Distance à chaque changement de segment et à la fin du trajet (E16)."""
    for a, b in zip(trames, trames[1:]):
        if b["segment"] != a["segment"] and b["segment"] != 0:
            print(f"  trajet : segment {a['segment']} -> {b['segment']} a {b['dist_x10'] / 10:.1f} tours "
                  f"(t = {b['ticks_ms']} ms), consigne {b['consigne_rpm']} tr/min")
    d = max(t["dist_x10"] for t in trames)
    if d:
        print(f"  trajet : distance maximale relevee {d / 10:.1f} tours")


def resume_regulation(trames, avis):
    """Erreur de vitesse 2 s après chaque changement de consigne, en marche (E17)."""
    i = 1
    while i < len(trames):
        t = trames[i]
        if t["consigne_rpm"] and t["consigne_rpm"] != trames[i - 1]["consigne_rpm"] and t["etat"] in (1, 2):
            j = i
            while (j < len(trames) and trames[j]["consigne_rpm"] == t["consigne_rpm"]
                   and trames[j]["etat"] in (1, 2)
                   and trames[j]["ticks_ms"] - t["ticks_ms"] < DELAI_REGUL_MS):
                j += 1
            if (j < len(trames) and trames[j]["consigne_rpm"] == t["consigne_rpm"]
                    and trames[j]["etat"] in (1, 2)):
                v = trames[j]["vitesse_rpm_x100"] / 100.0
                e = 100.0 * (v - t["consigne_rpm"]) / t["consigne_rpm"]
                verdict = "OK" if abs(e) <= TOLERANCE_PCT else "HORS TOLERANCE"
                print(f"  regulation : consigne {t['consigne_rpm']} tr/min a t = {t['ticks_ms']} ms ; "
                      f"{DELAI_REGUL_MS / 1000:.0f} s plus tard : {v:.1f} tr/min, erreur {e:+.1f} % ({verdict})")
                if abs(e) > TOLERANCE_PCT:
                    avis.append(f"erreur de vitesse {e:+.1f} % a t = {trames[j]['ticks_ms']} ms "
                                f"(tolerance +/- {TOLERANCE_PCT} %, E17) : a expliquer (plafond, limitation ?)")
            else:
                print(f"  regulation : consigne {t['consigne_rpm']} tr/min a t = {t['ticks_ms']} ms ; "
                      f"maintenue moins de {DELAI_REGUL_MS / 1000:.0f} s : non evaluee")
        i += 1


def analyser(lignes, extraire=None):
    trames, evenements, balayages, autres, erreurs, avis = [], [], [], 0, [], []
    entete = False
    partie1 = 0
    for i, l in enumerate(lignes):
        no = i + 1
        l = l.rstrip("\r\n")
        if not l:
            continue
        if l.startswith("H,"):
            entete = True
            separer(l, no, erreurs, obligatoire=False)
            continue
        if l[:2] not in ("T,", "E,", "I,"):
            autres += 1
            continue
        corps = separer(l, no, erreurs)
        if corps is None:
            continue
        champs = corps.split(",")[1:]
        if l.startswith("I,"):
            try:
                n, adr = int(champs[0]), [int(a, 16) for a in champs[1:]]
            except (ValueError, IndexError):
                erreurs.append(f"ligne {no} : balayage mal forme : {l!r}")
                continue
            if n != len(adr):
                erreurs.append(f"ligne {no} : {n} adresses annoncees, {len(adr)} listees")
            if sorted(adr) != ADRESSES_ATTENDUES:
                erreurs.append(f"ligne {no} : adresses {[f'{a:02X}' for a in adr]} au lieu de "
                               f"{[f'{a:02X}' for a in ADRESSES_ATTENDUES]}")
            balayages.append(adr)
            continue
        if l.startswith("E,"):
            attendus = (3,)
        else:
            attendus = (len(COLONNES), len(COLONNES_I))
        if len(champs) not in attendus:
            erreurs.append(f"ligne {no} : {len(champs)} colonnes au lieu de {attendus[0]} : {l!r}")
            continue
        try:
            valeurs = list(map(int, champs))
        except ValueError:
            erreurs.append(f"ligne {no} : champ non entier : {l!r}")
            continue
        if l.startswith("E,"):
            evenements.append(valeurs)
            continue
        if len(valeurs) == len(COLONNES_I):
            partie1 += 1
            valeurs += [0] * len(COLONNES_II)
        t = dict(zip(COLONNES, valeurs))
        sans_capteur = all(t[k] == 0 for k in COLONNES_ENV)
        for k, (a, b) in PLAGES.items():
            if k in COLONNES_ENV and sans_capteur:
                continue
            if not a <= t[k] <= b:
                erreurs.append(f"ligne {no} : {k} = {t[k]} hors de [{a}, {b}]")
        trames.append(t)

    print(f"lignes lues : {len(lignes)} ; trames T valides : {len(trames)} ; evenements E : "
          f"{len(evenements)} ; balayages I : {len(balayages)} ; autres lignes : {autres}")
    print(f"en-tete H : {'present' if entete else 'ABSENT'}")
    if partie1:
        avis.append(f"{partie1} trame(s) de 17 colonnes (format de la partie I) : les sept colonnes "
                    f"de la partie II sont comptees a zero (E21)")
    for adr in balayages:
        print("  balayage :", " ".join(f"0x{a:02X}" for a in adr))
    if len(trames) >= 3:
        dt = [b["ticks_ms"] - a["ticks_ms"] for a, b in zip(trames, trames[1:])]
        moy = statistics.mean(dt)
        duree_s = (trames[-1]["ticks_ms"] - trames[0]["ticks_ms"]) / 1000.0
        freq = (len(trames) - 1) / duree_s if duree_s > 0 else 0.0
        cadence_ok = 4.5 <= freq <= 5.5
        print(f"periode (ticks_ms) : moyenne {moy:.1f} ms, min {min(dt)}, max {max(dt)} ; "
              f"cadence {freq:.2f} Hz ({'OK' if cadence_ok else 'HORS TOLERANCE'})")
        if not cadence_ok:
            erreurs.append(f"cadence {freq:.2f} Hz hors de 5 Hz +/- 10 %")
        for k in COLONNES[1:]:
            v = [t[k] for t in trames]
            print(f"  {k:18s} min {min(v):7d}  max {max(v):7d}  dernier {v[-1]:7d}")
        d = trames[-1]
        print(f"  vitesse {min(t['vitesse_rpm_x100'] for t in trames) / 100:.2f} .. "
              f"{max(t['vitesse_rpm_x100'] for t in trames) / 100:.2f} tr/min")
        vus = sorted(set(t["etat"] for t in trames))
        print("  etats releves :", ", ".join(ETATS[e] for e in vus))
        if all(d[k] == 0 for k in COLONNES_ENV):
            avis.append("colonnes env_* a zero : capteur environnemental absent ou non lu (E7)")
        else:
            print(f"  capteur : {d['env_t_x10'] / 10:.1f} C, {d['env_h_x10'] / 10:.1f} %, "
                  f"{d['env_p_x10'] / 10:.1f} hPa ; analogique {d['temp_c_x10'] / 10:.1f} C")
            if abs(d["env_t_x10"] - d["temp_c_x10"]) > 30 and not d["mode_essai"]:
                avis.append("ecart de plus de 3 C entre les deux capteurs de temperature (E7)")
        resume_trajet(trames)
        resume_regulation(trames, avis)
        essai = sum(1 for t in trames if t["mode_essai"])
        if essai:
            avis.append(f"{essai} trame(s) en mode essai : les valeurs injectees ne sont pas des mesures (E20)")
        codes = sorted(set(t["code_defaut"] for t in trames if t["code_defaut"]))
        if codes:
            avis.append("codes de defaut releves : " + ", ".join(f"{c} ({DEFAUTS[c]})" for c in codes))
        print(f"  file : occupation maximale {max(t['evq_max'] for t in trames)} ; "
              f"masquage maximal {max(t['crit_max_cycles'] for t in trames)} cycles (borne : 200)")
        if d["ticks_perdus"]:
            erreurs.append(f"ticks_perdus = {d['ticks_perdus']} (doit rester a 0, E11)")
        if d["i2c_erreurs"]:
            avis.append(f"i2c_erreurs = {d['i2c_erreurs']} (attendu apres la commande d, sinon a expliquer)")
        if d["evq_debordements"]:
            avis.append(f"debordements de la file signales : {d['evq_debordements']} "
                        f"(attendu apres la commande e, sinon a expliquer)")
    if evenements:
        par_type = {}
        for _, typ, _ in evenements:
            par_type[typ] = par_type.get(typ, 0) + 1
        print("  evenements par type :", ", ".join(f"{k} x {n}" for k, n in sorted(par_type.items())))
    if extraire:
        with open(extraire, "w", encoding="ascii", newline="\n") as f:
            f.write(",".join(COLONNES) + "\n")
            for t in trames:
                f.write(",".join(str(t[k]) for k in COLONNES) + "\n")
        print(f"{len(trames)} trames ecrites dans {extraire} (entree du rejeu, L5-C)")
    for a in avis:
        print("AVIS :", a)
    for e in erreurs[:20]:
        print("ERREUR :", e)
    if len(erreurs) > 20:
        print(f"... {len(erreurs) - 20} autres erreurs")
    ok = not erreurs and entete and len(trames) >= 3
    print("RESULTAT :", "OK" if ok else "ECHEC")
    return 0 if ok else 1


def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", help="port serie (COM8, /dev/ttyACM0) ou fichier journal")
    p.add_argument("--duree", type=float, default=30.0, help="secondes de lecture du port")
    p.add_argument("--sauver", help="fichier ou ecrire les lignes lues (trace du scenario)")
    p.add_argument("--extraire", help="fichier CSV ou ecrire les trames T valides (rejeu)")
    a = p.parse_args()
    if a.source.upper().startswith("COM") or a.source.startswith("/dev/"):
        lignes = lire_port(a.source, a.duree)
        if a.sauver:
            with open(a.sauver, "w", encoding="ascii", errors="replace", newline="") as f:
                f.writelines(lignes)
    else:
        with open(a.source, encoding="ascii", errors="replace", newline="") as f:
            lignes = f.readlines()
    sys.exit(analyser(lignes, a.extraire))


if __name__ == "__main__":
    main()
