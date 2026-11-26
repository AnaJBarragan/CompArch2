# Scénario de désaccord — CEG 3536, projet final (composante L5-D)

À utiliser **seulement si l'équipe n'a pas connu de désaccord réel** à
documenter. L'équipe joue le scénario : chaque membre défend une position
(tirée au sort), puis l'équipe applique la démarche ci-dessous et consigne le
résultat dans `JOURNAL.md`.

## Situation

Nous sommes à trois jours de la démonstration finale. La file d'événements
déborde pendant la rafale de la commande `e`, comme prévu, mais un essai a
montré qu'un événement de défaut a été perdu une fois. Deux corrections sont
possibles, et il n'y a le temps d'en réaliser et d'en essayer qu'une.

- **Position A — rejeter le nouvel événement, avec une mémorisation à part.**
  La file reste telle quelle (le nouvel événement est refusé quand elle est
  pleine) ; les événements E-Stop et défaut refusés sont mémorisés dans des
  drapeaux relus en priorité. Changement limité à `evq.c`, facile à essayer
  sur hôte. Les événements ordinaires arrivés pendant la saturation sont perdus.
- **Position B — écraser le plus ancien événement.**
  La file garde toujours les événements les plus récents ; un événement
  prioritaire n'est jamais écrasé. L'état affiché est plus à jour après une
  rafale, mais l'algorithme touche aux deux indices dans `evq_push`, la
  section critique s'allonge et les essais existants de `test_evq` sont à
  réécrire.

Le membre A a écrit la file au laboratoire 3 et tient à ne pas la remanier à
trois jours de l'échéance. Le membre B a écrit la traduction des événements
dans `fsm.c` et constate que des événements périmés compliquent son code.

## Démarche à appliquer

1. Chacun expose sa position sans être interrompu, puis reformule celle de
   l'autre jusqu'à ce que l'autre la reconnaisse.
2. L'équipe écrit les **critères** de décision avant de discuter des options :
   par exemple la sécurité (aucun événement prioritaire perdu), la durée de
   masquage (au plus 200 cycles), le risque de régression, le temps
   disponible, la facilité de preuve par essai.
3. L'équipe cherche un **fait** qui départage : un essai court, une mesure,
   une relecture de l'exigence E14.
4. L'équipe décide, écrit la décision et sa raison, et note ce qui la ferait
   réviser.
5. Après coup : la décision a-t-elle tenu ? Qu'aurait-il fallu faire plus tôt ?

## À consigner dans le journal

| Élément | Contenu |
|---|---|
| Positions défendues et arguments | |
| Critères retenus, dans l'ordre d'importance | |
| Fait ou essai qui a départagé | |
| Décision et raison | |
| Ce que chacun retient de la démarche | |
