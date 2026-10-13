#!/bin/bash
S="$(dirname "$0")"
case "$1" in
  press)   bash "$S/vars.sh" -w32 0x42020800 0xF7FF7FFF -w32 0x42020814 0x00002000 ;;
  release) bash "$S/vars.sh" -w32 0x42020814 0x00000000 ;;
  pulse)   bash "$S/vars.sh" -w32 0x42020814 0x00002000 -w32 0x42020814 0x00000000 ;;
  restore) bash "$S/vars.sh" -w32 0x42020814 0x00000000 -w32 0x42020800 0xF3FF7FFF ;;
esac
