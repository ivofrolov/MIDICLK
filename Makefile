ENV = debug

MCU = attiny85
F_CPU = 8000000L

LOCKBITS = 0xFF
EFUSE = 0xFF
HFUSE = 0xDF
ifneq ($(ENV), debug) # WARNING: reset disabled
	HFUSE = 0x5F
endif
# clock divide by 8 disabled
LFUSE = 0xE2

PROGRAMMER = usbasp

SRC = main.c
# DEFINES = 

TARGET = firmware
OBJDIR = build/$(ENV)
VPATH = src

ifeq ($(ENV), debug)
	DEFINES += -DDEBUG
endif
CFLAGS = -std=gnu11 -Os -Wall \
	-fuse-linker-plugin -flto -fno-fat-lto-objects -ffunction-sections -fdata-sections \
	-Wl,--gc-sections -Wl,--start-group -lm -Wl,--end-group \
	-mmcu=$(MCU) -DF_CPU=$(F_CPU) \
	$(DEFINES)

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZETOOL = avr-size
UPLOADER = avrdude

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/$(TARGET).elf: $(OBJDIR)/$(SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/$(TARGET).hex: $(OBJDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

.DEFAULT_GOAL = build
.PHONY: build
build: $(OBJDIR)/$(TARGET).elf

.PHONY: fuses
fuses:
	$(UPLOADER) -p $(MCU) -c $(PROGRAMMER) -Ulock:w:$(LOCKBITS):m -Uhfuse:w:$(HFUSE):m -Ulfuse:w:$(LFUSE):m -Uefuse:w:$(EFUSE):m

.PHONY: upload
upload: $(OBJDIR)/$(TARGET).hex
	$(UPLOADER) -p $(MCU) -c $(PROGRAMMER) -U flash:w:$<:i

.PHONY: size
size: $(OBJDIR)/$(TARGET).elf
	$(SIZETOOL) --mcu=$(MCU) -C $<
	$(SIZETOOL) --mcu=$(MCU) -A $<

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*
