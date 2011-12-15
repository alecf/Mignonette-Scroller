#
# Makefile for Mignonette programs
#
# created: mar 18, 2008
#
# last udpated: dec 12, 2011
#
#
# revision history:
#
# - dec 12, 2011 - alecf
#       copy/fork for my scroller
#
# - feb 21, 2010 - rolf
#		minor changes for Jegge's tri2s game.
#
# - may 24, 2008 - rolf
#		cleanup.
#
# - may 18, 2008 - rolf
#		rename mig-testmunch.c to munch.c.
#		add "programonly" target.
#		fix "burn-fuse" target so it works correctly for Atmel's avrispmkII programmer.
#
# - may 13, 2008 - rolf
#		try to merge "munch" game and miggl (version of 5/3) with mitch's audio code (version of 5/2/08)
#
#
#

PRG            = tri2s
OBJ            = tri2s.o miggl.o

PRGWORKING     = tri2s.hex-v0.06

MCU_TARGET     = atmega88
OPTIMIZE       = -O2

DEFS           =
LIBS           =

# set to one of the following:
# 	"usbtiny" for the ladyada usbtiny programmer OR
#	"avrispmkII" for the atmel AVR ISP MKII programmer
##AVRDUDE_PROGRAMMER = avrispmkII 
AVRDUDE_PROGRAMMER = usbtiny 


# You should not have to change anything below here. (except dependencies!)

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

##all: $(PRG).elf lst text eeprom
all: $(PRG).elf lst text

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# dependencies (optional)
##uart.o: uart.h
miggl.o: miggl.h miggl-private.h

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak 
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@


#
# Rules for programming via avrdude
#

AVRDUDE_PORT = usb
AVRDUDE_FLAGS = -p $(MCU_TARGET) -c $(AVRDUDE_PROGRAMMER) -P $(AVRDUDE_PORT)

program: $(PRG).hex
	avrdude $(AVRDUDE_FLAGS) -e -U flash:w:$(PRG).hex

programonly:
	avrdude $(AVRDUDE_FLAGS) -e -U flash:w:$(PRGWORKING)


# to burn fuses with avrispmkII, don't do "-B 250" as it messes up the programmer.

burn-fuse:
	avrdude $(AVRDUDE_FLAGS) -V -U hfuse:w:0xdf:m -U lfuse:w:0xe2:m 
#	avrdude $(AVRDUDE_FLAGS) -B 250 -u -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m

read-fuse:
	avrdude $(AVRDUDE_FLAGS) -u -U lfuse:r:l.txt:r
	avrdude $(AVRDUDE_FLAGS) -u -U hfuse:r:h.txt:r
	avrdude $(AVRDUDE_FLAGS) -u -U efuse:r:e.txt:r


# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV                 = fig2dev
EXTRA_CLEAN_FILES       = *.hex *.bin *.srec

dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@

