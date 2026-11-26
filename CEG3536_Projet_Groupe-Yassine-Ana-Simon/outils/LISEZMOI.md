# automate.py — vérificateur et générateur d'automates finis

Outil pédagogique du projet final de CEG 3536 (exigence E15, composante L5-B).
Python 3.8 ou plus récent, bibliothèque standard seulement. **Ne pas modifier
l'outil** : s'il semble fautif, le signaler à l'équipe enseignante. L'outil est
une source à citer dans le rapport.

```
python outils/automate.py verifier  table.json [--rapport rapport.txt]
python outils/automate.py generer   table.json [--dossier D | --inc Core/Inc --src Core/Src]
python outils/automate.py simuler   table.json sequence.txt [--sortie trace.txt]
python outils/automate.py comparer  trace_a.txt trace_b.txt
```

| Commande | Effet | Code de retour |
|---|---|---|
| `verifier` | Explore tous les états accessibles et toutes les entrées ; contrôle le déterminisme, la complétude et les invariants ; donne un contre-exemple pour chaque erreur | 0 si tout passe, 1 sinon, 2 si la table est illisible |
| `generer` | Refait la vérification, puis écrit `automate_gen.h` et `automate_gen.c` ; **rien n'est écrit si la vérification échoue** | 0 ou 1 |
| `simuler` | Exécute le **modèle** sur une suite d'entrées et écrit la trace | 0 |
| `comparer` | Compare deux traces ligne à ligne | 0 si identiques |

## Format de la table

Un fichier JSON, encodé en UTF-8. Les noms d'états et d'entrées sont en
majuscules non accentuées (`ARRET_URGENCE`), les noms de sorties en minuscules
(`moteur`) : ils deviennent des identificateurs C.

```json
{
  "nom": "ventilateur",
  "entrees": ["SURCHAUFFE", "ACQUIT", "BOUTON"],
  "sorties": ["moteur", "alarme"],
  "initial": "ARRET",
  "etats": {
    "ARRET":  { "moteur": 0, "alarme": 0 },
    "MARCHE": { "moteur": 1, "alarme": 0 },
    "ALARME": { "moteur": 0, "alarme": 1 }
  },
  "transitions": [
    { "de": "*",      "sur": "SURCHAUFFE", "vers": "ALARME" },
    { "de": "ARRET",  "sur": "BOUTON",     "vers": "MARCHE" },
    { "de": ["ARRET", "MARCHE"], "sur": "ACQUIT", "vers": "=" }
  ],
  "invariants": [ ... ]
}
```

