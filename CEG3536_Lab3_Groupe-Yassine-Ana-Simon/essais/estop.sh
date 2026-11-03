#!/bin/bash
# estop.sh : simule le bouton E-Stop (PB2) ou l'acquittement Touch En (PB5)
#   swier    : déclenche EXTI2 par logiciel (EXTI_SWIER1), sans toucher PB2
#   press    : PB2 en sortie à 1 (E-Stop maintenu)      release : PB2 à 0
#   touch    : PB5 en sortie à 1 puis 0 après 100 ms (appui Touch En validé)
#   restore  : PB2 et PB5 remis en entrée (GPIOB_MODER = 0xFFFF728F)
S="$(dirname "$0")"
case "$1" in
  swier)   bash "$S/vars.sh" -w32 0x4002F408 0x00000004 ;;
  press)   bash "$S/vars.sh" -w32 0x42020400 0xFFFF729F -w32 0x42020418 0x00000004 ;;
  release) bash "$S/vars.sh" -w32 0x42020418 0x00040000 ;;
  touch)   bash "$S/vars.sh" -w32 0x42020400 0xFFFF769F -w32 0x42020418 0x00000020
           sleep 0.1
           bash "$S/vars.sh" -w32 0x42020418 0x00200000 ;;
  restore) bash "$S/vars.sh" -w32 0x42020418 0x00240000 -w32 0x42020400 0xFFFF728F ;;
esac
