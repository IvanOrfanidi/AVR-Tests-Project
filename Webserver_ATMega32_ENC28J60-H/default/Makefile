###############################################################################
# Makefile for the project Webserver
###############################################################################

## General Flags
PROJECT = Webserver
MCU = atmega32
TARGET = Webserver.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99                                            -DF_CPU=14745600UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=Webserver.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = enc28j60.o ip_arp_udp_tcp.o main.o USART.o ATCMD.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) Webserver.hex Webserver.eep Webserver.lss size

## Compile
enc28j60.o: ../enc28j60.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ip_arp_udp_tcp.o: ../ip_arp_udp_tcp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

USART.o: ../USART.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ATCMD.o: ../ATCMD.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) Webserver.elf dep/* Webserver.hex Webserver.eep Webserver.lss Webserver.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

