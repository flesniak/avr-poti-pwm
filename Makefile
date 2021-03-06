SRC = main.c
GENERATED_HDR = table.h

MCU = attiny13a
CFLAGS = -Os -mrelax -fshort-enums -fdata-sections -ffunction-sections -mtiny-stack -Wl,--gc-sections -Wall -Wextra -Wstrict-prototypes -std=c99 -mmcu=$(MCU)
AVRDUDE_MCU = t13
AVRDUDE_PROG = usbasp

HFUSE = 0xff
LFUSE = 0x7a

BIN = $(SRC:%.c=%)
HEX = $(BIN).hex

.PHONY: all flash fuses clean

all: $(HEX)

$(GENERATED_HDR): generate-lut.py
	./$< >$@

$(BIN): $(SRC) $(GENERATED_HDR)
	avr-gcc $(CFLAGS) -o $(BIN) $<

$(HEX): $(BIN)
	avr-objcopy -S -O ihex $< $@

flash: $(HEX)
	avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROG) -eU flash:w:$<

fuses:
	avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROG) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m

clean:
	rm -f $(BIN) $(HEX) $(GENERATED_HDR)
