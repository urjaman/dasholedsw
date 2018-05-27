# AVR-GCC Makefile
PROJECT=dasholedsw
SOURCES=main.c uart.c console.c lib.c appdb.c commands.c lcd.c timer.c backlight.c buttons.c adc.c relay.c tui.c cron.c ssd1331.c tui-lib.c tui-mod.c tui-modules.c saver.c pulse.c speedo.c ee.c eecounter.c odo.c tui-color.c tinygif/tdgif_lib.c
HEADERS=buttons.h main.h cron.h uart.h tui-lib.h ssd1331.h tui-mod.h saver.h pulse.h speedo.h ee.h eecounter.h odo.h tui-color.h
DEPS=Makefile $(HEADERS)
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atxmega32e5
#AVRBINDIR=~/avr-tools/bin/
AVRDUDEMCU=x32e5
AVRDUDECMD=avrdude -p $(AVRDUDEMCU) -c atmelice_pdi
DFLAGS=-DUSE_ALLOCA
CFLAGS=-mmcu=$(MMCU) -Os -g -Wall -W -pipe -mcall-prologues -std=gnu99 -Wno-main $(DFLAGS)

all: $(PROJECT).hex size

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) timer-ll.o logo.o
	./make_tuidb.sh tui-modules.c > tuidb_db.c
	$(AVRBINDIR)$(CC) $(CFLAGS) -flto -fwhole-program -flto-partition=none -mrelax -I./ -o $(PROJECT).out  $(SOURCES) timer-ll.o logo.o -lc -lm

timer-ll.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS) -I./ -c -o timer-ll.o timer-ll.c

logo.o: logo.bin
	$(AVRBINDIR)avr-objcopy -I binary -O elf32-avr -B avr:102 --rename-section .data=.text,contents,alloc,load,readonly,code logo.bin logo.o

logo.bin: logo.gif tinygif/convert
	./tinygif/convert logo.gif logo.bin

tinygif/convert:
	cd tinygif; make convert

asm: $(SOURCES)
	$(AVRBINDIR)$(CC) -S $(CFLAGS) -I./ -o $(PROJECT).S $(SOURCES)

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT).out | less

program: $(PROJECT).hex
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:w:$(PROJECT).hex


size: $(PROJECT).out
	$(AVRBINDIR)avr-size $(PROJECT).out

clean:
	-rm -f *.o
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).S
	-rm -f logo.bin

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r

astyle:
	astyle -A8 -t8 -xC110 -S -z2 -o -O $(SOURCES) $(HEADERS)
