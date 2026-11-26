# JOURNAL.md — Journal d'équipe, CEG 3536, projet final (automne 2026)

Équipe : `Ana` et `Simon` et `Yassine` — Section : ` A02` — Dépôt Git : `<adresse>`
— Variante L4-A attribuée : `<0 à 3>` — Journal du laboratoire 3 : `JOURNAL_Lab3.md`

# Partie I

## Partie I — Rôles et rotation

| Séance | Réalise | Valide | Pilote I²C écrit par chacun | Pilote SPI ou routine assembleur de chacun |
|---|---|---|---|---|
| Séance 1 |Ana:début du programme pour la partie 1 Simon:Début du rapport de laboratoire Yassine: Début du journal.md  |tous |- | -|
| Entre les séances |Yassine |Les test de la première parti du laboratoire on été effectué soit t1-t10 |aucun |aucun |
| Séance 2 |Tous |Tous |i2c.c, ioexp.c, lcd.c, touchpad.c |spi.c, env.c, somme.s, exti_l4.s |

## Partie I — Calculs préalables

| Grandeur | Formule | Valeur | Vérifiée par |
|---|---|---|---|
| Pas de l'horloge I²C | (PRESC + 1) / 4 MHz | 250ns| T1 |
| SCL bas / SCL haut | (SCLL + 1) × pas / (SCLH + 1) × pas |5µs | T1 |
| Fréquence SCL (avec les délais de synchronisation) |1/ (SCL bas + SCL haut) |100 kHz | T1 |
| `TIMINGR` retenu |PRESC=0, SCLDEL=4, SDADEL=2, SCLH=0x0F, SCLL=0x13 |0x00420F13 | T1 |
| Durée d'une écriture de deux octets de données | 27 bits / fréquence SCL |270µs | T1 (DWT) |
| Débit SPI      | PCLK2 / 2^(BR + 1) |1MHz | T7 |
| Pire cas de réécriture du LCD |32 caractères × (position+ caractère) × 1 tick |64 ticks | T3 |
| Transactions I²C par seconde au repos |1 sur TouchPad/s et 100 sur le commutateur | ≈101/s| T10 |

## Partie I — Relevés des modules

| Module | Relevé | Valeur | Preuve (fichier) |
|---|---|---|---|
| Bus I²C | Adresses trouvées par `i2c_scan`    |   0x38, 0x39, 0x3E, 0X5A, 0x2D | |
| Bus I²C | `GPIOF->IDR` bits 0 et 1 au repos ; `I2C2->ISR` après une transaction | | |
| Extenseur des DEL | Octet écrit pour allumer Y1 seule ; R4 seule |    0x7F, 0xFE | |
| Extenseur des commutateurs | Octet lu au repos ; SW1 enfoncé ; SW8 enfoncé | 0x7F, 0xFE, 0xFF | |
| LCD | Octets de commande confirmés (fonction, allumage, effacement, mode) |0x01,0x06, 0x0C, 0x28 | |
| Rétroéclairage | Registres rouge / vert / bleu confirmés | 0x01, 0x02, 0x03 | |
| TouchPad | Front actif de THI ; `0x5E` relu après `tp_init` ; deux seuils relus | | |
| Capteur environnemental | Identifiant lu ; page utilisée pour chaque registre |env_id = 97/0x61 |T7 - TEST(t1-t18).pdf |
| Capteur environnemental | Température / humidité / pression au repos ; écart avec le capteur analogique  |25.1 C / 1021.7hPa / 22,4%|T7 - TEST(t1-t18).pdf|
| PIR | Durée du niveau haut ; `pir_t_ms` de deux fronts successifs | | |2

Table des touches (électrode → touche), relevée au débogueur :

| Électrode | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Touche |0|1|2|3|4|5|6|7|8|9|SHIFT|NEXT/BACK(<->) |

## Partie I — Essais hôte (E11)

