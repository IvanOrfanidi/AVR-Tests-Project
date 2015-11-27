

	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <util/delay.h>
	#include <util/crc16.h>

	// Standard Input/Output functions.
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>


	#define DELAY_PRESENCE 300
	#define RESET_PULSE 222
	#define PAUSE 255
	#define RESET 			for(uint8_t n=0; n<RESET_PULSE; n++) asm volatile ("nop");

	#define ONE_PULSE 14
	#define ONE_COMMAND 	for(uint8_t n=0; n<ONE_PULSE; n++) asm volatile ("nop");

	#define ZERO_PULSE 36
	#define ZERO_COMMAND	for(uint8_t n=0; n<ZERO_PULSE; n++) asm volatile ("nop");

	#define DELAY_ONE	10
	#define DELAY_ZERO	2
			
	#define ONE	2

	#define DALLAS_DDR DDRA
	#define DALLAS_PORT PORTA
	#define DALLAS_PIN PINA
	#define DALLAS (1<<PA5)

	#define MASTER_DALLAS 1
	#define MASTER_SLAVE 1


	uint8_t dallas_master(char *pData_Dallas, uint8_t command);	//(0 - Data OK, 1 - Data NO, 2 - Error CRC)




