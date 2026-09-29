# JOURNAL.md — Journal d'équipe, CEG 3536, laboratoire 1 (automne 2026)

Équipe : `Ana Barragan Martinez`, `Yassine Yandouzi` et `Simon Brown` — Section : `A02` — Dépôt Git : `https://github.com/AnaJBarragan/CompArch2`

## Jalon J1 (au plus tard le vendredi 25 septembre 2026, validé dans Git)

### Exigences de l'équipe
| Id | Exigence (reformulée par l'équipe) | Critère d'acceptation | Hypothèses |
|---|---|---|---|
| E1 | Initialisation du systeme | La DEL rouge s'allume lors d'appuyer sur uC Reset |  |
| E2 | Peut cycler entre les etats stop -> go -> stop -> reverse | Chaque etat est map a une couleur de DEL, elles s'allument dans la bonne sequence |  |
| E3 | Anti-Rebond | Un long appui du button compte comme un seul appui, plusieurs appuis rapides ne sont pas manques | |
| E4 | E-Stop par interruption | Les DEL bleue ou verte s'eteint quand appuie | |
| E5 | DEL E-Stop | Le DEL roughe clignote pour signaler qu'on est arrete d'urgence | |
| E6 | Touch Enable en arret | La DEL rouge passe de clignoter a rouge solide, et le button USER est utilisable a nouveau | |
| E7 | Touch Enable en fonctionnement | La DEL allume s'eteint pour un instant tres court | |
| E8 | Calibrer le delay avec l'oscilloscope | *Pas a faire, on n'utilisera pas l'oscilloscope | |
| E9 | Priorite E-Stop et Appuis simultanes | Appuyer sur User et Touch En ne genere pas deux DEL allumes, on ne sort pas de l'etat d'arret d'urgence sans Touch En. | |

### Rôles et rotation
| Séance | Réalise | Valide (essais, mesures, relecture) |
|---|---|---|
| Séance 0 | Creer le git, telecharger le code de demarrage et initializer la board. E1 | T1 Appuyer sur uC Reset |
| Séance 1 | E2, E3 | T1, T2, T3, T4 |
| Séance 2 | E4, E5, E6, E7, E9 |  |

### Échéancier des laboratoires 1 à 5
| Laboratoire | Séances | Démonstration | Remise | Responsable du suivi |
|---|---|---|---|---|
| 1 | 0 - 15 Septembre, 1 - 22 Septembre, 2 - 29 Septembre | | 9 octobre 2026 | |
| 2 | | | | |
| 3 | | | | |
| 4 | | | | |
| 5 | | | | |

## Journal des séances

### Séance 0 — `15/09/2026 (ecrit le 21/09/2026)` — réalise : `Ana` / valide : `Ana`
- Objectifs : Se familiariser avec le materiel et le programme, creer le git repo et faire marcher le premier programme pour allumer la DEL rouge. 
- Fait : Nous avons complete les 3 objectifs, le repo est maintenant cree et nous avons reussi a allumer la DEL rouge. 
- Décisions : Apres allumer la DEL rouge, nous avons essaye de commencer le Lab 1. 
- Difficultés et solutions : Apres avoir etabli que nous devions faire E1-E3, nous savions toujours pas comment le faire. Notre solution a ete de bien prendre le temps de lire la documentation avant la prochaine session pour ne pas avoir cette problematique lors de la session planifie du lab 1. 
- Essais et mesures : N/A
- Validations Git (auteur, message) : Ana, 'Journal Seance 0'

### Séance 1 — `22/09/2026 (ecrit le 22/09/2026)` — réalise : `Ana` / valide : `Ana`
- Objectifs : Ecrire le code requis pour E1, E2 et E3 et tester leur fonctionnement. 
- Fait : Le code est ecrit et nous avons pris une video des DEL rouge, bleue et verte. 
- Décisions : Apres des difficultes avec l'oscilloscope, nous avons decide d'attendre et demander au prof pendant le cours du 23 sept. 
- Difficultés et solutions : Nous avions rencontre des difficultes a utiliser l'oscilloscope et a mesurer les broches car nous n'y avons pas acces. Les broches sont proteges par une plaque plastique, et aussi on a seulement des crocodile clips dans le lab. 
- Essais et mesures : Nous n'avons pas pris des mesures, mais nous avons pris des captures d'ecran des valeures a IDR et ODR tel que requis dans le lab
- Validations Git : Ana, 'Journal Seance 1'

### Séance 2 — `<date>` — réalise : `<nom>` / valide : `<nom>`
- Objectifs : Ecrire le code pour E4-E9 (sans compter E8) et le demontrer au TA. Aussi, revoir notre code pour E1-E3 maintenant qu'on comprend mieux. 
- Fait : Code du E2 et E3 a ete revu et reecrit, E5, 56, 57, 59 sont faits. 
- Décisions : Nous avons decide de refaire E2 et E3 car on ne comprennait pas trop bien notre ancien code. 
- Difficultés et solutions : La board que nous utilisions en premier donnait une erreure sur le link du STM32 IDE. Nous avons passe a une autre board. 
- Essais et mesures : 
- Validations Git :

## Tableau des essais (T1 à T10)
| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Réinitialisation | 22 Sept | La DEL rouge s'allume lors d'appuyer sur uC Reset apres avoir telecharge le code dans la board | La board est bien initialise | |
| T2 Cycle User | | | | |
| T3 Anti-rebond | | | | |
| T4 Niveaux logiques | | | | |
| T5 E-Stop | | | | |
| T6 Clignotement | | | | |
| T7 Acquittement | | | | |
| T8 User ignoré en urgence | | | | |
| T9 Touch En hors urgence | | | | |
| T10 Robustesse | | | | |

## Routine conservée pour L3-A
- Routine : `button_pressed` ou `led_set`
- Interface :
- Cas d'essai :

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources : Documents de lab fournis dans Brightspace
- Outils d'IA (outil, version, usage) ou « aucun usage » : Claude Sonnet 5.5. Prompt: "Explain the given file line per line so that I can understand its functioning better before filling in the 'A COMPLETER' sections. "
