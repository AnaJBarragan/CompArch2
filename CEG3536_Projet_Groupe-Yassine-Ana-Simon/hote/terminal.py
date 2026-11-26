#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""terminal.py — terminal série et filtre des changements (CEG 3536, outil d'équipe)

Deux usages, dans deux fenêtres :

  1. Console (tient le port, affiche TOUT, enregistre tout, envoie les touches) :
       python3 terminal.py /dev/cu.usbmodem1403 --log trace.log
     Chaque touche tapée est envoyée à la carte (a, x, f, w, d, z, ...).
     Quitter : Ctrl-C.

  2. Changements seulement (lit le fichier, n'ouvre pas le port) :
       python3 terminal.py --suivre trace.log
     Affiche les lignes E, I, A, X, ... telles quelles mais décodées, et une
     trame T seulement quand l'état du véhicule change (état, mode, cause,
     consigne, plafond, compteurs d'erreurs) ou qu'une mesure varie nettement.

Le fichier trace.log contient toutes les lignes reçues : c'est la trace d'un
essai, que telemetrie.py analyse ensuite (python3 telemetrie.py trace.log).
Bibliothèque standard seulement (termios) : macOS et Linux. Sous Windows,
utiliser PuTTY avec journalisation, puis --suivre sur le journal.
"""
import argparse, os, select, sys, termios, time, tty

COLONNES = ["ticks_ms", "etat", "sens", "consigne_pct", "pwm_pct", "temp_c_x10",
            "vitesse_rpm_x100", "nb_evenements", "evq_debordements", "mode", "cause",
            "plafond_pct", "env_t_x10", "env_h_x10", "env_p_x10", "i2c_erreurs",
            "ticks_perdus",
            # partie II (E21)
            "mode_essai", "segment", "dist_x10", "consigne_rpm", "code_defaut", "evq_max",
            "crit_max_cycles"]
ETATS = ["ARRET", "MARCHE_AVANT", "MARCHE_ARRIERE", "URGENCE", "ARRET_EVENEMENT",
         "INIT", "DEFAUT"]
CAUSES = ["-", "FIN", "FEU", "OBSTACLE", "DEFAUT", "THERMIQUE"]
EVENEMENTS = {1: "User", 2: "Touch En", 3: "E-Stop", 7: "essai", 8: "touche",
              9: "PIR", 10: "commutateur", 11: "mesure env.", 12: "defaut",
              13: "thermique", 14: "fin de trajet", 15: "mode essai"}
TOUCHES = {10: "Fn (Shift)", 11: "Next/Back"}
# colonnes comparées telles quelles : tout changement est affiché
EXACTES = ["etat", "sens", "consigne_pct", "mode", "cause", "plafond_pct",
           "evq_debordements", "i2c_erreurs", "ticks_perdus",
           "mode_essai", "segment", "consigne_rpm", "code_defaut"]
# colonnes bruitées : affichées seulement si l'écart dépasse le seuil
SEUILS = {"pwm_pct": 5, "temp_c_x10": 5, "vitesse_rpm_x100": 1000,
          "env_t_x10": 5, "env_h_x10": 10, "env_p_x10": 10, "dist_x10": 50}


# ---- 1. console --------------------------------------------------------------
def ouvrir_port(chemin, bauds=termios.B115200):
    fd = os.open(chemin, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    tty.setraw(fd)
    a = termios.tcgetattr(fd)
    a[2] |= termios.CLOCAL | termios.CREAD           # cflag : ignorer le modem
    a[4] = a[5] = bauds                               # vitesses entrée et sortie
    termios.tcsetattr(fd, termios.TCSANOW, a)
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd


def console(port, journal):
    try:
        fd = ouvrir_port(port)
    except OSError as e:
        if e.errno == 16:                            # EBUSY : port déjà ouvert ailleurs
            sys.exit(f"Port {port} occupe : un autre programme l'a ouvert (screen, console de "
                     f"STM32CubeIDE, telemetrie.py...). Fermer ce programme, puis relancer.\n"
                     f"Pour voir lequel : lsof {port}\n"
                     f"Pour fermer un screen oublie : screen -ls, puis screen -X -S <numero> quit")
        if e.errno == 2:                             # ENOENT : carte absente ou autre nom
            sys.exit(f"Port {port} introuvable : carte branchee ? Ports presents : ls /dev/cu.usbmodem*")
        raise
    log = open(journal, "a", encoding="ascii", errors="replace", newline="") if journal else None
    clavier = sys.stdin.fileno() if sys.stdin.isatty() else None
    ancien = termios.tcgetattr(clavier) if clavier is not None else None
    if clavier is not None:
        tty.setcbreak(clavier)                       # une touche = un caractère envoyé
    print(f"[{port} a 115200 bauds{' -> ' + journal if journal else ''} ; Ctrl-C pour quitter]")
    tampon = b""
    try:
        while True:
            entrees = [fd] + ([clavier] if clavier is not None else [])
            pret, _, _ = select.select(entrees, [], [], 0.5)
            if clavier in pret:
                c = os.read(clavier, 1)
                if c:
                    os.write(fd, c)
            if fd in pret:
                try:
                    tampon += os.read(fd, 4096)
                except BlockingIOError:
                    continue
                while b"\n" in tampon:
                    ligne, tampon = tampon.split(b"\n", 1)
                    texte = ligne.decode("ascii", "replace") + "\n"
                    sys.stdout.write(texte.rstrip("\r\n") + "\n")
                    sys.stdout.flush()
                    if log:
                        log.write(texte if texte.endswith("\r\n") else texte.rstrip("\n") + "\r\n")
                        log.flush()
    except KeyboardInterrupt:
        print("\n[fin]")
    finally:
        if ancien is not None:
            termios.tcsetattr(clavier, termios.TCSADRAIN, ancien)
        if log:
            log.close()
        os.close(fd)


# ---- 2. changements seulement -------------------------------------------------
def nom(liste, i):
    return liste[i] if 0 <= i < len(liste) else str(i)


def decrire_evenement(champs):
    t, typ, v = (int(x) for x in champs)
    quoi = EVENEMENTS.get(typ, f"type {typ}")
    if typ == 8:
        detail = TOUCHES.get(v, f"chiffre {v}")
    elif typ == 9:
        detail = f"signal haut, traite {t - v} ms apres l'IRQ" if v else "signal retombe"
    elif typ == 10:
        detail = f"SW{v}"
    elif typ == 12:
        etat = "retabli" if v & 0x8000 else "MUET"
        detail = f"0x{v & 0x7F:02X} {etat}"
    else:
        detail = str(v)
    return f"{t / 1000:9.3f} s  EVENEMENT {quoi} : {detail}"


def decrire_trame(t, changes):
    morceaux = []
    for k in changes:
        if k == "etat":
            morceaux.append(f"etat {nom(ETATS, t[k])}")
        elif k == "mode":
            morceaux.append("mode " + ("AUT" if t[k] else "MAN"))
        elif k == "cause":
            morceaux.append(f"cause {nom(CAUSES, t[k])}")
        elif k == "sens":
            morceaux.append("sens " + ["immobile", "avant", "arriere"][t[k]] if t[k] <= 2 else str(t[k]))
        elif k == "vitesse_rpm_x100":
            morceaux.append(f"vitesse {t[k] / 100:.1f} tr/min")
        elif k in ("temp_c_x10", "env_t_x10"):
            morceaux.append(f"{'T analog.' if k == 'temp_c_x10' else 'T env.'} {t[k] / 10:.1f} C")
        elif k == "env_h_x10":
            morceaux.append(f"H {t[k] / 10:.1f} %")
        elif k == "env_p_x10":
            morceaux.append(f"P {t[k] / 10:.1f} hPa")
        else:
            morceaux.append(f"{k} {t[k]}")
    return f"{t['ticks_ms'] / 1000:9.3f} s  " + " | ".join(morceaux)


def filtrer(ligne, precedente):
    """Retourne le texte à afficher pour cette ligne (ou None) ; met à jour precedente."""
    ligne = ligne.rstrip("\r\n")
    if not ligne:
        return None
    corps = ligne.split("*")[0]
    if ligne.startswith("T,"):
        try:
            champs = corps.split(",")[1:]
            noms = COLONNES[:len(champs)] if len(champs) in (17, 24) else COLONNES
            t = dict(zip(noms, map(int, champs)))
            for k in COLONNES[len(t):]:
                t[k] = 0                        # trame de 17 colonnes (partie I)
        except ValueError:
            return f"(trame illisible) {ligne}"
        if len(champs) not in (17, 24):
            return f"(trame tronquee) {ligne}"
        if not precedente:
            precedente.update(t)
            return decrire_trame(t, COLONNES[1:7] + COLONNES[9:])
        changes = [k for k in EXACTES if t[k] != precedente[k]]
        changes += [k for k, s in SEUILS.items() if abs(t[k] - precedente[k]) >= s]
        if not changes:
            return None
        for k in changes:
            precedente[k] = t[k]
        return decrire_trame(t, [k for k in COLONNES if k in changes])
    if ligne.startswith("E,"):
        try:
            return decrire_evenement(corps.split(",")[1:4])
        except ValueError:
            return ligne
    if ligne.startswith("H,"):
        precedente.clear()                           # réinitialisation de la carte
        return "---------- carte reinitialisee (ligne H) ----------"
    if ligne.startswith("I,"):
        champs = corps.split(",")
        return f"           BALAYAGE I2C : {champs[1]} adresse(s) : {' '.join(champs[2:])}"
    return f"           {ligne}"                     # réponses aux commandes (A, X, F, W, ...)


def suivre(journal, depuis_debut):
    while not os.path.exists(journal):
        print(f"[en attente de {journal} : lancer la console avec --log]")
        time.sleep(1)
    precedente = {}
    with open(journal, encoding="ascii", errors="replace", newline="") as f:
        if not depuis_debut:
            f.seek(0, os.SEEK_END)
        print(f"[changements de {journal} ; Ctrl-C pour quitter]")
        reste = ""
        try:
            while True:
                morceau = f.readline()
                if not morceau:
                    time.sleep(0.1)
                    continue
                reste += morceau
                if not reste.endswith("\n"):
                    continue                         # ligne pas encore complète
                texte = filtrer(reste, precedente)
                reste = ""
                if texte:
                    print(texte, flush=True)
        except KeyboardInterrupt:
            print("\n[fin]")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("port", nargs="?", help="port série, par exemple /dev/cu.usbmodem1403")
    p.add_argument("--log", help="fichier où enregistrer toutes les lignes reçues")
    p.add_argument("--suivre", metavar="FICHIER", help="afficher seulement les changements de ce fichier")
    p.add_argument("--debut", action="store_true", help="avec --suivre : relire le fichier depuis le début")
    a = p.parse_args()
    if a.suivre:
        suivre(a.suivre, a.debut)
    elif a.port:
        console(a.port, a.log)
    else:
        p.print_help()


if __name__ == "__main__":
    main()
