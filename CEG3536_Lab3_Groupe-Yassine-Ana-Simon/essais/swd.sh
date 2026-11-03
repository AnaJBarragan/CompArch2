#!/bin/bash
# swd.sh <cmd...> : exécute STM32_Programmer_CLI en HOTPLUG et n'affiche que les lignes de lecture
P="/c/ST/STM32CubeCLT_1.21.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
"$P" -c port=SWD mode=HOTPLUG "$@" 2>&1 | grep -E '^0x|Error|error' 
