# JOURNAL.md — Journal d'équipe, CEG 3536, laboratoire 3 (automne 2026)

Équipe : `<NomA>` et `<NomB>` — Section : `A01 / A02` — Dépôt Git : `<adresse>`

## Jalon J2 — module d'événements testé sur hôte

| Plateforme | Compilateur et version | Commande | Résultat (cas réussis / échecs) | Date et validation Git |
|---|---|---|---|---|
| Windows | | `make` dans `hote/` | | |
| Linux | | | | |
| STM32 (cible) | arm-none-eabi-gcc | commande série `q` | | |

Cas ajoutés par l'équipe (au-delà des 9 fournis) :

| Cas | Description | Résultat |
|---|---|---|
| 10 | | |

Différences observées entre hôtes (tailles des types, options, avertissements) :

## Rôles et rotation

| Séance | Réalise | Valide | Module C écrit par chacun | Routine assembleur écrite ou modifiée par chacun |
|---|---|---|---|---|
| Séance 1 |Ana, Simon, Yassine|Ana, Simon, Yassine |main, fsm, evq,acq, adc_dma,telem   |vitesse.s |
| Séance 2 | Ana, Simon, Yassine| Ana, Simon, Yassine| aucun module ecrit, seulement les test de validations|aucun |

## Calculs préalables

| Grandeur | Formule | Valeur | Vérifiée par |
|---|---|---|---|
| `RPM_Q16_K` | round(240 × 65 536 / 748) |21028 | T3 |
| Erreur relative de la constante | (k − 240 × 65 536 / 748) / (240 × 65 536 / 748) |≈0,0019% | T3 |
| fronts maximal sans débordement | 2³² / k | 204249| T3 |
| `LPUART1_BRR` | 256 × 4 000 000 / 115 200 |0x22B9 | T9 |
| Débit ADC par voie | 4 MHz / (2 × (640,5 + 12,5)) | 3,06 kHz| T4 |
| Durée d'un tour du tampon DMA | 16 / débit par voie | 5,21ms| T4 |
| Résolution de la température | 3 300 mV / 4 095 / 10,9 mV/°C |≈0,07°C | T6 |

## Étalonnage de la température (E6)

| Mesure | Température de référence (°C) | `temp_mv` lu | `TEMP_MV_REF` / `TEMP_C10_REF` retenus | Position de S2 |
|---|---|---|---|---|
| Ambiante |24°C | 1800mV| aucun| Int(gauche)|
| Capteur tenu entre deux doigts (30 s) | 32°C|1770mV |aucun |Int (gauche) |
| Conditionnement externe (pont OUT–IN ou circuit) |1820mV | |aucun |Ext (droite) |

Analyse interne / externe (plage, résolution, bruit) :

## Journal des séances

### Séance 1 — `<date>` — réalise : `<nom>` / valide : `<nom>`
- Objectifs :  Finir les calculs de l'horloge, creer le nouveau fichier sur github et le project sur STM32IDE. S'assurer que le projet fonctionne et les fonctionalites du lab1 sont presentes. Commencer Lab3. Completer E1, E2, E3, E4, E9 et E10 au minimum. 
- Fait : Calculs de l'horloge; projet sur STM32IDE; E1, E2, E3, E4, E9, E10 documentes sur Journal.  
- Décisions (découpage des modules, format des trames, routine L3-A choisie) : : Nous avons décidé de garder le projet du laboratoire 2 comme base et de retirer main.s et fsm.s, remplacés par main.c et fsm.c. Pour le découpage, on a séparé le code en modules : evq.c pour la file d'événements, acq.c pour l'acquisition, adc_dma.c pour l'ADC et le DMA, telem.c pour la télémétrie, et fsm.c pour la machine à états. On a aussi choisi le format des trames CSV avec les dix colonnes demandées, terminées par \r\n, et une ligne d'en-tête H,... au démarrage. Pour la routine L3-A, on a décidé de comparer motor_set_pwm avec sa version en C, parce qu'elle est courte et facile à vérifier.
- Difficultés et solutions : Difficulté à créer un nouveau projet sur STM32IDE, nous avons importé les launch files du lab1 pour régler le problème. 
- Mesures : Aucun - Vérification dans Seance 2.  
- Validations Git : Ana, 'Lab 3 - Séance 1'

### Séance 2 — `<date>` — réalise : `<nom>` / valide : `<nom>`
- Objectifs : : Finir les essaie (t1 à t10), effectuer la démonstration au TA et début du rapport de laboratoire.
- Fait : Essais t1-t10
- Décisions : Nous avons décidé de nous concentrer sur la prise de mesure lors de nos essaie et de la démonstration
- Difficultés et solutions : Nous avons eu de la difficulté avec la précision les prise de mesure de temps (ex: E_Stop). En effet, les intervalles de temps sont très petits donc la précisions ce devait d'être conforme aux attentes.
- Mesures (température, cycles DWT, désassemblage) : Les essais T1 à T10 on été effectués conformément aux attentes. 
- Validations Git :Yassine, lab3 - séance 2

## Composante L3-A : comparaison assembleur / C

Routine comparée : `<motor_set_pwm `

| Cas | Entrée | Résultat assembleur | Résultat C | Effet de bord (registre / variable) | Écart |
|---|---|---|---|---|---|
| 1 | 100 | 200 | 200 | CCR1= 200 |0 |

| Version | Cycles DWT (moyenne) | Taille (octets) | Registres utilisés | Observations (prologue, volatile) |
|---|---|---|---|---|
| Assembleur | | | | |
| C, -O0 | | | | |
| C, -O2 | | | | |

## Composante L3-C : objet d'acquisition

- Version 1 (validation Git) :
- Version 2 (validation Git) et ce qui a changé :
- Essai d'indépendance des deux instances :
- Comparaison avec la version impérative (annexe) :
- Limites :

## Composante L3-D : traces

| Élément | Preuve (fichier) |
|---|---|
| Désassemblage -O0 annoté |
| Désassemblage -O2 annoté | |
| Trois instructions et leur encodage Thumb ||
| Captures du débogueur (R0–R3, SP, variable volatile) | |
| Cycles DWT -O0 / -O2 et transformation analysée ||

## Tableau des essais (T1 à T10)

| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Initialisation en C | 16 novembre 2026| | | |
| T2 Appel C → assembleur |16 novembre 2026 | | | |
| T3 Virgule fixe | 16 novembre 2026| | | |
| T4 ADC et DMA |16 novembre 2026 | | | |
| T5 Consigne |16 novembre 2026 | | | |
| T6 Température |16 novembre 2026 | | | |
| T7 File d'événements |16 novembre 2026 | | | |
| T8 E-Stop | 16 novembre 2026| | | |
| T9 Télémétrie | 16 novembre 2026| | | |
| T10 Comparaison et traces |16 novembre 2026 | | | |

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources (complément, corrigé du laboratoire 2 le cas échéant, CMSIS, manuel) :
- Outils d'IA (outil, version, usage) ou « aucun usage » :