| Plateforme | Compilateur et version | `test_evq` (réussis / échecs) | `test_tableau` (réussis / échecs) | Date et validation Git |
|---|---|---|---|---|
| Windows |gcc 16.2.0 (MinGW-w64)|9 / 0 |6 / 0 |24 novembre 2026 |
| Linux | 13.3.0 (ubuntu)| 9/0| 6/0|24 novembre 2026 |
| STM32 (cible) | arm-none-eabi-gcc | commande `q` :9/0 | commande `f` :6/0 | 24 novembre 2026|

Cas ajoutés à `tableau_test.c` (au-delà des 6 fournis) :

| Cas | Description | Résultat |
|---|---|---|
| 7 | MARCHE_ARRIÈRE en mode MANUEL | réussi |
| 8 |ARRÊT_ÉVÉNEMENT, cause DEFAUT | réussi |
| 9 | ARRÊT, cause FIN| réussi |
| 10 |PAGE_VERROU |  réussi|
| 11 |Mesure environnementale non valide | réussi |
| 12 |Température négative  | réussi |
| 13 | Vitesse à 4 chiffres | réussi |


## Partie I — Banc de `trame_xor8` (T9)

| Cas | Attendu | Obtenu | Verdict |
|---|---|---|---|
| Chaîne vide | 0x00 | | |
| « A » | 0x41 | | |
| « T,0 » | 0x48 | | |
| « 123456789 » | 0x31 | | |
| Octets 0x00 à 0xFF | 0x00 | | |

## Partie I — Journal des séances

### Séance 1 — `17 novembre 2026` — réalise : `Ana, Simon, Yassine` / valide : `Ana, Simon, Yassine`
- Objectifs :Commencer le code pour la partie 1, début du journal.md et commencer le rapport de laboratoire
- Fait :  Code pour la partie 1, avancement dans le rapport de laboratoire et le journal
- Décisions (découpage, répartition du bus, format des lignes) :
- Difficultés et solutions :  On a eu de la difficulté à importer le projet dans STM32CubeIDE. On a réglé le problème en important les launch files d'un laboratoire précédent.
- Mesures : Aucune mesure cette séance
- Validations Git :  Ana, "Initial commit for project folder from the Brightspace Complement zip"

### Entre les séances — travail sur hôte
- Fait (tableau.c, test_tableau, fsm.c, lecture de l4a_defaut.c) :
- Validations Git :

### Séance 2 — `24 novembre 2026` — réalise : `Ana, Simon, Yassine` / valide : `Ana, Simon, Yassine`
- Objectifs :Démonstration de la partie I, devant l'assistant, commencer la partie II, continuer le journal d'équipe,  commencer le rapport de laboratoire, planifier la vidéo de présentation.
- Fait :Démonstration de la partie I devant le TA, Journal d'équipe mis à jour, Début du rapport de laboratoire
- Décisions : Poursuite sur le code de départ fourni (complément du projet) plutôt que substitution du code du laboratoire 3 
- Difficultés et solutions: Aucun compilateur C sur l'hôte Windows (Yassine)(gcc absent). Solution: installation de MSYS2/MinGW-w64 (gcc 16.2.0)
- Mesures (capteur, PIR, télémétrie, L4-A): Investigation du défaut TouchPad/I²C menée — symptôme, hypothèses classées, mesures au débogueur (`i2c_scan`, `GPIOF->IDR`, `I2C2->ISR`, relecture de `0x5E`, `thi_flag`)
- Validations Git :

## Partie I — Composante L4-A : diagnostic d'un défaut TouchPad/I²C

Variante : `<n>` — validation Git de la version défectueuse : — de la version corrigée :

1. Symptôme (attendu, observé, conditions de reproduction) : Quand on remplace tp_init() et tp_lire() par l4a_tp_init() et l4a_tp_lire() (en mettant L4A_ESSAI à 1 dans main.c), le TouchPad ne fonctionne pas correctement. Selon la variante, soit aucune touche n'est détectée, soit seulement quelques-unes, soit la mauvaise touche est rapportée.

2. Hypothèses, classées avant toute mesure :

