# JOURNAL.md — Journal d'équipe, CEG 3536, laboratoire 1 (automne 2026)

Équipe : `Ana Barragan Martinez`, `Yassine Yandouzi` et `Simon Brown` — Section : `A02` — Dépôt Git : `https://github.com/AnaJBarragan/CompArch2`

## Jalon J1 (au plus tard le vendredi 25 septembre 2026, validé dans Git)

### Exigences de l'équipe
| Id | Exigence (reformulée par l'équipe) | Critère d'acceptation | Hypothèses |
|---|---|---|---|
| E1 | Initialisation du systeme | La DEL rouge s'allume lors d'appuyer sur uC Reset | PA9 High |
| E2 | Peut cycler entre les etats stop -> go -> stop -> reverse | Chaque etat est map a une couleur de DEL, elles s'allument dans la bonne sequence | Appui sur PC13 = PA9 -> PC7 -> PB7 -> PA9 |
| E3 | Anti-Rebond | Un long appui du button compte comme un seul appui, plusieurs appuis rapides ne sont pas manques | Watch sur la variable compte un seul appui |
| E4 | E-Stop par interruption | Les DEL bleue ou verte s'eteint quand appuie | Appui sur PB2 -> EXTI2 Low PB7/PC7  |
| E5 | DEL E-Stop | Le DEL roughe clignote pour signaler qu'on est arrete d'urgence | PB9 Clignote a 2Hz, aucune entree sure PC13 |
| E6 | Touch Enable en arret | La DEL rouge passe de clignoter a rouge solide, et le button USER est utilisable a nouveau | Appui sur PB5 = PA9 Solide, PC13 prend des entrees |
| E7 | Touch Enable en fonctionnement | La DEL allume s'eteint pour un instant tres court | Appui sur PB5 = eteint PA9 brevement |
| E8 | Calibrer le delay avec l'oscilloscope | *Pas a faire, on n'utilisera pas l'oscilloscope | * |
| E9 | Priorite E-Stop et Appuis simultanes | Appuyer sur User et Touch En ne genere pas deux DEL allumes, on ne sort pas de l'etat d'arret d'urgence sans Touch En. | Jamais deux DEL (PA9, PB7, PC7) allumes simultanement |

### Rôles et rotation
| Séance | Réalise | Valide (essais, mesures, relecture) |
|---|---|---|
| Séance 0 | Creer le git, telecharger le code de demarrage et initializer la board. E1 | T1 Appuyer sur uC Reset |
| Séance 1 | E2, E3 | T1, T2, T3, T4 |
| Séance 2 | E4, E5, E6, E7, E9 | T5, T6, T7, T9 |

### Échéancier des laboratoires 1 à 5
| Laboratoire | Séances | Démonstration | Remise | Responsable du suivi |
|---|---|---|---|---|
| 1 | 0 - 15 Septembre, 1 - 22 Septembre, 2 - 29 Septembre | 29 Septembre | 9 octobre 2026 | Tous |
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

### Séance 2 — `29/09/2026` — réalise : `Ana` / valide : `Ana`
- Objectifs : Ecrire le code pour E4-E9 (sans compter E8) et le demontrer au TA. Aussi, revoir notre code pour E1-E3 maintenant qu'on comprend mieux. 
- Fait : Code du E2 et E3 a ete revu et reecrit, E5, 56, 57, 59 sont faits. 
- Décisions : Nous avons decide de refaire E2 et E3 car on ne comprennait pas trop bien notre ancien code. 
- Difficultés et solutions : La board que nous utilisions en premier donnait une erreure sur le link du STM32 IDE. Nous avons passe a une autre board. 
- Essais et mesures : Demonstration au TA
- Validations Git : Ana, 'Journal Seance 2 (Mise a jour)'

## Tableau des essais (T1 à T10)
| Essai | Date | Résultat observé | Verdict | Preuve (fichier) |
|---|---|---|---|---|
| T1 Réinitialisation | 22 Sept | La DEL rouge s'allume lors d'appuyer sur uC Reset apres avoir telecharge le code dans la board | La board est bien initialise | Voir annexe du rapport et video |
| T2 Cycle User | 22 Sept | Apuyer sur User cycle de rouge -> vert -> rouge -> blueu | Mise a jour des etats de marche et arret es correcte | Voir annexe du rapport et video |
| T3 Anti-rebond | 22 Sept | Des longs appuis comptent une seule fois | Le anti-rebond est bien implemente | Voir annexe du rapport et video |
| T4 Niveaux logiques | 29 Oct | PC13, PB2 et PB5 lus dans le registre IDR | PUPD fonctionnels | Voir annexe du rapport et video |
| T5 E-Stop | 29 Oct | DEL verte et bleue eteintes | Passe a l'etat arret d'urgence correctement | Voir annexe du rapport et video |
| T6 Clignotement | 29 Oct | DEL rouge clignote en etat arret d'urgence | etat arret d'urgence valide | Voir annexe du rapport et video |
| T7 Acquittement | 29 Oct | Retour a DEL rouge solide lors de Touch-En | Systeme passe de l'etat arret durgence a arret | Voir annexe du rapport et video |
| T8 User ignoré en urgence | 29 Oct | Aucune entree sur User ne change pas le DEL | L'etat du systeme ne change pas a marche avant ou arriere sans acquittement | Voir annexe du rapport et video |
| T9 Touch En hors urgence | 29 Oct | Appuyer Touch-En en etat non arret d'urgence eteint le DEL brevement | touch_en passe a 1 et 0 | Voir annexe du rapport et video |
| T10 Robustesse | 29 Oct | Appuyer sur e-stop et user rend le systeme en etat d'urgence | E-stop prend priorite | Voir annexe du rapport et video |

## Routine conservée pour L3-A
- Routine : `button_pressed`
- Interface : reçoit l'identifiant du bouton (BTN_USER, BTN_ESTOP ou BTN_TOUCH) dans R0 et retourne 1 lorsqu'un nouvel appui est validé. 
- Cas d'essai : maintenir le bouton User appuyé pendant plusieurs secondes doit produire un seul événement; après relâchement puis nouvel appui, un nouvel événement doit être produit.

- Routine : `led_set`
- Interface : reçoit LED_ROUGE, LED_VERTE ou LED_BLEUE dans R0 et allume uniquement la DEL demandée.
- Cas d'essai : appeler led_set(LED_ROUGE), led_set(LED_VERTE) et led_set(LED_BLEUE) une seule DEL doit être allumée.

## Déclaration des sources et de l'usage d'outils d'IA générative
- Sources : Documents de lab fournis dans Brightspace
- Outils d'IA (outil, version, usage) ou « aucun usage » : 

Claude Sonnet 5.5. Prompt: "Explain the given file line per line so that I can understand its functioning better before filling in the 'A COMPLETER' sections. "

Claude Sonnet 5.5. Prompt: "I need to combine the main branch and the 'Ana' branch. The main branch contains all the functional and finished code. The Ana branch contains the most updated version of Journal.md. Do i do a rebase or a merge? give me the line commands and explain what exactly they do. Do not do it for me. "