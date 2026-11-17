# JOURNAL.md — Journal d'équipe, CEG 3536, laboratoire 3 (automne 2026)

Équipe : `<NomA>` et `<NomB>` — Section : `A01 / A02` — Dépôt Git : `<adresse>`

## Jalon J2 — module d'événements testé sur hôte

| Plateforme | Compilateur et version | Commande | Résultat (cas réussis / échecs) | Date et validation Git |
|---|---|---|---|---|
| Windows |gcc Chocolatay v2.5.1| `make` dans `hote/` |9 cas réussis, 0  échecs |16 novembre 2026 |
| Linux |gcc 13.3.0 (Ubuntu)| `make` dans `hote/|9 cas réussis, 0  échecs |16 novembre 2026 |
| STM32 (cible) | arm-none-eabi-gcc | commande série `q` |9 cas réussis, 0  échecs |16 novembre 2026 |



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
| Ambiante |25°C | 1800mV| aucun| Int(gauche)|
| Capteur tenu entre deux doigts (30 s) | 32°C|1770mV |aucun |Int (gauche) |
| Conditionnement externe (pont OUT–IN ou circuit) |25°C|1820mV| aucun |Ext (droite) |

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
| Assembleur |environs 15 cycles | 24 octets|r0,r1,r2 |Aucun prologue, rien n'est volatile |
| C, -O0 |environs 30 cycles | 64 octets|r0,r2,r3,r7 |Prologue pour push/sub, str est conservée (volatile) |
| C, -O2 |environs 6 cycles |20 octets |r0,r3 |Aucun prologue, str est conservée (volatile) |

## Composante L3-C : objet d'acquisition

- Version 1 (validation Git) :
- Version 2 (validation Git) et ce qui a changé :
- Essai d'indépendance des deux instances :
- Comparaison avec la version impérative (annexe) :
- Limites :

## Composante L3-D : traces

| Élément | Preuve (fichier) |
|---|---|
| Désassemblage -O0 annoté |L3d_desassemblage_00.txt |
| Désassemblage -O2 annoté |L3d_desassemblage_02.txt |
| Trois instructions et leur encodage Thumb |l3_encodage_thumb|
| Captures du débogueur (R0–R3, SP, variable volatile) |voir table d'essais (T2) |
| Cycles DWT -O0 / -O2 et transformation analysée |Voir table d'essais (T10)|

## Tableau des essais (T1 à T10)

| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Initialisation en C | 16 novembre 2026| Après la réinitialisation, le programme démarre en C. Le véhicule est en ARRÊT,  la DEL rouge est allumée seule et le moteur ne bouge pas (CCR1 = 0). La ligne d'en-tête H est envoyée dans le terminal et la première trame arrive en moins de 300 ms. |Le résultat observé fonctionne selon les attentes de laboratoire  | test-t1-t10-lab3-ceg3536.pdf |
| T2 Appel C → assembleur |16 novembre 2026 | On met un point d'arrêt dans motor_set_pwm appelé depuis le C. On voit le paramètre dans R0, la valeur de retour dans R0 et SP est un multiple de 8. Les variables sont déclarer en mémoire. |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T3 Virgule fixe | 16 novembre 2026| On appelle la routine sur les cas 0, 1, 748, 1870 et 65535. Les résultats sont 0, 0,32, 240,00, 600,01 et 21027 tr/min. L'erreur est bien en dessous de 0,05 %. |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T4 ADC et DMA |16 novembre 2026 |Le potentiomètre est lu par l'ADC avec le DMA. En bas on lit 0, au milieu 2048 et en haut 4095. Le débit par voie est d'environ 3 kHz, comme calculé. |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T5 Consigne |16 novembre 2026 |En marche, la consigne suit le potentiomètre de 0 à 100 % avec la rampe. En ARRÊT, le potentiomètre n'a aucun effet (CCR1 reste à 0). L'hystérésis évite les oscillations au repos. |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T6 Température |16 novembre 2026 |La température est étalonnée avec le thermomètre de référence. Au repos, l'écart est de moins de 2 °C. Quand on tient le capteur entre les doigts pendant 30 s, la température monte de plusieursdegrés. | Le résultat observé fonctionne selon les attentes de laboratoire| test-t1-t10-lab3-ceg3536.pdf|
| T7 File d'événements |16 novembre 2026 |Fonctionne selon les attentes |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T8 E-Stop | 16 novembre 2026|Quand on appuie sur l'E-Stop, le PWM et STB sont coupés en  moins de 1 ms, comme au lab 2 (le gestionnaire d'interruption n'a pas changé). Le système passe en arret URGENCE. Les trames de télémétrie restent intactes et le moteur ne redémarre pas tant qu'on n'a pas acquitté avec Touch En. |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T9 Télémétrie | 16 novembre 2026|Le SysTick garde sa période de 1 ms (déjà validée au lab 2), ce qui permet d'émettre une trame toutes les 200 ms, donc à 5 Hz. On a enregistré 30 secondes de trames et le script telemetrie.py confirme le bon format et la cadence (150 trames). |Le résultat observé fonctionne selon les attentes de laboratoire |test-t1-t10-lab3-ceg3536.pdf |
| T10 Comparaison et traces |16 novembre 2026 |On compare la version assembleur de motor_set_pwm (dont le rapport avait été validé au lab 2) avec la version C. Les deux donnent exactement les mêmes résultats, donc zéro écart.  | Le résultat observé fonctionne selon les attentes de laboratoire|test-t1-t10-lab3-ceg3536.pdf |

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources (complément, corrigé du laboratoire 2 le cas échéant, CMSIS, manuel) :
- Outils d'IA (outil, version, usage) ou « aucun usage » :
