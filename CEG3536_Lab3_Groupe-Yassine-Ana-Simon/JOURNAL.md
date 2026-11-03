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
| Séance 1 | | | | |
| Période d'étude | | | | |
| Séance 2 | | | | |

## Calculs préalables

| Grandeur | Formule | Valeur | Vérifiée par |
|---|---|---|---|
| `RPM_Q16_K` | round(240 × 65 536 / 748) | | T3 |
| Erreur relative de la constante | (k − 240 × 65 536 / 748) / (240 × 65 536 / 748) | | T3 |
| fronts maximal sans débordement | 2³² / k | | T3 |
| `LPUART1_BRR` | 256 × 4 000 000 / 115 200 | | T9 |
| Débit ADC par voie | 4 MHz / (2 × (640,5 + 12,5)) | | T4 |
| Durée d'un tour du tampon DMA | 16 / débit par voie | | T4 |
| Résolution de la température | 3 300 mV / 4 095 / 10,9 mV/°C | | T6 |

## Étalonnage de la température (E6)

| Mesure | Température de référence (°C) | `temp_mv` lu | `TEMP_MV_REF` / `TEMP_C10_REF` retenus | Position de S2 |
|---|---|---|---|---|
| Ambiante | | | | |
| Capteur tenu entre deux doigts (30 s) | | | | |
| Conditionnement externe (pont OUT–IN ou circuit) | | | | |

Analyse interne / externe (plage, résolution, bruit) :

## Journal des séances

### Séance 1 — `<date>` — réalise : `<nom>` / valide : `<nom>`
- Objectifs :
- Fait :
- Décisions (découpage des modules, format des trames, routine L3-A choisie) :
- Difficultés et solutions :
- Mesures :
- Validations Git :

### Période d'étude — travail sur hôte
- Fait (evq, test_evq, cas ajoutés, L3-B / L3-C rédigés) :
- Validations Git :

### Séance 2 — `<date>` — réalise : `<nom>` / valide : `<nom>`
- Objectifs :
- Fait :
- Décisions :
- Difficultés et solutions :
- Mesures (température, cycles DWT, désassemblage) :
- Validations Git :

## Composante L3-A : comparaison assembleur / C

Routine comparée : `<motor_set_pwm | button_pressed | led_set | encoder_read>`

| Cas | Entrée | Résultat assembleur | Résultat C | Effet de bord (registre / variable) | Écart |
|---|---|---|---|---|---|
| 1 | | | | | |

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
| Désassemblage -O0 annoté | |
| Désassemblage -O2 annoté | |
| Trois instructions et leur encodage Thumb | |
| Captures du débogueur (R0–R3, SP, variable volatile) | |
| Cycles DWT -O0 / -O2 et transformation analysée | |

## Tableau des essais (T1 à T10)

| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Initialisation en C | | | | |
| T2 Appel C → assembleur | | | | |
| T3 Virgule fixe | | | | |
| T4 ADC et DMA | | | | |
| T5 Consigne | | | | |
| T6 Température | | | | |
| T7 File d'événements | | | | |
| T8 E-Stop | | | | |
| T9 Télémétrie | | | | |
| T10 Comparaison et traces | | | | |

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources (complément, corrigé du laboratoire 2 le cas échéant, CMSIS, manuel) :
- Outils d'IA (outil, version, usage) ou « aucun usage » :
