# CEG 3536 — Projet final — Code de départ

Ce dossier est le **code de départ du projet final** : un projet STM32CubeIDE
complet, qui **compile tel quel, sans avertissement**, et qui couvre la partie I
(exigences E1 à E12) et la partie II (exigences E13 à E22). C'est le **seul**
complément du projet. Voir l'énoncé, sections 4.2 à 4.13.

Il réunit trois couches :

- la base du laboratoire 3 (corrigé publié : routines assembleur, `main.c`,
  `fsm.c`, `telem.c`, file d'événements, acquisition), à citer comme source ;
- les fichiers de la partie I : interface `lab4.h`, squelettes des pilotes et
  des routines assembleur, tableau de bord, pilote défectueux de L4-A ;
- les fichiers de la partie II : `projet.h`, sections critiques, outil
  d'automate, squelettes des modules sans matériel, gabarits du dossier.

Programmé tel quel, le véhicule se comporte **comme à la fin du laboratoire 3** :
les fichiers nouveaux sont compilés, mais aucun n'est encore appelé. Les
squelettes marqués « à compléter » retournent des valeurs neutres. Les ajouts
à `lab3.h` (neuf champs à la fin de `etat_vehicule_t`) et à `evq.h` (huit
types d'événements, prototype de `evq_max`) sont **déjà faits**.

## Fichiers de la partie I

| Fichier | Rôle | État |
|---|---|---|
| `Core/Inc/lab4.h` | Prototypes de `trame_xor8` et `exti_l4_init`, adresses I²C, constantes, état `ETAT_ARRET_EVENEMENT`, modes, causes, drapeaux, image des DEL, `env_mesure_t`, `L4A_VARIANTE` | Fourni ; à tenir à jour |
| `Core/Src/registres_lab4.inc` | Constantes assembleur des lignes EXTI 6 et 9 (EXTICR, IRQn, priorités, filtre du PIR) | Fourni ; `THI_FRONT_MONTANT` et priorités à confirmer |
| `Core/Src/somme.s` | Routine assembleur `trame_xor8` | **À compléter (E10)** |
| `Core/Src/exti_l4.s` | `exti_l4_init` et gestionnaire `EXTI9_IRQHandler` (PIR) ; variables `pir_flag`, `pir_t_ms`, `thi_flag` | **À compléter (E6, E4)** ; l'effacement de la demande est fourni |
| `Core/Inc/somme_test.h`, `Core/Src/somme_test.c` | Banc des cinq cas obligatoires de `trame_xor8` (commande série `x`) | Fournis |
| `Core/Inc/i2c.h`, `Core/Src/i2c.c` | Pilote I2C2 : horloges, broches, balayage, compteurs et simulation d'absence fournis | **À compléter (E1)** : `TIMINGR`, les trois opérations |
| `Core/Inc/ioexp.h`, `Core/Src/ioexp.c` | Objet extenseur (DEL, commutateurs) | **À compléter (E2)** |
| `Core/Inc/lcd.h`, `Core/Src/lcd.c` | LCD et rétroéclairage : images demandée et affichée fournies | **À compléter (E3)** : `lcd_step` |
| `Core/Inc/touchpad.h`, `Core/Src/touchpad.c` | Contrôleur tactile ; `tp_touche` fournie | **À compléter (E4)** : `tp_init`, `tp_lire`, table des touches à relever |
| `Core/Inc/spi.h`, `Core/Src/spi.c` | Pilote SPI1 | **À compléter (E7)** |
| `Core/Inc/env.h`, `Core/Src/env.c` | Capteur environnemental, machine à trois phases | **À compléter (E7)** |
| `Core/Inc/bme680_comp.h`, `Core/Src/bme680_comp.c` | Formules de compensation entières (température, pression, humidité), sans matériel | Fournis ; ne pas modifier ; source à citer |
| `Core/Inc/tableau.h`, `Core/Src/tableau.c` | Formatage des deux lignes et image des huit DEL, sans matériel ; les formats sont définis dans `tableau.h` | **À compléter (E3, E8)** |
| `Core/Src/tableau_pres.c` | Présentation : page, message « VERROU », texte remis à `lcd`, image des DEL | Fourni, sauf le rétroéclairage : **à compléter (E3)** |
| `Core/Inc/tableau_test.h`, `Core/Src/tableau_test.c`, `hote/test_tableau.c` | Autotest du formatage (6 cas) sur hôte et sur cible (commande série `f`) | Fournis ; cas à ajouter (E11) |
| `Core/Inc/l4a_defaut.h`, `Core/Src/l4a_defaut.c` | Pilote de TouchPad défectueux de la composante L4-A | Fourni ; à investiguer, puis à corriger (section 4.5 de l'énoncé) |

## Fichiers de la partie II et fichiers communs

| Fichier | Rôle | État |
|---|---|---|
| `Core/Inc/projet.h` | Prototypes de `crit_entrer` et `crit_sortir`, macros de mesure du masquage, constantes de la partie II, états `ETAT_INIT` et `ETAT_DEFAUT`, cause `CAUSE_THERMIQUE`, codes de défaut, drapeaux, `segment_t`, valeurs injectées du mode essai | Fourni ; à tenir à jour |
| `Core/Src/crit.s` | Routines assembleur `crit_entrer` et `crit_sortir` | **À compléter (E13)** ; tant qu'il ne l'est pas, rien n'est masqué |
| `Core/Inc/crit_test.h`, `Core/Src/crit_test.c` | Banc des cinq cas de section critique (commande série `c`) ; rafale d'événements poussés depuis un gestionnaire d'interruption (commande `e`) ; variable `crit_max_cycles` | Fournis |
| `outils/automate.py`, `outils/LISEZMOI.md` | Vérificateur et générateur d'automates : format de la table JSON, invariants, commandes, code généré | Fournis ; ne pas modifier |
| `hote/automate/exemple.json`, `exemple_sequence.txt` | Automate d'exemple du tutoriel, avec **une erreur volontaire** à corriger, et sa suite d'entrées | Fournis ; modèles de `automate.json` et des séquences de l'équipe |
| `Core/Inc/trajet.h`, `Core/Src/trajet.c` | Suivi du trajet en distance ; `TRAJET_REF` fourni | **À compléter (E16)** |
| `Core/Inc/regul.h`, `Core/Src/regul.c` | Objet régulateur PI en virgule fixe ; `regul_init` et `regul_reset` fournis | **À compléter (E17)** : `regul_step` |
| `Core/Inc/surv.h`, `Core/Src/surv.c` | Surveillances : sens, encodeur muet, deux seuils thermiques | **À compléter (E18, E19)** |
| `Core/Inc/essai.h`, `Core/Src/essai.c` | Mode essai ; bascule du mode fournie | **À compléter (E20)** : les injections |
| `hote/banc.h`, `hote/test_trajet.c`, `test_regul.c`, `test_surv.c` | Essais hôte des modules sans matériel (13, 13 et 33 cas) | Fournis ; cas à ajouter |
| `hote/test_automate.c` | Trace du code généré, au format de `automate.py simuler` | Fourni |
| `hote/crit_hote.c` | Équivalents C de `crit_entrer` et `crit_sortir` pour Windows et Linux (couche d'adaptation, L5-C) | Fourni |
| `hote/Makefile` | Construit et exécute tous les essais hôte (`make evq tableau` pour ceux de la partie I seulement) ; `make automate` enchaîne vérification, génération et comparaison des traces | Fourni |
| `hote/telemetrie.py` | Trames de 24 colonnes (celles de 17 colonnes de la partie I sont acceptées, avec un avis), résumé du trajet et de la régulation, `--sauver` (trace d'un scénario), `--extraire` (CSV pour le rejeu) | Fourni (pyserial requis) |
| `dossier/` | Gabarits : matrice exigences-essais-preuves, échéancier, fiche périphérique, bilan individuel, évaluation entre pairs, scénario de désaccord, références réglementaires | Fournis ; à remplir |
| `Makefile`, `build.sh` | Construction en ligne de commande (facultatif) : `make` ou `bash build.sh [flash]` | Fournis |
| `JOURNAL.md` | Modèle de journal du projet (parties I et II) | Fourni ; à tenir à chaque séance |

## Mise en route

1. Dézipper l'archive, puis renommer le dossier en `CEG3536_Projet_<équipe>`.
2. Dans STM32CubeIDE : *File > Import > Existing Projects into Workspace*,
   choisir le dossier, puis renommer le projet (clic droit > *Rename*).
3. *Build Project* : zéro erreur, zéro avertissement. Sans l'IDE : `make`, ou
   `bash build.sh` (Git Bash et STM32CubeCLT).
4. Programmer la carte : le comportement est celui du laboratoire 3
   (télémétrie, potentiomètre, deux sens, E-Stop, acquittement).
5. Dans `hote/` : `make evq tableau` (ou les commandes gcc indiquées dans
   `test_evq.c` et `test_tableau.c`) : `test_evq` passe ; les six cas de
   `test_tableau` échouent tant que `tableau.c` n'est pas écrit. `make` seul
   exécute aussi les essais de la partie II : ils échouent tant que les
   squelettes ne sont pas complétés, et `automate` s'arrête sur l'erreur
   volontaire de l'exemple (prise en main : `outils/LISEZMOI.md`).
6. Créer le dépôt Git de l'équipe et valider cet état de départ avant toute
   modification.

**Équipe qui préfère son propre code du laboratoire 3** : remplacer les
fichiers du laboratoire 3 de ce dossier par les siens (`main.c`, `fsm.c`,
`telem.c`, `evq.c`, `acq.c`, `adc_dma.c`, les fichiers `.s` et leurs
en-têtes), puis refaire dans `lab3.h` et `evq.h` les ajouts marqués
« projet final ». Le projet doit de nouveau compiler sans avertissement
avant d'aller plus loin.

# Partie I (exigences E1 à E12)

## Partie I — modifications des fichiers du laboratoire 3

`lab3.h` et `evq.h` sont déjà modifiés (champs `mode`, `cause`, `plafond_pct`
et `drapeaux` ; types `EV_TOUCHE` à `EV_DEFAUT`). Restent à modifier par
l'équipe :

**`fsm.c`** : état `ETAT_ARRET_EVENEMENT`, `mode`, `cause`, `plafond_pct`
(100 après réinitialisation) et `drapeaux` ; traitement de `EV_TOUCHE`,
`EV_PIR`, `EV_COMMUT`, `EV_ENV` et `EV_DEFAUT` selon les priorités de la
section 4.6 ; consigne appliquée = minimum de la consigne demandée, du plafond
et, s'il y a lieu, de `LIMITATION_PCT`. `fsm_sorties` reste le seul endroit
qui commande le moteur, les DEL de la Nucleo et l'avertisseur.

**`telem.c`** : huit colonnes de plus dans `telem_trame`, somme de contrôle
`*HH` avant `\r\n` (calculée par `trame_xor8` sur tout ce qui précède
l'astérisque), nouvelle fonction `void telem_evenement(const evq_event_t *e)`
et ligne de balayage. Le prototype de `telem_trame` est étendu par l'équipe
(par exemple avec un pointeur vers `env_mesure_t` et les deux compteurs).

**`main.c`** : initialisations, puis répartiteur du bus (une transaction par
tick, énoncé 4.7) :

```c
    i2c_init();  spi_init();  exti_l4_init();
    n = i2c_scan(adresses, 8u);          /* avant la boucle : ligne « I,… »   */
    ioexp_init(&x_del, I2C_ADR_DEL);  ioexp_init(&x_commut, I2C_ADR_COMMUT);
    tp_init();  lcd_init();  tableau_init();  env_init();
    ...
    /* à chaque tick, UN SEUL demandeur obtient le bus, par ordre de priorité */
    if (thi_flag)                         { thi_flag = 0u; /* tp_lire, tp_touche */ }
    else if (/* 10 ms écoulées */)        { /* ioexp_lire, anti-rebond, EV_COMMUT */ }
    else if (/* image des DEL changée */) { /* ioexp_ecrire(&x_del, ~image) */ }
    else                                  { lcd_step(); }
    env_step();                           /* SPI : hors de ce budget */
```

## Partie I — lignes de télémétrie (E10)

| Ligne | Format | Exemple |
|---|---|---|
| Trame, 5 Hz | `T,<ticks_ms>,<etat>,<sens>,<consigne_pct>,<pwm_pct>,<temp_c_x10>,<vitesse_rpm_x100>,<nb_evenements>,<evq_debordements>,<mode>,<cause>,<plafond_pct>,<env_t_x10>,<env_h_x10>,<env_p_x10>,<i2c_erreurs>,<ticks_perdus>*HH` | `T,1200,1,1,60,60,241,24530,12,0,1,0,90,234,452,10032,0,0*71` |
| Événement | `E,<ticks_ms>,<type>,<valeur>*HH` (type = numéro de `evq_type_t`) | `E,1234,8,5*60` |
| Balayage | `I,<n>,<adr>,…*HH` (adresses sur deux chiffres hexadécimaux) | `I,5,2D,38,39,3E,5A*09` |

`HH` est le OU exclusif, sur deux chiffres hexadécimaux en majuscules, de
tous les caractères qui précèdent l'astérisque. Tant qu'aucune mesure du
capteur environnemental n'a abouti, les trois colonnes `env_*` valent 0.

## Partie I — composante L4-A

`l4a_defaut.c` est compilé dans le projet mais n'est appelé nulle part. Pour
l'investigation, remplacer temporairement, dans `main.c`, les appels à
`tp_init` et `tp_lire` par `l4a_tp_init` et `l4a_tp_lire` (par exemple sous
`#if L4A_ESSAI`), après avoir fixé `L4A_VARIANTE` dans `lab4.h` à la valeur
attribuée sur Brightspace. La table des touches de l'équipe (`tp_table`)
reste utilisée. Consigner le symptôme et les hypothèses **avant** de mesurer,
et la prédiction **avant** l'essai discriminant (`JOURNAL.md`). La version
défectueuse et la version corrigée sont deux validations Git distinctes.

## Partie I — ordre de travail suggéré

| Étape | Fichiers | Vérification |
|---|---|---|
| 1. Bus I²C et balayage | `i2c.c`, `main.c` | Cinq adresses dans la ligne `I,…` ; `I2C2->ISR` au repos (T1) |
| 2. Extenseurs | `ioexp.c`, `main.c` (anti-rebond, `EV_COMMUT`) | Chenillard sur les huit DEL ; un événement par appui (T2) |
| 3. LCD et rétroéclairage | `lcd.c`, puis `tableau.c` sur hôte | Texte fixe, puis page de conduite ; `make` dans `hote/` (T3) |
| 4. TouchPad | `touchpad.c`, `exti_l4.s` ou gestionnaire C, `fsm.c` | Table des touches relevée ; lignes `E,…,8,…` (T4, T5) |
| 5. SPI et capteur | `spi.c`, `env.c` | Identifiant 0x61 ; page des capteurs ; souffler sur le capteur (T7) |
| 6. PIR | `exti_l4.s`, `fsm.c` | `pir_flag`, `pir_t_ms` au débogueur ; arrêt en moins de 100 ms (T6) |
| 7. Mode autonome et priorités | `fsm.c`, `tableau_pres.c` | Tableau des événements de la section 4.6 (T8) |
| 8. Télémétrie contrôlée | `somme.s`, `telem.c` | Commande `x` ; `telemetrie.py` sur 30 s (T9) |
| 9. L4-A et robustesse | `l4a_defaut.c`, commande `d` | Investigation consignée ; rétablissement du LCD (T10) |

## Partie I — commandes série (un caractère, terminal ou `telemetrie.py`)

Celles du laboratoire 3 restent (`a`, `q`, `b`, `t`, `z`). À ajouter :
`x` banc de `trame_xor8` (`X,echecs=0` attendu) · `f` autotest du tableau de
bord (`F,echecs=0` attendu) · `i` nouveau balayage du bus (ligne `I,…`) ·
`d` LCD simulé absent pendant 2 s :
`i2c_simuler_absence(I2C_ADR_LCD, ABSENCE_SIMULEE_MS)` (E12).

## Partie I — points à confirmer sur le matériel

Les registres du contrôleur tactile (MPR121) et du capteur environnemental
(BME680), les octets de commande du LCD, la valeur de `TIMINGR`, le front
actif de THI et la table des touches sont donnés comme points de départ :
chacun se vérifie dans la fiche technique liée en annexe D du guide, puis au
débogueur, et se consigne dans `JOURNAL.md`.

# Partie II (exigences E13 à E22)

## Partie II — passage à la partie II et modifications

Avant de commencer, valider dans Git l'état du projet à la démonstration
intermédiaire (étiquette `partie1`) : c'est le point de retour en cas de
régression.

**`evq.c`** (E14, L5-A) : la file garde sa capacité de `EVQ_CAPACITE` = 32
entrées, mais elle est désormais écrite par des gestionnaires d'interruption.
Toute lecture ou modification des indices et du compteur se fait entre
`crit_entrer` et `crit_sortir` ; la section ne contient **aucun appel de
fonction** et sa durée se mesure :

```c
    uint32_t p  = crit_entrer();
    uint32_t t0 = CRIT_CYCLES();
    ... indices, compteur, copie de l'événement ...
    CRIT_MESURER(t0);
    crit_sortir(p);
```

S'y ajoutent `evq_max`, la politique de débordement retenue et la garantie
qu'un événement `EV_ESTOP` ou `EV_DEFAUT` n'est jamais perdu. `evq.c` inclut
`projet.h` ; sur hôte, `crit_hote.c` remplace `crit.s` (déjà prévu dans
`hote/Makefile`).

**Gestionnaires d'interruption** : les événements E-Stop, PIR et THI peuvent
maintenant être poussés dans la file par le gestionnaire lui-même (appel de
`evq_push` depuis un gestionnaire C, ou `bl evq_push` depuis l'assembleur),
avec l'horodatage de l'instant de l'interruption. La coupure matérielle de
E-Stop (CCR1 = 0, STB = 0 dans le gestionnaire) ne change pas.

**`fsm.c`** (E15) : la fonction de transition écrite à la main est remplacée
par `automate_transition` du code généré ; `fsm.c` traduit chaque événement
en entrée du modèle, appelle la transition et applique les sorties en un seul
endroit (`outils/LISEZMOI.md`, « Du modèle au véhicule »). États ajoutés à la
colonne `<etat>` : `ETAT_INIT` (5) et `ETAT_DEFAUT` (6), qui remplace
`ETAT_ARRET_EVENEMENT` avec `CAUSE_DEFAUT` ; cause ajoutée : `CAUSE_THERMIQUE`.

**`telem.c`** (E21) : sept colonnes de plus avant la somme de contrôle ; la
ligne d'en-tête `H` est mise à jour.

**`tableau.c`, `tableau_pres.c`** : troisième page, astérisque du mode essai,
états INIT et DEFAUT, cause THERMIQUE, message « FILE » (formats ci-dessous).
Les six cas de `tableau_test.c` restent valides ; l'équipe ajoute les siens.

**`main.c`** : initialisations, fenêtre de mesure de 100 ms, puis, à chaque
fenêtre, l'enchaînement suivant (les noms des variables sont indicatifs) :

```c
    crit_rafale_init();  essai_init();  surv_init();
    trajet_init(TRAJET_REF, TRAJET_REF_N);
    regul_init(&regul, REGUL_KP_Q8, REGUL_KI_Q8);
    ...
    /* toutes les REGUL_PERIODE_MS : fronts et sens de la fenêtre écoulée     */
    rpm = /* fronts convertis avec RPM_Q16_K_100MS */;
    if (/* en marche, mode AUTONOME */) {
        trajet_avancer(fronts);                       /* mesure RÉELLE         */
        pwm = regul_step(&regul, consigne, rpm, plafond);   /* mesure RÉELLE   */
    }
    surv_step(&vehicule, essai_sens(sens), essai_fronts(fronts),
              essai_temp_c10(temp_c10), REGUL_PERIODE_MS);
    /* surv_code() ou surv_thermique() a changé : evq_push(EV_DEFAUT, ...) ou
       evq_push(EV_THERMIQUE, ...) ; trajet_fini() devient vrai : EV_FIN      */
```

Les valeurs injectées par `essai_sens` et `essai_fronts` ne vont **qu'à
`surv_step`** : le régulateur et le trajet reçoivent toujours la mesure réelle
(un encodeur « muet » injecté dans le régulateur le ferait accélérer jusqu'à
la commande maximale).

## Partie II — fenêtre de mesure de la vitesse

La fenêtre du laboratoire 3 (`FENETRE_VITESSE_MS` = 250 ms, constante
`RPM_Q16_K` = 21 028) passe à `REGUL_PERIODE_MS` = 100 ms :
tr/min = fronts × 600 / 748, soit la constante `RPM_Q16_K_100MS` = 52 569 en
Q16.16. Deux solutions : donner la nouvelle constante à la routine assembleur
`enc_fronts_vers_rpm_q16`, ou garder la routine et corriger son résultat
(× 5 / 2). Le choix se justifie dans le rapport ; le banc de la routine
(laboratoire 3) est repassé dans les deux cas.

## Partie II — pages du LCD (16 caractères par ligne)

| Page | Ligne 1 | Ligne 2 |
|---|---|---|
| Conduite (partie I) | `AUT AV MARCHE` ; en mode essai, l'espace qui suit le mode devient un astérisque : `AUT*AV MARCHE` | inchangée |
| Capteurs (partie I) | inchangée | inchangée |
| Trajet (`PAGE_TRAJET`) | `S2/4 D 012.5/105` : segment / nombre de segments, distance parcourue en tours (trois chiffres, un point, un dixième) / distance totale en tours | `R250 V245.3 +02%` : consigne en tr/min, vitesse mesurée, erreur relative signée en pour cent (bornée à ±99) |

États ajoutés sur la ligne 1 : `INIT`, `DEFAUT`, `THERMIQUE` (cause de
l'arrêt sur événement). Message `FILE` : pendant `FILE_MESSAGE_MS` = 1 s
après un débordement de la file, à la place de l'état, comme le message
« VERROU » de la partie I. Hors du mode AUTONOME, la page de trajet affiche
des tirets à la place de la consigne et de l'erreur. Aucun type flottant :
les formats ci-dessus s'obtiennent en entiers, comme à la partie I.

## Partie II — lignes de télémétrie (E21)

| Ligne | Format | Exemple |
|---|---|---|
| Trame, 5 Hz | les 17 colonnes de la partie I, puis `,<mode_essai>,<segment>,<dist_x10>,<consigne_rpm>,<code_defaut>,<evq_max>,<crit_max_cycles>*HH` | `T,61200,1,1,74,73,241,24530,512,0,1,0,100,234,452,10032,0,0,0,2,251,250,0,9,61*6C` |
| Événement | `E,<ticks_ms>,<type>,<valeur>*HH`, inchangée | `E,61234,13,1*68` |
| Balayage | `I,<n>,<adr>,…*HH`, inchangée | |

`<etat>` va de 0 à 6, `<cause>` de 0 à 5. `<consigne_pct>` reste la consigne
en pour cent : en mode AUTONOME, consigne en tr/min × 100 / `VITESSE_MAX_RPM`.
`<segment>` et `<consigne_rpm>` valent 0 hors du mode AUTONOME.
`<code_defaut>` : 0 aucun, 1 initialisation, 2 I²C, 3 sens, 4 encodeur,
5 simulé (SW5). `<crit_max_cycles>` ne dépasse pas 200.

Une trace par scénario, puis son extraction pour le rejeu (L5-C) :

```
python telemetrie.py COM8 --duree 60 --sauver preuves/s3_autonome.log
python telemetrie.py preuves/s3_autonome.log --extraire preuves/s3_autonome.csv
```

## Partie II — commandes série (un caractère)

Celles du laboratoire 3 et de la partie I restent. À ajouter :

| Commande | Effet | Réponse attendue |
|---|---|---|
| `m` | `essai_commande('m')` : mode essai actif ou inactif (comme SW7) | événement `EV_ESSAI` ; colonne `mode_essai` |
| `1` `2` `3` `h` `s` `n` | `essai_commande(c)` : injections, acceptées en mode essai seulement | événement `EV_ESSAI`, valeur = caractère |
| `e` | `crit_rafale(ESSAI_RAFALE_N)` : 40 événements `EV_TEST` poussés depuis un gestionnaire d'interruption, pendant que la boucle principale consomme la file | débordement compté, « FILE », système cohérent |
| `r` | `trajet_reset()`, à l'arrêt seulement (comme SW6) | refusée en marche |
| `c` | `crit_autotest()` | `C,echecs=0` |
| `g` | `automate_autotest()` | `G,echecs=0,somme=XXXXXXXX` |

La commande `t` du laboratoire 3 pousse ses événements depuis la boucle
principale ; la commande `e` les pousse depuis une interruption, ce qui met la
section critique à l'épreuve : sans elle, les compteurs de la file finissent
par se contredire (`evq_total` différent du nombre d'événements consommés
plus `evq_count`).

## Partie II — ordre de travail suggéré

| Étape | Fichiers | Vérification |
|---|---|---|
| 1. Prise en main de l'outil (avant J3) | `hote/automate/exemple.json` | Contre-exemple compris, table corrigée, `make automate` (T12) |
| 2. Automate du véhicule (J3, 27 nov.) | `hote/automate/automate.json`, séquences | Rapport sans erreur ; version fautive conservée ; traces identiques |
| 3. Modules sans matériel, sur hôte | `trajet.c`, `regul.c`, `surv.c`, `essai.c` | `make` dans `hote/`, sur Windows et sur Linux |
| 4. Sections critiques | `crit.s`, `evq.c` | Commande `c` ; `test_evq` étendu ; `crit_max_cycles` (T11) |
| 5. Migration vers le code généré | `fsm.c`, `automate_gen.*` | Comportement de la partie I reproduit ; commande `g` (T12) |
| 6. Fenêtre de 100 ms, distance, trajet | `main.c`, `tableau.c` | Page de trajet ; changements de segment à 20, 60 et 75 tours (T13) |
| 7. Régulation | `main.c`, gains dans `projet.h` | `telemetrie.py` : erreur 2 s après chaque changement (T14) |
| 8. Surveillances, thermique, mode essai | `main.c`, `fsm.c` | Commandes `s`, `n`, `1`, `2`, `3`, `h` (T15, T16) |
| 9. Initialisation, rafale, non-régression | `fsm.c`, `main.c` | INIT puis PRÊT ; commande `e` ; T1 à T10 repassés (T11, T17, T18) |

## Partie II — points à confirmer sur le matériel

Les gains `REGUL_KP_Q8` et `REGUL_KI_Q8` sont des points de départ, obtenus
sur un modèle simplifié du moteur : ils s'ajustent sur le banc et se
justifient par les relevés de la télémétrie. `VITESSE_MAX_RPM` = 340 tr/min
se compare à la vitesse mesurée à 100 % au laboratoire 3. La ligne EXTI15,
utilisée par la rafale, doit rester libre dans le projet de l'équipe.

# Sécurité

Les règles de l'énoncé du laboratoire 2 (encadré « Sécurité moteur ») restent
en vigueur. Le PIR arrête maintenant le véhicule : garder les mains hors du
champ du capteur pendant les mesures de vitesse, et le potentiomètre en bas
avant chaque démarrage en mode MANUEL.

Partie II : en mode AUTONOME, le moteur démarre seul et change de vitesse
seul : dégager l'arbre avant d'appuyer sur User. Le mode essai n'affaiblit
aucune protection : E-Stop, le PIR et les arrêts restent prioritaires, et une
valeur injectée n'entre jamais dans le régulateur.
