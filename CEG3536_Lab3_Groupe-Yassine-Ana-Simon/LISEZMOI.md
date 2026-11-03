# CEG 3536 — Laboratoire 3 — Complément de démarrage

Ce complément s'ajoute au projet du laboratoire 2 de l'équipe (ou au corrigé
du laboratoire 2 publié sur Brightspace, à citer comme source). Il fournit les
en-têtes CMSIS, les interfaces de tous les modules, les squelettes C et
assembleur, le programme d'essai hôte de la file d'événements (jalon J2), le
banc de comparaison L3-A et le script de vérification de la télémétrie.
Voir l'énoncé, sections 4.2 à 4.8.

| Fichier | Rôle | État |
|---|---|---|
| `Drivers/CMSIS/…` | En-têtes CMSIS (core_cm33.h, stm32l552xx.h, …), licence Apache-2.0 | Fournis ; ne pas modifier |
| `Core/Inc/lab3.h` | Prototype de `enc_fronts_vers_rpm_q16`, type `etat_vehicule_t`, constantes (fenêtre, rampe, télémétrie, ADC, zone morte, hystérésis, étalonnage de la température) | Fourni ; constantes d'étalonnage à relever (E6) |
| `Core/Inc/evq.h`, `acq.h`, `adc_dma.h`, `telem.h`, `fsm.h`, `bench_l3a.h`, `evq_test.h` | Interfaces des modules (section 4.2 de l'énoncé) | Fournis ; à étendre si nécessaire, en le documentant |
| `Core/Src/vitesse.s` | Routine assembleur de conversion en virgule fixe | **À compléter (E3)** : constante et multiplication |
| `Core/Src/evq.c` | File circulaire d'événements, sans matériel | **À compléter (E7)** : `evq_push`, `evq_pop` |
| `Core/Src/acq.c` | Objet d'acquisition (version 1 puis version 2) | **À compléter (E4, E5, E6)** : `acq_brut`, `acq_valeur` |
| `Core/Src/adc_dma.c` | ADC1 deux voies + DMA circulaire | **À compléter (E4)** : étapes 2 à 6 |
| `Core/Src/telem.c` | LPUART1, file d'émission, trames | **À compléter (E10)** : BRR/CR1, `telem_step`, `telem_rx`, `telem_trame` |
| `Core/Src/fsm.c` | Machine à états en C par événements (`EV_USER` et `EV_ESTOP` fournis) | **À compléter (E8, E5, E9)** : `EV_TOUCH`, `EV_CONSIGNE`, `EV_VITESSE`, rampe |
| `Core/Src/main.c` | Programme principal en C, boucle par tick, commandes série | **À compléter (E3, E5)** : fenêtre de l'encodeur, événement de consigne |
| `Core/Src/evq_test.c`, `hote/test_evq.c`, `hote/Makefile` | Autotest de la file (9 cas) sur hôte et sur cible (commande série `q`) | Fournis ; cas à ajouter (E11) |
| `Core/Src/l3a_c.c`, `Core/Src/bench_l3a.c` | Squelette de la version C de la routine conservée (`motor_set_pwm` par défaut) et banc de comparaison avec cycles DWT (commande `b`) | **`l3a_c.c` à compléter (section 4.5)** ; banc fourni, cas à adapter si une autre routine est comparée |
| `hote/telemetrie.py` | Vérifie les trames reçues (format, 5 Hz, plages) ; `python telemetrie.py COM8 --duree 30 --sauver journal.csv` | Fourni (pyserial requis) |
| `Makefile`, `build.sh` | Construction en ligne de commande (facultatif) : `make` ou `bash build.sh [flash]` | Fournis |
| `JOURNAL.md` | Modèle de journal du laboratoire 3 | Fourni ; à tenir à chaque séance |

## Mise en route

1. Copier le projet du laboratoire 2 sous le nom `CEG3536_Lab3_<équipe>`
   (clic droit > *Copy* / *Paste* dans STM32CubeIDE).
2. Dézipper le complément et copier son contenu **par-dessus** le projet
   copié (`Core/`, `Drivers/`, `hote/`, `Makefile`, `build.sh`, `JOURNAL.md`).
   Aucun fichier du laboratoire 2 n'est écrasé.
3. **Retirer `Core/Src/main.s` et `Core/Src/fsm.s`** du projet (les supprimer
   ou les exclure de la construction) : `main.c` et `fsm.c` les remplacent.
   Les autres fichiers `.s` (gpio, buttons, estop, motor, encoder, systick,
   buzzer) restent tels quels et sont appelés depuis le C.
4. Dans *Project > Properties > C/C++ Build > Settings > MCU/MPU GCC Compiler*
   : *Include paths* : ajouter `../Drivers/CMSIS/Include` et
   `../Drivers/CMSIS/Device/ST/STM32L5xx/Include` ; *Preprocessor* : vérifier
   que `STM32L552xx` est défini (il l'est dans le projet de départ) ; *Warnings*
   : ajouter `-Wextra`. (Le `Makefile` et `build.sh` fournis contiennent déjà
   ces chemins.)
5. *Build Project* : le projet compile sans avertissement. Après *Resume*, la
   DEL rouge s'allume (fsm_init) ; la télémétrie n'émet rien tant que
   `telem_init` (BRR, CR1) et `telem_step` ne sont pas écrites.
6. Dans `hote/` : `make` (ou la commande gcc indiquée dans `test_evq.c`) :
   tant que `evq_push` et `evq_pop` ne sont pas écrites, les cas échouent :
   première tâche du jalon J2.

## Ordre de travail suggéré

| Étape | Fichiers | Vérification |
|---|---|---|
| 1. Programme principal en C qui reproduit le laboratoire 2 | `main.c`, `fsm.c` (EV_USER, EV_TOUCH, rampe) | User fait tourner le moteur ; point d'arrêt dans `motor_set_pwm` (T2) |
| 2. SysTick → LPUART1 | `telem.c` (BRR, CR1, `telem_step`) | Terminal 115 200 8N1 : en-tête `H,…` puis trames `T,0,…` à 5 Hz |
| 3. Routine en virgule fixe | `vitesse.s`, `main.c` (fenêtre), `fsm.c` (EV_VITESSE), `telem.c` (`telem_trame`) | Vitesse dans la télémétrie ; commande `b` ; T3 |
| 4. ADC/DMA et potentiomètre | `adc_dma.c`, `acq.c` (v1), `main.c` (EV_CONSIGNE), `fsm.c` (EV_CONSIGNE) | Commande `a` ; consigne suit le potentiomètre en marche (T4, T5) |
| 5. File d'événements et jalon J2 | `evq.c`, `hote/` | `make` dans `hote/` sur Windows et Linux ; commande `q` sur cible |
| 6. Température et version 2 de l'objet | `acq.c` (v2), `lab3.h` (étalonnage) | T6 ; hystérésis visible sur la consigne |
| 7. L3-A et L3-D | `bench_l3a.c`, désassemblage, DWT | Commande `b` à -O0 et -O2 ; captures |

## Capteur de température

Le capteur est un LMT86 (lien dans l'annexe D du guide) : sortie d'environ
2,10 V à 0 °C et 1,78 V à 30 °C, pente −10,9 mV/°C. Avec S2 à gauche (Int),
l'ADC doit lire environ 1,8 à 1,9 V à température ambiante. Une lecture très
différente (par exemple 0,8 V, soit « 118 °C ») indique que S2 est à droite
(Ext) sans circuit sur J5 : l'entrée flotte. Relever `A,temp_mv` avec la
commande `a` à température ambiante connue, puis reporter `TEMP_MV_REF` et
`TEMP_C10_REF` dans `lab3.h`.

## Commandes série (un caractère, terminal ou `telemetrie.py`)

`a` lectures ADC brutes et en mV, tours du tampon DMA · `q` autotest de la
file (`Q,echecs=0` attendu) · `b` banc L3-A (`B,ecarts=0` attendu une fois la
version C de `l3a_c.c` écrite, cycles) · `t`
vingt événements de test (la colonne `evq_debordements` passe à 4 avec une
file de 16) · `z` remise à zéro des compteurs.

## Sécurité

Le potentiomètre commande maintenant la vitesse : le mettre en bas avant
chaque démarrage. Les règles de l'énoncé du laboratoire 2 (encadré « Sécurité
moteur ») restent en vigueur.