| Clé | Contenu |
|---|---|
| `nom` | Nom de l'automate (repris dans le rapport et dans le code généré) |
| `entrees` | Liste **ordonnée par priorité décroissante** : la première entrée est la plus prioritaire. Au plus 32 entrées |
| `sorties` | Liste des sorties ; chaque état donne une valeur entière de 0 à 255 à chacune |
| `initial` | État initial |
| `etats` | Un objet par état : la valeur de **toutes** les sorties dans cet état (automate de Moore : les sorties ne dépendent que de l'état) |
| `transitions` | Liste de règles `de`, `sur`, `vers` (ci-dessous) |
| `invariants` | Liste de propriétés à vérifier (ci-dessous) ; facultative, mais exigée par E15 |

Toute autre clé (`description`, `auteurs`, …) est ignorée : elle sert de
commentaire, JSON n'en ayant pas.

### Transitions

- `de` : un état, une liste d'états, ou `"*"` (tous les états).
- `sur` : une entrée, une liste d'entrées, ou `"*"` (toutes les entrées).
- `vers` : un état, ou `"="` pour rester dans l'état de départ.
- Une règle **explicite** (état nommé dans `de`) l'emporte sur une règle
  **générale** (`"de": "*"`) pour le même couple (état, entrée). L'outil le
  signale par une note : vérifier que c'est bien voulu.
- Deux règles de même nature qui donnent deux états d'arrivée différents au
  même couple sont un **indéterminisme** : erreur.
- Un couple (état accessible, entrée) sans aucune règle est une
  **incomplétude** : erreur. Une entrée sans effet s'écrit donc explicitement,
  avec `"vers": "="`.

### Invariants

Chaque invariant a un `nom` (le texte qui apparaît dans le rapport) et un `type`.

| `type` | Clés | Propriété vérifiée |
|---|---|---|
| `sortie` | `etats`, `sortie`, `valeur` | Dans chacun des états cités, la sortie a cette valeur |
| `sortie` | `si`, `alors` | Dans tout état accessible où les sorties de `si` ont ces valeurs, celles de `alors` ont les leurs : `"si": {"alarme": 1}, "alors": {"moteur": 0}` |
| `interdit` | `de`, `vers`, `sur` (facultatif) | Aucune transition ne mène directement d'un état de `de` à un état de `vers` (pour les entrées de `sur`, ou pour toutes) |
| `toujours` | `sur`, `vers`, `de` ou `sauf` (facultatifs) | Depuis tout état accessible (ou ceux de `de`, ou tous sauf ceux de `sauf`), les entrées de `sur` mènent à `vers` |
| `retour` | `vers` | Depuis tout état accessible, l'état `vers` reste atteignable : aucun état piège |
| `accessible` | `etats` (facultatif) | Les états cités (tous, par défaut) sont accessibles depuis l'état initial |

## Lire le rapport

Le rapport donne la table complète (un point pour « rester », un point
d'interrogation pour une transition absente), les sorties de chaque état, le
résultat de chaque contrôle, puis, pour chaque erreur, un **contre-exemple** :
le plus court chemin depuis l'état initial jusqu'à la situation fautive, et la
suite d'entrées qui le reproduit.

```
ERREURS ET CONTRE-EXEMPLES
  1. INVARIANT : « aucune reprise sans acquittement » : transition interdite ALARME --BOUTON--> MARCHE (transition n. 4)
     contre-exemple : ARRET --SURCHAUFFE--> ALARME --BOUTON--> MARCHE
     suite d'entrees : SURCHAUFFE BOUTON
```

La **somme** (huit chiffres hexadécimaux) est une empreinte de la table. Elle
est reprise dans le code généré (`AUTO_SOMME`) : le rapport, le code et le
projet programmé sur la carte se rattachent ainsi à la même version de la table.

Le rapport rappelle la **portée** de la vérification : elle porte sur le modèle
fini. Elle ne prouve ni le matériel, ni les délais, ni la traduction des
événements en entrées, qui relèvent des essais.

## Prise en main (avant le jalon J3)

Depuis le dossier `hote/` du projet :

1. `python ../outils/automate.py verifier automate/exemple.json` : le rapport
   signale **une** erreur, avec son contre-exemple. Lire le chemin, retrouver la
   transition fautive dans la table.
2. Corriger la table (une seule ligne), vérifier de nouveau : `RESULTAT : OK`.
   Conserver la version fautive et son rapport : c'est le modèle du
   contre-exemple demandé à la section 4.10 de l'énoncé.
3. `make automate` : génération dans `automate/gen/`, compilation de
   `test_automate`, trace du modèle, trace du code généré, comparaison.
   Sans `make`, les cinq commandes figurent dans `hote/Makefile`.
4. Retirer une transition de la table, puis donner deux états d'arrivée au
   même couple : observer les erreurs d'incomplétude et d'indéterminisme.

## Code généré

`automate_gen.h` et `automate_gen.c` ne sont **jamais modifiés à la main** :
on corrige la table, on vérifie, on régénère. Ils n'accèdent à aucun registre
et se compilent sur la cible et sur hôte.

```c
auto_etat_t   automate_transition(auto_etat_t e, auto_entree_t x, auto_sorties_t *s);
void          automate_sorties(auto_etat_t e, auto_sorties_t *s);
auto_entree_t automate_choisir(uint32_t masque);   /* entrée la plus prioritaire */
const char   *automate_nom_etat(auto_etat_t e);
const char   *automate_nom_entree(auto_entree_t x);
uint32_t      automate_autotest(void);             /* 0 = succès ; commande g   */
```

- Les états sont `AUTO_ETAT_<NOM>`, les entrées `AUTO_ENTREE_<NOM>`, numérotés
  dans l'ordre de la table ; `auto_sorties_t` a un champ `uint8_t` par sortie.
- `automate_choisir(masque)` rend l'entrée la plus prioritaire parmi celles
  dont le bit est à 1 (bit i = entrée numéro i), ou `AUTO_N_ENTREES` si le
  masque est vide. C'est le moyen d'appliquer les priorités de la section 4.6
  lorsque plusieurs conditions sont vraies au même instant.
- `automate_autotest()` compare la fonction de transition à une seconde copie
  de la table, pour tous les couples (état, entrée) : il détecte un fichier
  généré retouché ou une mémoire programme altérée.

Pour la cible, depuis la racine du projet :

```
python outils/automate.py generer hote/automate/automate.json --inc Core/Inc --src Core/Src
```

## Du modèle au véhicule

Le modèle est fini : le temps, la vitesse et la distance n'y figurent pas.
`fsm.c` garde trois responsabilités, et elles seules :

1. **traduire** chaque événement de la file, avec ses gardes, en une entrée du
   modèle (par exemple : `EV_COMMUT` de SW1 en mode AUTONOME devient l'entrée
   « feu rouge » ; « trajet fini » devient l'entrée de fin) ;
2. **appeler** `automate_transition` ;
3. **appliquer** les sorties de l'état obtenu, en un seul endroit (`fsm_sorties`).

Les états du modèle (INIT, PRÊT, MANUEL, AUTONOME, ARRÊT_ÉVÉNEMENT,
ARRÊT_URGENCE, DÉFAUT) ne sont pas les valeurs de la colonne `<etat>` de la
télémétrie : la correspondance (par exemple MANUEL ou AUTONOME avec le sens
choisi donne `ETAT_MARCHE_AVANT` ou `ETAT_MARCHE_ARRIERE`) s'écrit dans
`fsm.c` et se documente dans le rapport.
