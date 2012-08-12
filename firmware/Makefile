AVR_MCU := atmega328p
AVR_MCU_SPEED := 16000000UL
AVRDUDE_OPTIONS := -q -P usb -c avrispv2 -p $(AVR_MCU) -y

VERSION := \"$(shell git describe --dirty)\"

CC       := avr-gcc
LD       := avr-gcc
ELF_SIZE := avr-size
OBJCOPY  := avr-objcopy
CFLAGS   := -mmcu=$(AVR_MCU) -DF_CPU=$(AVR_MCU_SPEED) -Wall -W -Werror -Os -std=c99
LDFLAGS  := -mmcu=$(AVR_MCU) -Wl

all: fvcontroller.hex

fvcontroller.elf: fvcontroller.o serial.o hardware.o lcd.o registers.o
	$(CC) $(LDFLAGS) -o $@ $^

flash: fvcontroller.hex
	avrdude $(AVRDUDE_OPTIONS) -U flash:w:$<

# The efuse only controls brownout detection, and doesn't read back
# correctly because it has undefined bits in it.
fuses:
	avrdude $(AVRDUDE_OPTIONS) -U lfuse:w:0xc7:m -U hfuse:w:0xd1:m

registers.o: *.c *.h Makefile
	$(CC) $(CFLAGS) -DVERSION=$(VERSION) -o registers.o -c registers.c

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

%.hex: %.elf
	$(OBJCOPY) $^ -j .text -j .data -O ihex $@
	$(ELF_SIZE) $^

clean:
	rm -f *.o *.elf *.hex *~

.PHONY: all clean flash fuses