| Rang | Hypothèse | Niveau (lignes et broches / adressage et protocole / configuration du composant / interruption et garde) | Ce qu'on observerait si elle était vraie |
|---|---|---|---|
| 1 |Mauvaise adresse I²C (pas sur 7 bits) | adressage et protocole |i2c_scan ne trouve pas le composant |
| 2 | Registre 0x5E mal écrit |configuration du composant |seulement quelques touches répondent |
| 3 |Octets d'état lus dans le mauvais ordre |lignes et broches (lecture) |la mauvaise touche est rapportée |

3. Mesures:

| Hypothèse visée | Mesure (registre, balayage, relecture, compteur) | Résultat | Preuve (fichier) |
|---|---|---|---|
| | | | |

4. Essai discriminant — prédiction écrite **avant** l'essai (date et heure) :

   Résultat de l'essai :

5. Interprétation (hypothèses écartées, hypothèse retenue, pourquoi) :

6. Correction et non-régression (les 12 touches, la garde Touch En) :

7. Conclusion et limites (ce qui est démontré, ce qui est supposé, ce qui reste à vérifier) :

## Partie I — Table des états et des priorités (en vue du jalon J3)

 **ARRÊT** — C'est l'état de départ, le moteur est arrêté et la DEL rouge est allumée
 fixe. En premier, l'E-Stop est toujours prioritaire et il nous envoie en ARRÊT_URGENCE.
 Ensuite, un défaut (un module qui ne répond plus), le PIR qui détecte un mouvement ou le
 feu rouge (SW1) nous envoient en ARRÊT_ÉVÉNEMENT avec la bonne cause. Le bouton User fait
 démarrer le véhicule : en MARCHE_AVANT la première fois, puis en MARCHE_ARRIÈRE la fois
 suivante, en alternance. Le TouchPad et la consigne du potentiomètre ne changent pas
 l'état, ils mettent juste à jour des valeurs. Il y a un bip au moment de démarrer.
 
**MARCHE_AVANT** — Le moteur tourne en avant avec la rampe et la DEL verte est allumée.
 L'E-Stop est toujours prioritaire et nous envoie en ARRÊT_URGENCE. Un défaut, le PIR
 (obstacle) ou le feu rouge (SW1) nous envoient en ARRÊT_ÉVÉNEMENT avec la bonne cause. Le
 bouton User nous ramène à l'ARRÊT. Le reste (consigne, vitesse, TouchPad) ne change pas
 l'état, ça met seulement à jour les valeurs.
 
**MARCHE_ARRIÈRE** — C'est exactement comme MARCHE_AVANT, mais le moteur tourne en
 arrière et la DEL bleue est allumée.
 
**ARRÊT_URGENCE** — Le moteur est coupé, la DEL rouge clignote à 2 Hz et le buzzer sonne
 en continu. Le seul événement utile est le TouchPad pour acquitter, et seulement si le
 bouton E-Stop est relâché. Si on acquitte, on revient à l'ARRÊT. Le bouton User et le PIR
 sont ignorés.
 
