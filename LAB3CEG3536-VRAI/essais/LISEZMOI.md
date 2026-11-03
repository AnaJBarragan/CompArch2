# Essais du corrigé sur carte (15 septembre 2026)

Scripts Git Bash qui pilotent la carte par SWD sans arrêter le processeur
(STM32_Programmer_CLI, mode HOTPLUG). Voir le tableau des essais du `JOURNAL.md`.

| Fichier | Rôle |
|---|---|
| `swd.sh` | Lance STM32_Programmer_CLI avec les options passées, n'affiche que les lectures |
| `vars.sh` | Lit `estop_flag`, `etat`, `touch_enabled`, `compteur_transitions`, `prochain_sens`, `clignote_compteur`, `clignote_phase`, `touch_signal_compteur`, `blink_cycles` (hexadécimal et décimal) et les ODR des ports A, C, B |
| `user.sh` | Simule le bouton User : `press`, `release`, `pulse`, `restore` (PC13 mis temporairement en sortie) |
| `estop.sh` | Simule E-Stop et Touch En : `swier` (EXTI2 par `EXTI_SWIER1`), `press` / `release` (PB2 en sortie), `touch` (impulsion de 100 ms sur PB5 en sortie), `restore` (PB2 et PB5 remis en entrée) |
| `dwt_avant_1000.txt` | Échantillons du compteur de cycles DWT avant calibration |
| `dwt_apres_1270.txt` | Mêmes échantillons après calibration |

Les adresses des variables viennent de `arm-none-eabi-nm build/CEG3536_Lab1.elf`
(revérifiées le 12 octobre 2026 : `estop_flag` 0x20000034 … `blink_cycles`
0x20000054) et doivent être revérifiées si le code change. Les écritures dans
`EXTI_SWIER1` ou `GPIOx_BSRR` font afficher « Error: Failed to download data »
par STM32_Programmer_CLI : c'est l'échec de relecture d'un registre en écriture
seule, l'écriture a bien eu lieu. Toujours exécuter `user.sh restore`
et remettre `GPIOB_MODER` à `0xFFFF728F` après un essai : une broche de bouton
laissée en sortie peut entrer en conflit avec le bouton physique.

Calcul de la période : (CYCCNT2 − CYCCNT1) / (compteur2 − compteur1) cycles par
pas, divisé par 4000 cycles/ms (MSI 4 MHz, 1 cycle ≈ 1 instruction de la boucle).
Depuis le 12 octobre 2026, `blink_cycles` donne directement l'écart de
`DWT_CYCCNT` entre deux basculements de la rouge (demi-période) ; période en ms
= 2 × `blink_cycles` / 4000.
