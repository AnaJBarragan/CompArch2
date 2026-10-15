# JOURNAL.md — Journal d'équipe, CEG 3536, laboratoire 2 (automne 2026)

Équipe : `Ana Barragan Martinez`, `Yassine Yandouzi` et `Simon Brown` — Section : `A02` — Dépôt Git : `https://github.com/AnaJBarragan/CompArch2`

## Jalon J1 (au plus tard le vendredi 13 Oct 2026, validé dans Git)

### Exigences de l'équipe
| Id | Exigence (reformulée par l'équipe) | Critère d'acceptation | Hypothèses |
|---|---|---|---|
| E1 | Reinitialization a ARRET | Apres reset ou mise sous tension ARRET en moins de 100 ms. DEL rouge; CCR1 = 0, AI1 = AI2 = 0, STB = 0, buzzer silencieux | PA9 High |
| E2 | PWM du moteur sur PE9 | 20 kHz +/- 5 %. Rapport cyclique de 0 a 100 %; definition de 0.5% (200 pas par cycle). Erreur max: 2% | PCLK2 = 4 MHz. PSC = 0, ARR = 199 |
| E3 | Sens du moteur | AVANT: AI1 = 1, AI2 = 0, STB = 1. ARRIERE: AI1 = 0, AI2 = 1, STB = 1. ARRET et URGENCE: tout a 0. Jamais AI1 = AI2 = 1. Sens fige si CCR1 != 0 | Avant = AI1 a 1. Le changement de sens passe par ARRET. |
| E4 | Touch EN et Rampe | Consigne: 25%,50%,75%,100%. 50% Quand reset. Rampe: 1% par 10 ms donc 0 a 100% en 1s. ARRET par User: CCR1=0 |Un pas de fsm_step = 1 ms | 
| E5 | | | |
| E6 | | | |
| E7 | | | |
| E8 | | | |
| E9 | Bips du buzzer dependant de l'etat | Bip de 100 ms +/- 10 % a chaque changement d'etat ou de consigne. Double bip en MARCHE_ARRIERE. Bip continu en urgence. | Buzzer sur PA0 en GPIO |
| E10 | SysTick: Configuration de l'horloge et interruptions| LOAD = 3999; SysTick_Handler incrémente ticks_ms et positionne tick_1ms; fsm_step est exécutée une fois par tick | Horloge CPU 4MHz, Priorite SysTick = 2 (basse) |
| E11 | | | |
| E12 | | | |


### Rôles et rotation
| Séance | Réalise | Valide (essais, mesures, relecture) |
|---|---|---|
| Séance 1 | Ana: Creer le nouveau fichier dans Git, Creer le nouveau projet dans STM32IDE et publier sur Git. Code E1-E4 & E9. Simon: Creer document rapport. Yassine: Validations.  |  |
| Séance 2 |  |  |

### Échéancier des laboratoires 1 à 5
| Laboratoire | Séances | Démonstration | Remise | Responsable du suivi |
|---|---|---|---|---|
| 1 | 0 - 15 Septembre, 1 - 22 Septembre, 2 - 29 Septembre | 29 Septembre | 9 octobre 2026 | Tous |
| 2 | 1 - 13 Octobre, 2 - 20 Octobre | 20 Octobre | 9 Novembre | Tous |
| 3 | | | | |
| 4 | | | | |
| 5 | | | | |

## Journal des séances

### Séance 1 — `13/10/2026` — réalise : `Ana` / valide : `Ana`
- Objectifs : Finir les calculs de l'horloge, creer le nouveau fichier sur github et le project sur STM32IDE. S'assurer que le projet fonctionne et les fonctionalites du lab1 sont presentes. Commencer Lab2. Completer E1, E2, E3, E4, E9 et E10 au minimum. 
- Fait : Calculs de l'horloge; projet sur STM32IDE; E1, E2, E3, E4, E9, E10 documentes sur Journal. 
- Décisions : Nous avons decide de se concentrer sur le developement du code et nous allons faire les verifications T1-T10 et prises d'ecran dans la prochaine seance pour assurer que le code est complet a temps pour la demonstration. Nous avons aussi complete la plupart du code a partir du portable avec le projet (Portable a Ana) donc la plupart des commits de la seance seront faits a partir de ceci. 
- Difficultés et solutions : Diffidulte a creer un nouveau projet sur STM32IDE, nous avons importe les launch files du lab1 pour regler le probleme. 
- Essais et mesures : Aucun - Verification dans Seance 2. 
- Validations Git (auteur, message) : Ana, 'Lab 2 -  Seance 1'

### Séance 2 — `20/10/2026` — réalise : ` ` / valide : ` `
- Objectifs :
- Fait :
- Décisions :
- Difficultés et solutions :
- Essais et mesures :
- Validations Git :

## Tableau des essais (T1 à T10)
| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 |15 octobre 2026  |  | L'essai fonctionne, le système démarre correctement à ARRÊT   |  | 
| T2 | 15 octobre 2026 |  | L'essai est conforme à l'exigence et fonctionne comme attendu. |  | 
| T3 |15 octobre 2026  |  | L'essai est conforme à l'exigence et fonctionne comme attendu. |  | 
| T4 |15 octobre 2026  |  |L'essai est conforme à l'exigence et fonctionne comme attendu.  |  | 
| T5 |15 octobre 2026  |  |L'essai est conforme à l'exigence et fonctionne comme attendu.  |  | 
| T6 |15 octobre 2026  |  |L'essai est conforme à l'exigence et fonctionne comme attendu.  |  | 
| T7 |15 octobre 2026  |  |L'essai est conforme à l'exigence et fonctionne comme attendu.  |  | 
| T8 |  |  |  |  | 
| T9 | 15 octobre 2026 |  | L'essai est conforme à l'exigence et fonctionne comme attendu. |  | 
| T10 | 15 octobre 2026 |  |  L'essai est conforme à l'exigence et fonctionne comme attendu.|  | 

## Calculs de l'Horloge
- Horloge : MSI 4 MHz (PCLK2 = 4 MHz, SysTick sur HCLK).
- PWM : N = 4 000 000 / 20 000 = 200 ; PSC = 0 ; ARR = 199 ; f = 20 000 Hz ; pas = 0,5 %.
- CCR1 : 25 % = 50 ; 50 % = 100 ; 75 % = 150 ; 100 % = 200 (= ARR + 1).
- SysTick : LOAD = 4 000 000 × 0,001 − 1 = 3999 (0xF9F).
- Fronts : 2 × 11 × 34 = 748 par tour ; fenêtre 500 ms à 100 % ≈ 2 170 fronts.

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources : 
Documents de lab fournis dans Brightspace
Code corrige du Lab 1 (fourni)
Code Complement Lab 2 (fourni)

- Outils d'IA (outil, version, usage) ou « aucun usage » : 
Claude Sonnet 5.5. Prompt: 
