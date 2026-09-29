# CEG 3536 — Laboratoire 1 — construction en ligne de commande (facultatif)
# Le projet se construit normalement dans STM32CubeIDE ; ce Makefile permet
# de vérifier la compilation et de programmer la carte sans l'IDE.
#   make          construire build/CEG3536_Lab1.elf et .bin
#   make flash    construire et programmer la carte (ST-LINK, SWD)
#   make clean
CLT   ?= C:/ST/STM32CubeCLT_1.21.0
CC     = $(CLT)/GNU-tools-for-STM32/bin/arm-none-eabi-gcc
OBJCPY = $(CLT)/GNU-tools-for-STM32/bin/arm-none-eabi-objcopy
SIZE   = $(CLT)/GNU-tools-for-STM32/bin/arm-none-eabi-size
PROG   = $(CLT)/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe

TARGET  = CEG3536_Lab1
BUILD   = build
SRCDIR  = Core/Src
STARTUP = Core/Startup/startup_stm32l552zetxq.s
LDSCRIPT = STM32L552ZETXQ_FLASH.ld

# Mêmes options que STM32CubeIDE (projet Debug) : -O0 -g3, nano.specs, nosys.specs.
# syscalls.c et sysmem.c sont les fichiers de support générés par CubeIDE.
CPU     = -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
ASFLAGS = $(CPU) -x assembler-with-cpp -g3 -DDEBUG -Wall
CFLAGS  = $(CPU) -std=gnu11 -O0 -g3 -DDEBUG -DSTM32L552xx -Wall -ICore/Inc \
          -ffunction-sections -fdata-sections -fstack-usage
LDFLAGS = $(CPU) -T $(LDSCRIPT) -Wl,--gc-sections -static \
          -Wl,-Map=$(BUILD)/$(TARGET).map --specs=nano.specs --specs=nosys.specs \
          -Wl,--start-group -lc -lm -Wl,--end-group

SRCS  = $(wildcard $(SRCDIR)/*.s)
CSRCS = $(wildcard $(SRCDIR)/*.c)
OBJS  = $(patsubst $(SRCDIR)/%.s,$(BUILD)/%.o,$(SRCS)) \
        $(patsubst $(SRCDIR)/%.c,$(BUILD)/%.o,$(CSRCS)) $(BUILD)/startup.o

all: $(BUILD)/$(TARGET).bin

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRCDIR)/%.s $(SRCDIR)/registres.inc | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRCDIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCPY) -O binary $< $@

flash: $(BUILD)/$(TARGET).elf
	"$(PROG)" -c port=SWD mode=UR -w $< -v -rst

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean
