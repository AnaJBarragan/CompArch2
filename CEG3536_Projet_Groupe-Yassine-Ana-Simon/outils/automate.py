#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""automate.py — vérificateur et générateur pédagogique d'automates finis
(CEG 3536, projet final, exigence E15, composante L5-B) — FOURNI, ne pas modifier

Usage :
  python automate.py verifier  table.json [--rapport rapport.txt]
  python automate.py generer   table.json [--dossier D | --inc Core/Inc --src Core/Src]
  python automate.py simuler   table.json sequence.txt [--sortie trace.txt]
  python automate.py comparer  trace_a.txt trace_b.txt

verifier  lit la table de transitions, explore tous les états accessibles et
          toutes les entrées, contrôle le déterminisme, la complétude et les
          invariants, et produit un contre-exemple (suite d'entrées depuis
          l'état initial) pour chaque erreur. Code de retour 0 si tout passe.
generer   refait la vérification, puis écrit automate_gen.h et automate_gen.c.
          Rien n'est écrit si la vérification échoue.
simuler   exécute le MODÈLE sur une suite d'entrées (un nom par ligne) et écrit
          la trace; hote/test_automate.c écrit la trace du CODE GÉNÉRÉ dans le
          même format.
comparer  compare deux traces ligne à ligne (fins de ligne ignorées).

Le format de la table est décrit dans outils/LISEZMOI.md. Aucune dépendance :
Python 3.8 ou plus récent, bibliothèque standard seulement.
"""
import sys
import os
import re
import json
import argparse
import hashlib
from collections import deque, OrderedDict

VERSION = "1.0"
IDENT_MAJ = re.compile(r"^[A-Z][A-Z0-9_]*$")
IDENT_MIN = re.compile(r"^[a-z][a-z0-9_]*$")
TYPES_INVARIANTS = ("sortie", "interdit", "toujours", "retour", "accessible")


class ErreurTable(Exception):
    """Erreur de structure : la table ne peut pas être analysée."""


# --------------------------------------------------------------------------
# Lecture et contrôle de structure
# --------------------------------------------------------------------------
def _liste(v, quoi):
    if isinstance(v, str):
        return [v]
    if isinstance(v, list) and all(isinstance(x, str) for x in v):
        return list(v)
    raise ErreurTable(f"{quoi} : une chaine ou une liste de chaines est attendue, recu {v!r}")


def charger(chemin):
    try:
        with open(chemin, encoding="utf-8") as f:
            d = json.load(f, object_pairs_hook=OrderedDict)
    except OSError as e:
        raise ErreurTable(f"lecture impossible : {e}")
    except json.JSONDecodeError as e:
        raise ErreurTable(f"JSON invalide, ligne {e.lineno}, colonne {e.colno} : {e.msg}")
    if not isinstance(d, dict):
        raise ErreurTable("la table doit etre un objet JSON")
    for cle in ("nom", "entrees", "sorties", "initial", "etats", "transitions"):
        if cle not in d:
            raise ErreurTable(f"cle obligatoire absente : \"{cle}\"")

    m = {"nom": str(d["nom"]), "chemin": chemin}
    m["entrees"] = _liste(d["entrees"], "entrees")
    m["sorties"] = _liste(d["sorties"], "sorties")
    if not isinstance(d["etats"], dict) or not d["etats"]:
        raise ErreurTable("\"etats\" doit etre un objet non vide : nom de l'etat -> valeurs des sorties")
    m["etats"] = list(d["etats"].keys())
    m["initial"] = d["initial"]

    for nom in m["etats"] + m["entrees"]:
        if not IDENT_MAJ.match(nom):
            raise ErreurTable(f"nom invalide \"{nom}\" : majuscules non accentuees, chiffres et _ "
                              f"(exemple : ARRET_URGENCE)")
    for nom in m["sorties"]:
        if not IDENT_MIN.match(nom):
            raise ErreurTable(f"nom de sortie invalide \"{nom}\" : minuscules non accentuees, chiffres et _")
    for quoi, noms in (("entrees", m["entrees"]), ("sorties", m["sorties"])):
        if len(set(noms)) != len(noms):
            raise ErreurTable(f"nom en double dans \"{quoi}\"")
    if len(m["entrees"]) > 32:
        raise ErreurTable("au plus 32 entrees (masque de 32 bits dans automate_choisir)")
    if len(m["etats"]) > 250:
        raise ErreurTable("au plus 250 etats")
    if m["initial"] not in m["etats"]:
        raise ErreurTable(f"etat initial inconnu : \"{m['initial']}\"")

    m["val"] = {}
    for e in m["etats"]:
        v = d["etats"][e]
        if not isinstance(v, dict):
            raise ErreurTable(f"etat {e} : un objet {{sortie: valeur}} est attendu")
        manquantes = [s for s in m["sorties"] if s not in v]
        inconnues = [s for s in v if s not in m["sorties"]]
        if manquantes:
            raise ErreurTable(f"etat {e} : sortie(s) non definie(s) : {', '.join(manquantes)}")
        if inconnues:
            raise ErreurTable(f"etat {e} : sortie(s) inconnue(s) : {', '.join(inconnues)}")
        for s in m["sorties"]:
            if isinstance(v[s], bool) or not isinstance(v[s], int) or not 0 <= v[s] <= 255:
                raise ErreurTable(f"etat {e}, sortie {s} : entier de 0 a 255 attendu, recu {v[s]!r}")
        m["val"][e] = {s: v[s] for s in m["sorties"]}

    if not isinstance(d["transitions"], list):
        raise ErreurTable("\"transitions\" doit etre une liste")
    m["regles"] = []
    for i, t in enumerate(d["transitions"]):
        no = i + 1
        if not isinstance(t, dict) or not all(k in t for k in ("de", "sur", "vers")):
            raise ErreurTable(f"transition {no} : les cles \"de\", \"sur\" et \"vers\" sont obligatoires")
        joker = t["de"] == "*"
        de = m["etats"] if joker else _liste(t["de"], f"transition {no}, \"de\"")
        sur = m["entrees"] if t["sur"] == "*" else _liste(t["sur"], f"transition {no}, \"sur\"")
        vers = t["vers"]
        for e in de:
            if e not in m["etats"]:
                raise ErreurTable(f"transition {no} : etat de depart inconnu \"{e}\"")
        for x in sur:
            if x not in m["entrees"]:
                raise ErreurTable(f"transition {no} : entree inconnue \"{x}\"")
        if vers != "=" and vers not in m["etats"]:
            raise ErreurTable(f"transition {no} : etat d'arrivee inconnu \"{vers}\" (\"=\" pour rester)")
        m["regles"].append({"no": no, "de": de, "sur": sur, "vers": vers, "joker": joker})

    m["invariants"] = []
    for i, inv in enumerate(d.get("invariants", [])):
        no = i + 1
        if not isinstance(inv, dict) or inv.get("type") not in TYPES_INVARIANTS:
            raise ErreurTable(f"invariant {no} : \"type\" doit valoir {', '.join(TYPES_INVARIANTS)}")
        inv = dict(inv)
        inv["no"] = no
        inv.setdefault("nom", f"invariant {no}")
        _controler_invariant(m, inv)
        m["invariants"].append(inv)
    return m


def _etats_connus(m, inv, cle, obligatoire=True):
    if cle not in inv:
        if obligatoire:
            raise ErreurTable(f"invariant {inv['no']} ({inv['type']}) : cle \"{cle}\" absente")
        return None
    noms = m["etats"] if inv[cle] == "*" else _liste(inv[cle], f"invariant {inv['no']}, \"{cle}\"")
    for e in noms:
        if e not in m["etats"]:
            raise ErreurTable(f"invariant {inv['no']} : etat inconnu \"{e}\"")
    return noms


def _condition(m, inv, cle):
    c = inv.get(cle)
    if not isinstance(c, dict) or not c:
        raise ErreurTable(f"invariant {inv['no']} : \"{cle}\" doit etre un objet {{sortie: valeur}}")
    for s, v in c.items():
        if s not in m["sorties"]:
            raise ErreurTable(f"invariant {inv['no']} : sortie inconnue \"{s}\"")
        if isinstance(v, bool) or not isinstance(v, int):
            raise ErreurTable(f"invariant {inv['no']} : valeur entiere attendue pour \"{s}\"")
    return c


def _controler_invariant(m, inv):
    t = inv["type"]
    if t == "sortie":
        if "si" in inv:
            inv["si"] = _condition(m, inv, "si")
            inv["alors"] = _condition(m, inv, "alors")
        else:
            inv["etats"] = _etats_connus(m, inv, "etats")
            if inv.get("sortie") not in m["sorties"]:
                raise ErreurTable(f"invariant {inv['no']} : sortie inconnue {inv.get('sortie')!r}")
            if isinstance(inv.get("valeur"), bool) or not isinstance(inv.get("valeur"), int):
                raise ErreurTable(f"invariant {inv['no']} : \"valeur\" entiere attendue")
    elif t == "interdit":
        inv["de"] = _etats_connus(m, inv, "de")
        inv["vers"] = _etats_connus(m, inv, "vers")
        if "sur" in inv:
            inv["sur"] = _liste(inv["sur"], f"invariant {inv['no']}, \"sur\"")
    elif t == "toujours":
        inv["sur"] = _liste(inv.get("sur"), f"invariant {inv['no']}, \"sur\"")
        if inv.get("vers") not in m["etats"]:
            raise ErreurTable(f"invariant {inv['no']} : etat d'arrivee inconnu {inv.get('vers')!r}")
        inv["de"] = _etats_connus(m, inv, "de", obligatoire=False)
        inv["sauf"] = _etats_connus(m, inv, "sauf", obligatoire=False) or []
    elif t == "retour":
        if inv.get("vers") not in m["etats"]:
            raise ErreurTable(f"invariant {inv['no']} : etat inconnu {inv.get('vers')!r}")
    elif t == "accessible":
        inv["etats"] = _etats_connus(m, inv, "etats", obligatoire=False) or list(m["etats"])
    for x in inv.get("sur", []) if t in ("interdit", "toujours") else []:
        if x not in m["entrees"]:
            raise ErreurTable(f"invariant {inv['no']} : entree inconnue \"{x}\"")


# --------------------------------------------------------------------------
# Analyse
# --------------------------------------------------------------------------
class Analyse:
    def __init__(self, m):
        self.m = m
        self.erreurs = []
        self.avis = []
        self.notes = []
        self.delta = {}          # (etat, entree) -> etat
        self.origine = {}        # (etat, entree) -> numero de la regle retenue
        self._determinisme()
        self._accessibles()
        self._completude()
        self._invariants()

    # ---- déterminisme : au plus une transition par couple (état, entrée) ----
    def _determinisme(self):
        m = self.m
        explicites, jokers = {}, {}
        for r in m["regles"]:
            cible = jokers if r["joker"] else explicites
            for e in r["de"]:
                for x in r["sur"]:
                    vers = e if r["vers"] == "=" else r["vers"]
                    cible.setdefault((e, x), []).append((vers, r["no"]))
        self.conflits = []
        for nature, table in (("explicites", explicites), ("generales (\"*\")", jokers)):
            for (e, x), lst in table.items():
                cibles = sorted(set(v for v, _ in lst))
                nos = ", ".join(str(n) for _, n in lst)
                if len(cibles) > 1:
                    self.conflits.append((e, x, f"transitions {nature} n. {nos} : vers {' ou '.join(cibles)}"))
                elif len(lst) > 1:
                    self.avis.append(f"doublon sans consequence : ({e}, {x}) defini par les transitions n. {nos}")
        for cle, lst in jokers.items():
            self.delta[cle], self.origine[cle] = lst[0]
        precisees = 0
        for cle, lst in explicites.items():
            if cle in jokers and jokers[cle][0][0] != lst[0][0]:
                precisees += 1
            self.delta[cle], self.origine[cle] = lst[0]
        if precisees:
            self.notes.append(f"{precisees} couple(s) (etat, entree) : une transition explicite precise une "
                              f"transition generale \"*\" (la transition explicite l'emporte)")

    # ---- états accessibles (parcours en largeur, pour des chemins minimaux) ----
    def _accessibles(self):
        m = self.m
        self.parent = {m["initial"]: None}
        file = deque([m["initial"]])
        while file:
            e = file.popleft()
            for x in m["entrees"]:
                s = self.delta.get((e, x))
                if s is not None and s not in self.parent:
                    self.parent[s] = (e, x)
                    file.append(s)
        self.acc = [e for e in m["etats"] if e in self.parent]
        for e, x, detail in self.conflits:
            if e in self.parent:
                self.erreurs.append(("INDETERMINISME", f"({e}, {x}) : {detail}", self.chemin(e) + [(x, "?")]))
            else:
                self.avis.append(f"indeterminisme dans un etat inaccessible : ({e}, {x}) : {detail}")
        for e in m["etats"]:
            if e not in self.parent:
                self.avis.append(f"etat inaccessible depuis {m['initial']} : {e}")

    def chemin(self, etat):
        """Suite [(entree, etat atteint), ...] de l'état initial à etat."""
        pas = []
        while self.parent.get(etat) is not None:
            p, x = self.parent[etat]
            pas.append((x, etat))
            etat = p
        return list(reversed(pas))

    # ---- complétude : une transition pour chaque (état accessible, entrée) ----
    def _completude(self):
        m = self.m
        for e in self.acc:
            manque = [x for x in m["entrees"] if (e, x) not in self.delta]
            for x in manque:
                self.erreurs.append(("INCOMPLETUDE", f"aucune transition pour ({e}, {x})",
                                     self.chemin(e) + [(x, "?")]))
        for e in m["etats"]:
            if e not in self.parent:
                manque = [x for x in m["entrees"] if (e, x) not in self.delta]
                if manque:
                    self.avis.append(f"etat inaccessible {e} : entrees sans transition : {', '.join(manque)}")

    # ---- invariants ----
    def _invariants(self):
        m = self.m
        self.resultats = []
        for inv in m["invariants"]:
            avant = len(self.erreurs)
            getattr(self, "_inv_" + inv["type"])(inv)
            self.resultats.append((inv, len(self.erreurs) == avant))

    def _viol(self, inv, texte, chemin):
        self.erreurs.append(("INVARIANT", f"« {inv['nom']} » : {texte}", chemin))

    def _inv_sortie(self, inv):
        m = self.m
        if "si" in inv:
            for e in self.acc:
                if all(m["val"][e][s] == v for s, v in inv["si"].items()):
                    for s, v in inv["alors"].items():
                        if m["val"][e][s] != v:
                            self._viol(inv, f"dans l'etat {e}, {s} = {m['val'][e][s]} au lieu de {v}",
                                       self.chemin(e))
        else:
            for e in inv["etats"]:
                if m["val"][e][inv["sortie"]] != inv["valeur"]:
                    self._viol(inv, f"dans l'etat {e}, {inv['sortie']} = {m['val'][e][inv['sortie']]} "
                                    f"au lieu de {inv['valeur']}", self.chemin(e) if e in self.parent else [])

    def _inv_interdit(self, inv):
        entrees = inv.get("sur", self.m["entrees"])
        for e in inv["de"]:
            if e not in self.parent:
                continue
            for x in entrees:
                s = self.delta.get((e, x))
                if s in inv["vers"] and s != e:
                    self._viol(inv, f"transition interdite {e} --{x}--> {s} (transition n. {self.origine[(e, x)]})",
                               self.chemin(e) + [(x, s)])

    def _inv_toujours(self, inv):
        etats = inv["de"] if inv["de"] is not None else self.acc
        for e in etats:
            if e not in self.parent or e in inv["sauf"]:
                continue
            for x in inv["sur"]:
                s = self.delta.get((e, x))
                if s != inv["vers"]:
                    self._viol(inv, f"depuis {e}, l'entree {x} mene a {s if s else 'aucun etat'} "
                                    f"au lieu de {inv['vers']}", self.chemin(e) + [(x, s if s else "?")])

    def _inv_retour(self, inv):
        cible = inv["vers"]
        peut = {cible}
        change = True
        while change:
            change = False
            for (e, _), s in self.delta.items():
                if s in peut and e not in peut:
                    peut.add(e)
                    change = True
        for e in self.acc:
            if e not in peut:
                self._viol(inv, f"depuis {e}, l'etat {cible} n'est plus jamais atteignable (etat piege)",
                           self.chemin(e))

    def _inv_accessible(self, inv):
        for e in inv["etats"]:
            if e not in self.parent:
                self._viol(inv, f"l'etat {e} n'est pas accessible depuis {self.m['initial']}", [])

    # ---- rapport ----
    @property
    def ok(self):
        return not self.erreurs

    def somme(self):
        m = self.m
        canon = json.dumps({"etats": m["etats"], "entrees": m["entrees"], "sorties": m["sorties"],
                            "initial": m["initial"], "val": [m["val"][e] for e in m["etats"]],
                            "delta": [[self.delta.get((e, x)) for x in m["entrees"]] for e in m["etats"]]},
                           sort_keys=True)
        return hashlib.sha256(canon.encode("utf-8")).hexdigest()[:8].upper()

    def fmt_chemin(self, pas):
        txt = self.m["initial"]
        for x, s in pas:
            txt += f" --{x}--> {s}"
        return txt

    def rapport(self):
        m = self.m
        L = []
        L.append(f"automate.py {VERSION} — rapport de verification")
        L.append(f"table      : {os.path.basename(m['chemin'])}  (automate « {m['nom']} »)")
        L.append(f"somme      : {self.somme()}  (empreinte de la table; reprise dans le code genere)")
        L.append(f"etats      : {len(m['etats'])} ({len(self.acc)} accessibles) — initial : {m['initial']}")
        L.append(f"entrees    : {len(m['entrees'])}, par priorite decroissante : {', '.join(m['entrees'])}")
        L.append(f"sorties    : {', '.join(m['sorties'])}")
        L.append(f"couples    : {len(self.acc) * len(m['entrees'])} (etat accessible, entree) explores; "
                 f"{len(m['regles'])} transitions ecrites")
        L.append("")
        L.append("TABLE DE TRANSITIONS (ligne = etat, colonne = entree; « . » = rester; « ? » = absente)")
        larg = max(len(e) for e in m["etats"]) + 1
        cols = [max(len(x), max(len(e) for e in m["etats"])) for x in m["entrees"]]
        L.append("  " + " " * larg + " ".join(x.ljust(c) for x, c in zip(m["entrees"], cols)))
        for e in m["etats"]:
            cell = []
            for x, c in zip(m["entrees"], cols):
                s = self.delta.get((e, x))
                cell.append(("?" if s is None else "." if s == e else s).ljust(c))
            marque = "" if e in self.parent else "  (inaccessible)"
            L.append("  " + e.ljust(larg) + " ".join(cell).rstrip() + marque)
        L.append("")
        L.append("SORTIES PAR ETAT")
        for e in m["etats"]:
            L.append("  " + e.ljust(larg) + "  ".join(f"{s}={m['val'][e][s]}" for s in m["sorties"]))
        L.append("")
        L.append("CONTROLES")
        nb = {k: sum(1 for t, _, _ in self.erreurs if t == k) for k in ("INDETERMINISME", "INCOMPLETUDE")}
        L.append(f"  determinisme : {'OK' if not nb['INDETERMINISME'] else str(nb['INDETERMINISME']) + ' erreur(s)'}")
        L.append(f"  completude   : {'OK' if not nb['INCOMPLETUDE'] else str(nb['INCOMPLETUDE']) + ' erreur(s)'}")
        if not m["invariants"]:
            L.append("  invariants   : aucun invariant dans la table")
        for inv, ok in self.resultats:
            L.append(f"  invariant {inv['no']:2d} ({inv['type']}) : {'OK   ' if ok else 'ECHEC'}  {inv['nom']}")
        for n in self.notes:
            L.append("  note : " + n)
        if self.avis:
            L.append("")
            L.append("AVIS")
            for a in self.avis:
                L.append("  " + a)
        if self.erreurs:
            L.append("")
            L.append("ERREURS ET CONTRE-EXEMPLES")
            for i, (typ, texte, pas) in enumerate(self.erreurs, 1):
                L.append(f"  {i}. {typ} : {texte}")
                if pas:
                    L.append(f"     contre-exemple : {self.fmt_chemin(pas)}")
                    L.append(f"     suite d'entrees : {' '.join(x for x, _ in pas)}")
        L.append("")
        L.append(f"RESULTAT : {'OK' if self.ok else 'ECHEC'} — {len(self.erreurs)} erreur(s), {len(self.avis)} avis")
        L.append("Portee : la verification porte sur ce modele fini. Elle ne prouve ni le materiel, ni les")
        L.append("delais reels, ni la traduction des evenements en entrees (fsm.c), qui relevent des essais.")
        return "\n".join(L) + "\n"


# --------------------------------------------------------------------------
# Simulation du modèle
# --------------------------------------------------------------------------
def lire_sequence(chemin, m):
    seq = []
    with open(chemin, encoding="utf-8") as f:
        for no, ligne in enumerate(f, 1):
            mot = ligne.split("#", 1)[0].strip()
            if not mot:
                continue
            if mot not in m["entrees"]:
                raise ErreurTable(f"{chemin}, ligne {no} : entree inconnue \"{mot}\"")
            seq.append(mot)
    return seq


def ligne_trace(i, entree, etat, m):
    sorties = " ".join(f"{s}={m['val'][etat][s]}" for s in m["sorties"])
    return f"{i} {entree} -> {etat} {sorties}"


def simuler(a, seq):
    m = a.m
    etat = m["initial"]
    lignes = [ligne_trace(0, "-", etat, m)]
    for i, x in enumerate(seq, 1):
        s = a.delta.get((etat, x))
        if s is None:
            raise ErreurTable(f"simulation : aucune transition pour ({etat}, {x}) au pas {i}")
        etat = s
        lignes.append(ligne_trace(i, x, etat, m))
    return lignes


# --------------------------------------------------------------------------
# Génération du C
# --------------------------------------------------------------------------
def generer_c(a):
    m = a.m
    somme = a.somme()
    src = os.path.basename(m["chemin"])
    ne, nx = len(m["etats"]), len(m["entrees"])
    idx = {e: i for i, e in enumerate(m["etats"])}

    h = []
    h.append("/* ---------------------------------------------------------------------------")
    h.append(f" * automate_gen.h — GENERE par outils/automate.py {VERSION} a partir de {src}")
    h.append(f" * Automate « {m['nom']} », somme de la table : {somme}")
    h.append(" * NE PAS MODIFIER A LA MAIN : corriger la table JSON, verifier, puis regenerer.")
    h.append(" * Aucun acces materiel : compile sur la cible STM32 et sur hote.")
    h.append(" * ------------------------------------------------------------------------- */")
    h.append("#ifndef AUTOMATE_GEN_H")
    h.append("#define AUTOMATE_GEN_H")
    h.append("")
    h.append("#include <stdint.h>")
    h.append("")
    h.append(f"#define AUTO_SOMME 0x{somme}u      /* empreinte de la table d'origine */")
    h.append("")
    h.append("typedef enum {")
    for i, e in enumerate(m["etats"]):
        h.append(f"    AUTO_ETAT_{e} = {i},")
    h.append(f"    AUTO_N_ETATS = {ne}")
    h.append("} auto_etat_t;")
    h.append("")
    h.append("/* Entrees par priorite decroissante : la premiere est la plus prioritaire. */")
    h.append("typedef enum {")
    for i, x in enumerate(m["entrees"]):
        h.append(f"    AUTO_ENTREE_{x} = {i},")
    h.append(f"    AUTO_N_ENTREES = {nx}        /* sert aussi de valeur « aucune entree » */")
    h.append("} auto_entree_t;")
    h.append("")
    h.append("typedef struct {")
    for s in m["sorties"]:
        h.append(f"    uint8_t {s};")
    h.append("} auto_sorties_t;")
    h.append("")
    h.append(f"#define AUTO_ETAT_INITIAL AUTO_ETAT_{m['initial']}")
    h.append("")
    h.append("/* Etat suivant pour l'entree x; si s != 0, *s recoit les sorties de l'etat")
    h.append(" * suivant. Un etat ou une entree hors domaine laisse l'etat inchange. */")
    h.append("auto_etat_t   automate_transition(auto_etat_t e, auto_entree_t x, auto_sorties_t *s);")
    h.append("/* Sorties de l'etat e (automate de Moore : les sorties ne dependent que de l'etat). */")
    h.append("void          automate_sorties(auto_etat_t e, auto_sorties_t *s);")
    h.append("/* Entree la plus prioritaire d'un masque (bit i = entree numero i);")
    h.append(" * AUTO_N_ENTREES si le masque est vide. */")
    h.append("auto_entree_t automate_choisir(uint32_t masque);")
    h.append("const char   *automate_nom_etat(auto_etat_t e);")
    h.append("const char   *automate_nom_entree(auto_entree_t x);")
    h.append("const char   *automate_nom_sortie(uint32_t i);      /* 0 si i >= AUTO_N_SORTIES */")
    h.append("uint32_t      automate_valeur_sortie(const auto_sorties_t *s, uint32_t i);")
    h.append(f"#define AUTO_N_SORTIES {len(m['sorties'])}u")
    h.append("/* Autotest : compare automate_transition et automate_sorties a une seconde")
    h.append(" * copie de la table, pour chaque couple (etat, entree). Retourne le nombre")
    h.append(" * d'ecarts (0 = succes). Commande serie g sur la cible. */")
    h.append("uint32_t      automate_autotest(void);")
    h.append("")
    h.append("#endif /* AUTOMATE_GEN_H */")

    c = []
    c.append("/* ---------------------------------------------------------------------------")
    c.append(f" * automate_gen.c — GENERE par outils/automate.py {VERSION} a partir de {src}")
    c.append(f" * Automate « {m['nom']} », somme de la table : {somme}")
    c.append(" * NE PAS MODIFIER A LA MAIN : corriger la table JSON, verifier, puis regenerer.")
    c.append(" * ------------------------------------------------------------------------- */")
    c.append("#include \"automate_gen.h\"")
    c.append("")
    c.append("/* table[etat][entree] = etat suivant */")
    c.append("static const uint8_t table[AUTO_N_ETATS][AUTO_N_ENTREES] = {")
    for e in m["etats"]:
        c.append("    /* %-18s */ { %s }," % (e, ", ".join("%3d" % idx[a.delta[(e, x)]] if (e, x) in a.delta
                                                          else "%3d" % idx[e] for x in m["entrees"])))
    c.append("};")
    c.append("")
    c.append("static const auto_sorties_t sorties[AUTO_N_ETATS] = {")
    for e in m["etats"]:
        c.append("    /* %-18s */ { %s }," % (e, ", ".join(str(m["val"][e][s]) for s in m["sorties"])))
    c.append("};")
    c.append("")
    c.append("static const char *const noms_etats[AUTO_N_ETATS] = {")
    c.append("    " + ", ".join(f"\"{e}\"" for e in m["etats"]))
    c.append("};")
    c.append("static const char *const noms_entrees[AUTO_N_ENTREES] = {")
    c.append("    " + ", ".join(f"\"{x}\"" for x in m["entrees"]))
    c.append("};")
    c.append("static const char *const noms_sorties[AUTO_N_SORTIES] = {")
    c.append("    " + ", ".join(f"\"{s}\"" for s in m["sorties"]))
    c.append("};")
    c.append("")
    c.append("void automate_sorties(auto_etat_t e, auto_sorties_t *s)")
    c.append("{")
    c.append("    if (s != 0 && (uint32_t)e < AUTO_N_ETATS) {")
    c.append("        *s = sorties[e];")
    c.append("    }")
    c.append("}")
    c.append("")
    c.append("auto_etat_t automate_transition(auto_etat_t e, auto_entree_t x, auto_sorties_t *s)")
    c.append("{")
    c.append("    if ((uint32_t)e < AUTO_N_ETATS && (uint32_t)x < AUTO_N_ENTREES) {")
    c.append("        e = (auto_etat_t)table[e][x];")
    c.append("    }")
    c.append("    automate_sorties(e, s);")
    c.append("    return e;")
    c.append("}")
    c.append("")
    c.append("auto_entree_t automate_choisir(uint32_t masque)")
    c.append("{")
    c.append("    for (uint32_t i = 0u; i < AUTO_N_ENTREES; i++) {")
    c.append("        if (masque & (1u << i)) {")
    c.append("            return (auto_entree_t)i;")
    c.append("        }")
    c.append("    }")
    c.append("    return AUTO_N_ENTREES;")
    c.append("}")
    c.append("")
    c.append("const char *automate_nom_etat(auto_etat_t e)")
    c.append("{")
    c.append("    return ((uint32_t)e < AUTO_N_ETATS) ? noms_etats[e] : \"?\";")
    c.append("}")
    c.append("")
    c.append("const char *automate_nom_entree(auto_entree_t x)")
    c.append("{")
    c.append("    return ((uint32_t)x < AUTO_N_ENTREES) ? noms_entrees[x] : \"-\";")
    c.append("}")
    c.append("")
    c.append("const char *automate_nom_sortie(uint32_t i)")
    c.append("{")
    c.append("    return (i < AUTO_N_SORTIES) ? noms_sorties[i] : 0;")
    c.append("}")
    c.append("")
    c.append("uint32_t automate_valeur_sortie(const auto_sorties_t *s, uint32_t i)")
    c.append("{")
    c.append("    switch (i) {")
    for i, s in enumerate(m["sorties"]):
        c.append(f"    case {i}u: return s->{s};")
    c.append("    default: return 0u;")
    c.append("    }")
    c.append("}")
    c.append("")
    c.append("/* Seconde copie de la table, sous une autre forme : triplets (etat, entree,")
    c.append(" * etat suivant) et somme des sorties de l'etat suivant. */")
    c.append("typedef struct { uint8_t e, x, suivant, somme_sorties; } auto_ref_t;")
    c.append("static const auto_ref_t reference[] = {")
    ligne = []
    for e in m["etats"]:
        for x in m["entrees"]:
            s = a.delta.get((e, x), e)
            som = sum(m["val"][s].values()) & 0xFF
            ligne.append("{%d,%d,%d,%d}" % (idx[e], m["entrees"].index(x), idx[s], som))
            if len(ligne) == 6:
                c.append("    " + ", ".join(ligne) + ",")
                ligne = []
    if ligne:
        c.append("    " + ", ".join(ligne) + ",")
    c.append("};")
    c.append("")
    c.append("uint32_t automate_autotest(void)")
    c.append("{")
    c.append("    uint32_t ecarts = 0u;")
    c.append("    for (uint32_t i = 0u; i < sizeof reference / sizeof reference[0]; i++) {")
    c.append("        auto_sorties_t s;")
    c.append("        uint32_t somme = 0u;")
    c.append("        auto_etat_t suivant = automate_transition((auto_etat_t)reference[i].e,")
    c.append("                                                  (auto_entree_t)reference[i].x, &s);")
    c.append("        for (uint32_t k = 0u; k < AUTO_N_SORTIES; k++) {")
    c.append("            somme += automate_valeur_sortie(&s, k);")
    c.append("        }")
    c.append("        if ((uint32_t)suivant != reference[i].suivant || (somme & 0xFFu) != reference[i].somme_sorties) {")
    c.append("            ecarts++;")
    c.append("        }")
    c.append("    }")
    c.append("    return ecarts;")
    c.append("}")
    return "\n".join(h) + "\n", "\n".join(c) + "\n"


# --------------------------------------------------------------------------
# Commandes
# --------------------------------------------------------------------------
def ecrire(chemin, texte):
    d = os.path.dirname(chemin)
    if d:
        os.makedirs(d, exist_ok=True)
    with open(chemin, "w", encoding="utf-8", newline="\n") as f:
        f.write(texte)


def cmd_verifier(args):
    a = Analyse(charger(args.table))
    r = a.rapport()
    sys.stdout.write(r)
    if args.rapport:
        ecrire(args.rapport, r)
        print(f"rapport ecrit dans {args.rapport}")
    return 0 if a.ok else 1


def cmd_generer(args):
    a = Analyse(charger(args.table))
    if not a.ok:
        sys.stdout.write(a.rapport())
        print("generation REFUSEE : la table ne passe pas la verification.")
        return 1
    inc = args.inc or args.dossier or "."
    src = args.src or args.dossier or "."
    h, c = generer_c(a)
    ecrire(os.path.join(inc, "automate_gen.h"), h)
    ecrire(os.path.join(src, "automate_gen.c"), c)
    print(f"verification OK ({len(a.acc)} etats accessibles, {len(a.m['entrees'])} entrees, "
          f"{len(a.m['invariants'])} invariants), somme {a.somme()}")
    print(f"ecrit : {os.path.join(inc, 'automate_gen.h')}")
    print(f"ecrit : {os.path.join(src, 'automate_gen.c')}")
    return 0


def cmd_simuler(args):
    a = Analyse(charger(args.table))
    lignes = simuler(a, lire_sequence(args.sequence, a.m))
    texte = "\n".join(lignes) + "\n"
    if args.sortie:
        ecrire(args.sortie, texte)
    else:
        sys.stdout.write(texte)
    return 0


def cmd_comparer(args):
    def lire(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            return [l.rstrip() for l in f.read().splitlines() if l.strip()]
    a, b = lire(args.a), lire(args.b)
    for i in range(max(len(a), len(b))):
        la = a[i] if i < len(a) else "(fin)"
        lb = b[i] if i < len(b) else "(fin)"
        if la != lb:
            print(f"ECART a la ligne {i + 1} :\n  {args.a} : {la}\n  {args.b} : {lb}")
            print("RESULTAT : ECHEC")
            return 1
    print(f"{len(a)} lignes identiques : traces du modele et du code genere concordantes")
    print("RESULTAT : OK")
    return 0


def main():
    p = argparse.ArgumentParser(description="Verificateur et generateur d'automates finis (CEG 3536)")
    sp = p.add_subparsers(dest="commande")
    v = sp.add_parser("verifier", help="controler determinisme, completude et invariants")
    v.add_argument("table")
    v.add_argument("--rapport", help="fichier ou ecrire le rapport")
    g = sp.add_parser("generer", help="verifier puis ecrire automate_gen.h et automate_gen.c")
    g.add_argument("table")
    g.add_argument("--dossier", help="dossier des deux fichiers generes")
    g.add_argument("--inc", help="dossier de automate_gen.h (prioritaire sur --dossier)")
    g.add_argument("--src", help="dossier de automate_gen.c (prioritaire sur --dossier)")
    s = sp.add_parser("simuler", help="executer le modele sur une suite d'entrees")
    s.add_argument("table")
    s.add_argument("sequence")
    s.add_argument("--sortie", help="fichier de la trace (sinon sortie standard)")
    c = sp.add_parser("comparer", help="comparer deux traces")
    c.add_argument("a")
    c.add_argument("b")
    args = p.parse_args()
    if not args.commande:
        p.print_help()
        return 2
    try:
        return {"verifier": cmd_verifier, "generer": cmd_generer,
                "simuler": cmd_simuler, "comparer": cmd_comparer}[args.commande](args)
    except ErreurTable as e:
        print(f"ERREUR DE TABLE : {e}")
        print("RESULTAT : ECHEC")
        return 2


if __name__ == "__main__":
    for flux in (sys.stdout, sys.stderr):
        try:
            flux.reconfigure(encoding="utf-8")
        except (AttributeError, ValueError):
            pass
    sys.exit(main())