**ARRÊT_ÉVÉNEMENT** — Le moteur est arrêté et la DEL allumée dépend de la cause (R1 pour
 le feu, R2 pour l'obstacle, R3 pour le défaut), le message sur l'écran LCD aussi. L'E-Stop
 nous envoie en ARRÊT_URGENCE. Si la cause disparaît (le feu repasse au vert avec SW1
 relâché, ou l'obstacle disparaît avec le PIR qui retombe), on repart en MARCHE_AVANT en
 mode autonome, ou on revient à l'ARRÊT en mode manuel. Pour un défaut, on acquitte avec le
 TouchPad et on revient à l'ARRÊT.

**Priorités des interruptions** — L'E-Stop a la priorité 0 parce que c'est la sécurité,
 il doit couper le moteur tout de suite. L'encodeur a la priorité 1 pour ne pas perdre de
 fronts (environ 4400 fronts par seconde). Le SysTick a la priorité 2 parce que c'est la
 base de temps du programme. Le PIR a aussi la priorité 2 parce que c'est un arrêt de
 sécurité, mais il est filtré sur 50 ms. Le THI a la priorité 3 parce que c'est juste une
 demande du TouchPad.

## Partie I — Tableau des essais (T1 à T10)

| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Bus I²C et balayage | | | | |
| T2 Extenseurs | | | | |
| T3 LCD et rétroéclairage | | | | |
| T4 TouchPad et garde | | | | |
| T5 Commandes du TouchPad | | | | |
| T6 PIR | | | | |
| T7 Capteur environnemental | | | | |
| T8 Événements et priorités | | | | |
| T9 Télémétrie contrôlée | | | | |
| T10 Robustesse et non-régression | | | | |

# Partie II

## Partie II — Rôles et rotation

| Période | Réalise | Valide | Module sans matériel de chacun | Routine assembleur ou intégration de chacun |
|---|---|---|---|---|
| Entre les séances 1 et 2 (prise en main de l'outil) |Tous | Tous|-|-|
| Séance 2 (24 / 27 nov.) |Ana, Simon, Yassine |Tous |trajet.c (Ana), regul.c (Yassine), surv.c/essaie.c (Simon) |crit.s (Ana), evq.c (Yassine), fsm.c (Simon) |
| Jalon J3 (27 nov.) | | | | |
| Entre les séances 2 et 3 |Tous |Tous |essaie (t10-t18) |aucun |
| Séance 3 (1er / 4 déc.) |Tous |Tous |- |- |
| Démonstration finale (4 / 8 déc.) |Tous | Tous| -|- |

## Partie II — Calculs préalables

| Grandeur | Formule | Valeur | Vérifiée par |
|---|---|---|---|
| Fronts par fenêtre à 100 tr/min | 100 × 748 / 600 |≈124,7 fronts | T14 |
| Résolution de la vitesse à 100 tr/min | 1 front / fronts par fenêtre |≈0,8% | T14 |
| Constante Q16.16 de la fenêtre de 100 ms | 600 × 65 536 / 748, arrondie |52 569 | banc du lab. 3 |
| Fronts du trajet de référence | 105 × 748 |78 540 fronts | T13 |
| Durée prévue du trajet de référence | somme des tours / (tr/min / 60) |≈35,6s | T13 |
| Commande initiale du régulateur à 150 tr/min | 150 × 100 / 340 |44% | T14 |
| Durée de masquage admise | 200 cycles / 4 MHz |50 µs | T11 |
| Occupation de la file au pire cas prévu |rafale `e`:40 événements |32 | T11 |

## Partie II — Automate (L5-B)

| Date | Version de la table (somme) | Résultat du vérificateur | Erreurs et contre-exemples | Correction |
|---|---|---|---|---|
|1 decembre 2026 |correct | |Aucun |- |

Invariants imposés et ajoutés par l'équipe :

| Invariant | Type | Pourquoi | Résultat |
|---|---|---|---|
| 'moteur autorisé' faux dans INIT, PRÊT, ARRÊT_ÉVÉNEMENT, ARRÊT_URGENCE |sortie |sécurité : aucun état d'arrêt/défaut n'actionne la propulsion | -|
|Aucune transition d'un arrêt/défaut vers MANUEL ou AUTONOME|interdit|la reprise passe toujours par PRÊT, après acquittement|---|
| E-Stop mène à ARRÊT_URGENCE depuis tous les états| toujours|l'arrêt d'urgence est prioritaire partout|---|
|Un changement de sens ne se fait que via EN|interdit|règle du laboratoire 2 (CCR1 = 0 avant de changer de sens)|---|
|Entrées traitées par priorité|---|un seul événement retenu quand plusieurs sont simultanés|---|
|Depuis tout état accessible, PRÊT reste atteignable|retour|le véhicule ne reste jamais bloqué|---|

Version fautive conservée : fichier `…`, validation Git `…`, contre-exemple :

```
(copier ici le contre-exemple produit par l'outil)
```

Correspondance entre les événements de la file et les entrées du modèle
(traduction faite dans `fsm.c`) :

| Événement, garde | Entrée du modèle |
|---|---|
|EV_ESTOP  |E-STOP  |
| EV_USER | BOUTON |
| EV_TOUCHE (Fn) | ACQUIT |
| EV_TOUCHE (Next/Back), en PRÊT | MODE |
| EV_PIR / EV_COMMUT SW2 | OBSTACLE |
| EV_COMMUT SW1 | FEU ROUGE |
| EV_COMMUT SW3 / humidité > seuil | LIMITATION |
| EV_COMMUT SW4 / trajet_fini() | FIN |
| EV_COMMUT SW5 / module muet / surv_code() | DEFAUT |
| EV_THERMIQUE (SURV_THERM_ARRET) | THERMIQUE |
| EV_ESSAI | ESSAI |

Comparaison des traces (au moins trois séquences) :

| Séquence | Pas | Hôte Windows | Hôte Linux | Cible (commande `g`) |
|---|---|---|---|---|
| | | | | |

## Partie II — File d'événements et sections critiques (L5-A)

| Question | Réponse de l'équipe | Preuve |
|---|---|---|
| Producteurs (gestionnaires) et consommateur | | |
| Instructions placées dans la section critique | | |
| Politique de débordement retenue, et pourquoi | | |
| Mécanisme de garantie pour E-Stop et défaut | | |
| `crit_autotest` (commande `c`) : cas réussis | | |
| `crit_max_cycles` relevé (repos, scénario S3, rafale) | | |
| `evq_max` relevé (repos, scénario S3, rafale) | | |
| Rafale `e` : événements acceptés, refusés, consommés | | |
| Délai de E-Stop pendant une rafale | | |

## Partie II — Réglage du régulateur

Consigner chaque essai, y compris ceux qui échouent : c'est la justification
des gains retenus.

| Essai | Kp (Q8) | Ki (Q8) | Consigne (tr/min) | Vitesse à 2 s | Erreur (%) | Dépassement | Observation |
|---|---|---|---|---|---|---|---|
| 1 | 26 | 26 | | | | | |
| | | | | | | | |

Gains retenus : Kp = `…`, Ki = `…` — raison :

## Partie II — Relevés de la partie II

| Essai | Relevé | Valeur | Preuve (fichier) |
|---|---|---|---|
| T13 | Distance aux changements de segment (20, 60, 75) et à la fin (105) | | |
| T13 | Reprise après un arrêt au feu rouge : distance avant / après | | |
| T14 | Erreur 2 s après chaque changement de consigne (quatre segments) | | |
| T14 | Plafond abaissé pendant S2 : commande et vitesse | | |
| T15 | Délai entre la commande `s` et l'état DÉFAUT | | |
| T15 | Délai entre la commande `n` et l'état DÉFAUT | | |
| T16 | Limitation à 42,0 °C ; arrêt à 52,0 °C ; sortie à 25,0 °C | | |
| T18 | Initialisation : durée de INIT, ressources vérifiées | | |
| T18 | Démarrage avec un module absent (commande `d`) | | |

## Partie II — Portage (L5-C)

| | Windows x86-64 | Linux x86-64 | STM32 |
|---|---|---|---|
| Compilateur et version | | | |
| Options | | | |
| Tailles : `int`, `long`, pointeur, `size_t` | | | |
| Fichiers remplacés (couche d'adaptation) | | | |
| `test_evq` / `test_tableau` | | | |
| `test_trajet` / `test_regul` / `test_surv` | | | |
| `test_automate` (traces) | | | |
| Rejeu de la trace du scénario `…` | | | |
| Rejeu de la trace du scénario `…` | | | |

Différences observées et explication :

## Partie II — Décision collective et désaccord (L5-D)

Décision technique prise en équipe :

| Élément | Contenu |
|---|---|
| Question à trancher | |
| Options envisagées | |
| Critères | |
| Choix et raison | |
| Résultat observé après coup | |

Désaccord (scénario de `dossier/scenario_desaccord.md`) :

| Élément | Contenu |
|---|---|
| Objet du désaccord |À trois jour de la démonstration, la file d'évenement déborde pendant la rafale de la commande E, causant un évenement par default à être perdu. Deux solution s'offre à nous pour régler le problème. La pemière solution implique de rejeter le nouvel évenement avec une mémorisation à part. La seconde solution implique d'écraser l'évenement le plus ancien afin de toujours garder la file avec les évenement les plus recent. |
| Position de chacun | Première position : Ana et Simon:  La seconde solution est plus appréciable, la file garde les élements les plus récent, sans jamais écraserun évenement prioritaire. En effet, l'état sera à jours et le fsm.c ne mémorisera plus de données inutile. Cependant, les élement evq_push et les tests doivent être modifiés en conséquence.---Seconde position:  Yassine: La premiere solution est plus appréciable la file du laboratoire 3 reste inchangée). En effet, nous pouvons seulement changer evq.c. Cela serait un petit changement qui nous évite d'avoir des problèemes plus conséquent juste avant la démonstration. Les évenement E-STOP sont mémoriser dans les drapeu. Cependant, les évenement ordinaire peuvent possiblement être perdu lors de la saturation.  |
| Démarche suivie |Nous avons commencé par avoir chacun des deux cote explique la situation et sont opinion. Par la suite, nous avons choisi la sécurité (évenemnt prioritaire)(E9), le temps réel qui aucun acces au bus lors d'une interuption(thick_perdu = 0) et la conservabilité de fsm.c. Afin de nous départagé, nous avons relue E9 et E14 et fai tune estimation sur la difficulté de réecrire test_evq.|
| Issue |Nous avons choisi la seconde option poru notre laboratoire. En effet, la première option nous protège seulement des évenements prioritaires (E-STOP). Cependant, le critère E9 nous demande de prendre en considération la détection d'objet et le feu rouge aussi. En utilisant la première solution, nous somme a risque de perdre certaine valeur essentiel pour notre critère 9. La seconde option nous assure que notre système prend en compte les obstacles et les feux rouges. En effet, en gardant toujours les évenements les plus récents. En somme, les critères de laboratoire nous ont permis de prendre une décision éclairer pour le futur de notre laboratoire   |

## Partie II — Séances

### Séance 2 — date : `…`

- Objectifs :
- Réalisé :
- Problèmes rencontrés, hypothèses, essais :
- Validations Git (auteur, identifiant, objet) :
- À faire avant la séance 3 :

### Entre les séances 2 et 3

- Réalisé :
- Validations Git :

### Séance 3 — date : `…`

- Objectifs :
- Réalisé :
- Problèmes rencontrés, hypothèses, essais :
- Validations Git :
- À faire avant la démonstration finale :

### Démonstration finale — date : `…`

- Scénarios S1 à S5 : résultat de chacun
- Questions posées et réponses à approfondir dans le rapport :

## Vidéo de présentation (énoncé, section 7.2)

Durée visée : 7 à 10 minutes. Filmer les scénarios dès qu'ils fonctionnent, sans attendre la veille de la remise.

| Séquence | Durée prévue | Qui parle | Ce qui est montré (véhicule, afficheur, télémétrie, code) | Tournée le |
|---|---|---|---|---|
| Présentation de l'équipe, du véhicule et de l'architecture | 1 min | | | |
| Démonstration : conduite manuelle, trajet autonome, arrêt et reprise, E-Stop, défaut injecté | 4 min | | | |
| Explication technique du membre A : `…` | 1 min 30 | | | |
| Explication technique du membre B : `…` | 1 min 30 | | | |
| Bilan : limites, ce qui serait fait autrement | 1 min | | | |

Durée finale : `…` — fichier ou lien : `…` — outils de montage : `…`

## Sources et outils

| Source ou outil | Usage |
|---|---|
| Compléments du laboratoire 4 et du projet | |
| Outil `automate.py` | |
| Corrigé du laboratoire 3 (le cas échéant) | |
| Fiches techniques, manuel | |
| Outils d'IA générative (outil, version, usage) ou « aucun usage » | |
