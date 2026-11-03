#!/bin/bash
S="$(dirname "$0")"
bash "$S/swd.sh" "$@" -r32 0x20000034 40 -r32 0x42020014 4 -r32 0x42020814 4 -r32 0x42020414 4 | awk '
/^0x20000034/ {printf "estop_flag=%s etat=%s touch_en=%s cpt_trans=%s ", $3,$4,$5,$6}
/^0x20000044/ {printf "sens=%s clig_cpt=%s clig_ph=%s touch_sig=%s ", $3,$4,$5,$6}
/^0x20000054/ {printf "blink_cycles=%s (%d) | ", $3, strtonum("0x" $3)}
/^0x42020014/ {printf "ODR A=%s ", $3}
/^0x42020814/ {printf "C=%s ", $3}
/^0x42020414/ {printf "B=%s\n", $3}
/rror/ {print}'
