# AVR-GCC Makefile
PROJECT=minivlcd
SOURCES=main.c uart.c console.c lib.c appdb.c commands.c lcd.c timer.c backlight.c buttons.c adc.c relay.c tui.c cron.c pcd8544.c tui-lib.c
DEPS=Makefile buttons.h main.h cron.h uart.h pcd8544.h tui-lib.h
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atxmega32e5
#AVRBINDIR=~/avr-tools/bin/
AVRDUDEMCU=x32e5
AVRDUDECMD=sudo avrdude -p $(AVRDUDEMCU) -c avrispmkII -P usb
DFLAGS=
CFLAGS=-mmcu=$(MMCU) -Os -g -Wall -W -pipe -mcall-prologues -std=gnu99 -Wno-main $(DFLAGS)

all: $(PROJECT).hex

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) timer-ll.o
	$(AVRBINDIR)$(CC) $(CFLAGS) -flto -fwhole-program -flto-partition=none -mrelax -I./ -o $(PROJECT).out  $(SOURCES) timer-ll.o -lc -lm

timer-ll.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS) -I./ -c -o timer-ll.o timer-ll.c

#adc-ll.o: adc-ll.c adc.c main.h
#	$(AVRBINDIR)$(CC) $(CFLAGS) -I./ -c -o adc-ll.o adc-ll.c

asm: $(SOURCES)
	$(AVRBINDIR)$(CC) -S $(CFLAGS) -I./ -o $(PROJECT).S $(SOURCES)

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT).out | less

program: $(PROJECT).hex
	cp $(PROJECT).hex /tmp && cd /tmp && $(AVRBINDIR)$(AVRDUDECMD) -U flash:w:$(PROJECT).hex


size: $(PROJECT).out
	$(AVRBINDIR)avr-size $(PROJECT).out

clean:
	-rm -f *.o
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).S

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r
