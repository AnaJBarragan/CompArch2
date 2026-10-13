# CEG 3536 — Laboratoire 1 — Projet de démarrage (automne 2026)

Projet STM32CubeIDE en **assembleur ARM** pour la carte **NUCLEO-L552ZE-Q**
(STM32L552ZET6QU, Cortex-M33), plateforme « Leafy ». Il correspond à l'énoncé
*Commandes et signalisation du véhicule : RCC/GPIO, DEL, boutons, anti-rebond et
arrêt d'urgence en assembleur ARM* (version 1.0, 12 septembre 2026).

Le projet **compile sans avertissement et se charge tel quel**. À la séance 0,
il réalise l'objectif attendu : après réinitialisation, la DEL rouge (PA9) est
allumée seule (exigence E1). Les autres exigences (E2 à E9) sont à réaliser dans
les routines marquées **À COMPLÉTER**.

## 1. Contenu

| Fichier | Rôle | État |
|---|---|---|
| `Core/Src/registres.inc` | Adresses, masques et constantes (`.equ`, noms RM0438) : RCC, GPIO, EXTI, NVIC, brochage, niveaux actifs, temporisations | Fourni — **niveaux actifs et PUPDR à confirmer par mesure (T4)** |
| `Core/Src/main.s` | `SystemInit` (vide, horloge MSI 4 MHz), `main` : `gpio_init`, `estop_init`, `fsm_init`, boucle `fsm_step` + `delay_ms(1)` | Fourni |
| `Core/Src/gpio.s` | `gpio_init` (RCC_AHB2ENR, MODER, OTYPER, OSPEEDR, PUPDR), `led_set` (BSRR) | Fourni |
| `Core/Src/buttons.s` | `button_raw` (IDR normalisé « appuyé = 1 »), `button_pressed` (anti-rebond, front), `delay_ms` (boucle calibrée) | `button_raw` et `delay_ms` fournis, **`button_pressed` à compléter (E3)** |
| `Core/Src/estop.s` | `estop_init` (EXTI_EXTICR1, RTSR1/FTSR1, IMR1, NVIC), `EXTI2_IRQHandler`, `estop_flag` | **`estop_init` à faire, ISR à compléter (E4)** ; effacement de la requête fourni |
| `Core/Src/fsm.s` | `fsm_init`, `fsm_step`, `fsm_maj_del` (seul point de mise à jour des DEL), variables `etat`, `touch_enabled`, `compteur_transitions` | `fsm_init` fourni, **transitions E2, E5, E6, E7 à compléter** |
| `Core/Inc/lab1.h` | Prototypes C des routines (réutilisés au laboratoire 3) | Fourni |
| `Core/Startup/startup_stm32l552zetxq.s` | Vecteurs d'interruption et démarrage, **généré par STM32CubeIDE** (à déclarer comme tel dans le rapport) | Fourni, ne pas modifier |
| `Core/Src/syscalls.c`, `Core/Src/sysmem.c` | Fichiers de support minimaux **générés par STM32CubeIDE** (évitent les avertissements de l'éditeur de liens) ; aucun code applicatif | Fourni, ne pas modifier |
| `STM32L552ZETXQ_FLASH.ld` | Script d'édition de liens généré par STM32CubeIDE | Fourni |
| `Makefile` | Construction et programmation en ligne de commande (facultatif) | Fourni |
| `JOURNAL.md` | Journal d'équipe et jalon J1 (modèle) | À tenir à chaque séance |

## 2. Ouvrir le projet dans STM32CubeIDE 2.2.0

1. Copier le dossier `CEG3536_Lab1_Demarrage` dans votre espace de travail et le
   renommer `CEG3536_Lab1_<équipe>` (renommer aussi dans *Project > Properties*
   après l'import, ou laisser CubeIDE le faire avec *Rename*).
2. *File > Import… > General > Existing Projects into Workspace*, choisir le dossier,
   cocher le projet, *Finish*.
3. *Project > Build Project* (marteau). La console doit se terminer sans
   `warning`. Le fichier `Debug/CEG3536_Lab1_<équipe>.elf` est produit.
4. Brancher la Nucleo (ST-LINK), *Run > Debug As > STM32 C/C++ Application*.
   Le débogueur s'arrête sur `main` ; *Resume* (F8) : la DEL rouge s'allume.
5. Fenêtres utiles : *Window > Show View > SFRs* (registres RCC, GPIOx, EXTI, NVIC),
   *Expressions* ou *Memory* pour `etat`, `estop_flag`, `touch_enabled`,
   `compteur_transitions`, `btn_valide`, `btn_compteur`.

Le projet est de type « Empty » : pas de HAL, pas de CubeMX. Le préprocesseur C
est actif sur les fichiers `.s` (option `-x assembler-with-cpp`), ce qui permet
`#include "registres.inc"` et les expressions comme `(3 << (2 * LED_ROUGE_PIN))`.

## 3. Construction en ligne de commande (facultatif)

```
make            # build/CEG3536_Lab1.elf et .bin (STM32CubeCLT 1.21.0)
make flash      # programme la carte par ST-LINK
```

Le `make` de STM32CubeIDE se trouve dans
`C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_*\tools\bin`.

## 4. Ce qu'il reste à faire, exigence par exigence

| Exigence | Où | Quoi |
|---|---|---|
| E1 Initialisation | `fsm_init` | Fourni : ARRÊT, rouge seule |
| E2 Défilement par User | `fsm_step` (étape B) | Transitions ARRÊT → AVANT → ARRÊT → ARRIÈRE → ARRÊT, `compteur_transitions++` |
| E3 Anti-rebond et front | `button_pressed` | Fenêtre `ANTIREBOND_MS` par comptage d'échantillons ; un appui = un événement |
| E4 E-Stop par interruption | `estop_init`, `EXTI2_IRQHandler` | EXTICR1/RTSR1 ou FTSR1/IMR1, priorité NVIC 0, ISR : état sûr + drapeau + effacement |
| E5 Signalisation d'urgence | `fsm_step` (A), `fsm_maj_del` | Clignotement 2 Hz par `clignote_compteur` / `clignote_phase`, User ignoré |
| E6 Acquittement | `fsm_step` (B) | Touch En validé ET `button_raw(BTN_ESTOP) == 0` → ARRÊT |
| E7 Touch En hors urgence | `fsm_step` (B), `fsm_maj_del` | `touch_enabled ^= 1`, extinction brève `TOUCH_SIGNAL_MS` |
| E8 Temporisation | `delay_ms`, `registres.inc` | Calibrer `DELAY_BOUCLES_PAR_MS` à l'oscilloscope et rapporter la valeur |
| E9 Robustesse | ensemble | Vérifier les invariants avec les essais T10 |

## 5. Points d'attention pour la STM32L5

- **EXTICR est dans EXTI, pas dans SYSCFG** (RM0438, section 16) :
  `EXTI_EXTICR1` à l'offset 0x60 de `EXTI_BASE` (0x4002F400), champ EXTI2 aux
  bits 18:16, valeur 1 = port B. `RCC_APB2ENR.SYSCFGEN` n'est pas nécessaire.
- **Deux registres de requête** : `EXTI_RPR1` (front montant) et `EXTI_FPR1`
  (front descendant), effacés par écriture de 1 (fourni dans l'ISR).
- **EXTI2_IRQn = 13** : priorité dans l'octet `0xE000E400 + 13`, activation par
  `NVIC_ISER0` bit 13. 3 bits de priorité utiles (7:5) ; 0x00 = la plus élevée.
- **TrustZone désactivée (TZEN = 0)** : les périphériques sont aux adresses
  0x4xxx_xxxx (alias non sécurisé) comme dans les exemples du cours.
- **AAPCS** : paramètres R0–R3, retour R0 ; préserver R4–R11 modifiés ; sauver LR
  si la routine appelle ; pile alignée sur 8 octets aux appels publics
  (`push {r4, lr}` ou `push {r4, r5, r6, lr}`, jamais un nombre impair de mots).
- **ISR** : le matériel empile R0–R3, R12, LR, PC, xPSR ; une ISR feuille
  utilise R0–R3 librement et retourne par `bx lr`.

## 6. Mesures à consigner dans le rapport

Niveaux logiques de PC13, PB2, PB5 relâché/appuyé (T4) → ajuster
`BTN_x_ACTIF_HAUT` et `BTN_x_PUPDR` dans `registres.inc` ; durée de rebond sur
PC13 ; fenêtre d'anti-rebond retenue ; période de clignotement (T6) ; délai
PB2 → PC7/PB7 (T5) ; valeur calibrée de `DELAY_BOUCLES_PAR_MS` (E8).
