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
| E5 |Lecture de l’encodeur et Interruption| deux fronts E_A déclenchent EXTI0. Priorité:   SysTick < EXTI0 < E-Stop. ISR: Lit E_A et E_B dans IDR, trouve le sens si EA=EB ou EA!=EB, incrémente ou décrémente enc_count, efface RPR1/FPR1 et se termine. Aucun front se perds à pleine vitesse (environ 4400 fronts par seconde). L’ISR dure moins de 15 micro secondes.| |
| E6 |Vitesse et sens | FENETRE_VITESSE_MS = 500ms, main copie la variation de enc_count dans vitesse_fronts et sens_mesure, mise à jour du repère. Tr/min = fronts * 120/748 pour une fenêtre de 500 ms. Compare le sens mesuré à commandé.| |
| E7 |E-Stop Interruption PWM|EXTI2_IRQHandler: CCR1 = 0, STB = 0 et AI1=AI2=0, DEL bleue et vertes éteint. Allume DEL Rouge. Buzzer à 1, et positionne drapeau estop. Efface requête d’interruption et se termine. Gestion de transition d’état complet maintenant gérer par fsm_step. Délai entre front sur PB2 et PWM sur PE9 < 1 ms. | |
| E8 |Arret d’urgence|    État ARRÊT_URGENCE : (DEL Rouge clignote à 2 Hz (+-10%) buzzer continue à faire du son sans arrêté. STB reste à 0. User et Touch En ignorés. Touch En touché + E-Stop relâché => État = ARRÊT), État ARRÊT : (Buzzer arrête de sonnée, DEL Rouge fixe. Consigne conservée. Moteur redémarre qu’après un nouvel appui sur btn User.) | |
| E9 | Bips du buzzer dependant de l'etat | Bip de 100 ms +/- 10 % a chaque changement d'etat ou de consigne. Double bip en MARCHE_ARRIERE. Bip continu en urgence. | Buzzer sur PA0 en GPIO |
| E10 | SysTick: Configuration de l'horloge et interruptions| LOAD = 3999; SysTick_Handler incrémente ticks_ms et positionne tick_1ms; fsm_step est exécutée une fois par tick | Horloge CPU 4MHz, Priorite SysTick = 2 (basse) |
| E11 |Mesures au débogueur|     Captures du débogueur de fréquence et rapport cyclique de PWM qui est à 25, 50, 75 et 100% (Calcués des registres PSC, ARR et CCR1).  Niveaux AI1, AI2 et STB dans chaque du système. Évolution de la variable enc_count pour marche_avant et marche_arrière. Délai entre E-Stop et mise à 0 de PWM. Durée de bip mesurée en ticks_ms | |
| E12 |Robustesse| E-Stop pendant une rampe, Touch En pendant une rampe, User pendant une rampe, appuis de boutons en même temps, E-Stop pendant l’ISR de l’encodeur. Jamais de AI1=AI2=1. Et jamais de changement de sens à CCR1 != 0.  Ne jamais sortir de ARRÊT_URGENCE sans acquittement. | |


### Rôles et rotation
| Séance | Réalise | Valide (essais, mesures, relecture) |
|---|---|---|
| Séance 1 | Ana: Creer le nouveau fichier dans Git, Creer le nouveau projet dans STM32IDE et publier sur Git. Code E1-E4 & E9. Simon: Creer document rapport. Yassine: Validations.  |  |
| Séance 2 | Ana: finition du programme dans STM32, Simon: Validation des résultats et rapport de laboratoire, Yassine: Validation des résultats |  |

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

### Séance 2 — `20/10/2026` — réalise : `Yassine ` / valide : `Yassine `
- Objectifs : Finir les essaie (t1 à t10), effectuer la démonstration au TA et début du rapport de laboratoire.
- Fait : Démonstration au TA et prise de mesure pour T1 à T10
- Décisions : Nous avons décider de nous concentrer sur la prise de mesure lors de nos essaie et de la démonstration
- Difficultés et solutions : Nous avons eu de la difficulté avec la précision les prise de mesure de temps (ex: E_Stop). En effet, les intervales de temps sont très petite donc la précisions ce devait d'être conforme aux attentes.
- Essais et mesures : Les essais T1 à T10 on été effectués conformement aux attentes.
- Validations Git : Yassine, lab2 - seance 1

## Tableau des essais (T1 à T10)
| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 |15 octobre 2026  |DEL rouge allumé seule, CCR1=0, STB=0 et le moteur est immobile  | L'essai fonctionne, le système démarre correctement à ARRÊT   |Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot| 
| T2 | 15 octobre 2026 |La séquence rouge-vert-rouge-bleu fonctionne conformément aux exigences  | L'essai est conforme à l'exigence et fonctionne comme attendu. | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T3 |15 octobre 2026  |Rapport cyclique sont conforme (25%, 50% et 100%) et PWM=20kHz| L'essai est conforme à l'exigence et fonctionne comme attendu. | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T4 |15 octobre 2026  |pmw_pct monte de 0 à 100 en 1 seconde, conformément aux attentes |L'essai est conforme à l'exigence et fonctionne comme attendu.  | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T5 |15 octobre 2026  |enc_count monte en AVANT et descend en ARRIÈRE avec environs 4400front/s avec une consigne à 100%.   |L'essai est conforme à l'exigence et fonctionne comme attendu.  | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T6 |15 octobre 2026  |La vitesse est croissante avec 170,273 et 367tour/min pour 25%, 50% et 100% |L'essai est conforme à l'exigence et fonctionne comme attendu.  | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T7 |15 octobre 2026  | STB ET PWM sont coupés, estop_flag=1 et le buzzer est activé.  |L'essai est conforme à l'exigence et fonctionne comme attendu.  | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T8 |15 octobre 2026  |User est ignoré, l'acquittement est refusé |L'essai est conforme à l'exigence et fonctionne comme attendu.  | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T9 | 15 octobre 2026 |Bip de 99ms   | L'essai est conforme à l'exigence et fonctionne comme attendu. | Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot | 
| T10 | 15 octobre 2026 |Période Systhick de 1ms  |  L'essai est conforme à l'exigence et fonctionne comme attendu.| Voir le fichier LAB2CEG3536-essai(t1-t10) dans le dêpot| 

## 6.Routine conservée pour L3-A: 
- Nom : Motor_Set_pwm 
- Interface : Reçoit le rapport cyclique demandé (0 à 100%) dans R0 et retourne la valeur écrite dans IM1_CCR1 ; toute valeur supérieure à 100 % est saturée à 100 %. 
- Cas d’essai: On donne à la routine différentes consignes (0, 25, 50, 75 et 100 %) et on vérifie que CCR1 sort 	la bonne valeur à chaque fois (0, 50, 100, 150, 200). Si on donne plus de 100 % , la routine 	doit se limiter à 100 % (ne peut dépasser 100). 
- Résultats: Vérifié lors de T3, avec PSC = 0 et ARR = 199, on obtient CCR1 = 50, 100, 150 et 200 pour 25, 50, 	75 et 100 %, soit un rapport cyclique exact.  

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
