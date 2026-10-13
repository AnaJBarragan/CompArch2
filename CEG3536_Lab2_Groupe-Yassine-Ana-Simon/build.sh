#!/bin/bash
# build.sh — construction sans make (Git Bash + STM32CubeCLT), équivalent du Makefile.
#   ./build.sh          construit build/CEG3536_Lab2.elf et .bin
#   ./build.sh flash    construit puis programme la carte (ST-LINK, SWD)
set -e
CLT="${CLT:-C:/ST/STM32CubeCLT_1.21.0}"
CC="$CLT/GNU-tools-for-STM32/bin/arm-none-eabi-gcc"
OBJCPY="$CLT/GNU-tools-for-STM32/bin/arm-none-eabi-objcopy"
SIZE="$CLT/GNU-tools-for-STM32/bin/arm-none-eabi-size"
PROG="$CLT/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
TARGET=CEG3536_Lab2
BUILD=build
CPU="-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard"
ASFLAGS="$CPU -x assembler-with-cpp -g3 -DDEBUG -Wall"
CFLAGS="$CPU -std=gnu11 -O0 -g3 -DDEBUG -DSTM32L552xx -Wall -ICore/Inc -ffunction-sections -fdata-sections"
cd "$(dirname "$0")"
mkdir -p "$BUILD"
OBJS=()
for f in Core/Src/*.s Core/Startup/*.s; do
    o="$BUILD/$(basename "${f%.s}").o"
    "$CC" $ASFLAGS -ICore/Src -c "$f" -o "$o"
    OBJS+=("$o")
done
for f in Core/Src/*.c; do
    o="$BUILD/$(basename "${f%.c}").o"
    "$CC" $CFLAGS -c "$f" -o "$o"
    OBJS+=("$o")
done
"$CC" "${OBJS[@]}" $CPU -T STM32L552ZETXQ_FLASH.ld -Wl,--gc-sections -static \
    -Wl,-Map="$BUILD/$TARGET.map" --specs=nano.specs --specs=nosys.specs \
    -Wl,--start-group -lc -lm -Wl,--end-group -o "$BUILD/$TARGET.elf"
"$SIZE" "$BUILD/$TARGET.elf"
"$OBJCPY" -O binary "$BUILD/$TARGET.elf" "$BUILD/$TARGET.bin"
if [ "$1" = "flash" ]; then
    "$PROG" -c port=SWD mode=UR -w "$BUILD/$TARGET.elf" -v -rst
fi
