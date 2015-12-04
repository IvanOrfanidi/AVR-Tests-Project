



//Functions Device.
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <util/delay.h>
	#include <avr/eeprom.h>

// Standard Input/Output functions.
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <math.h>

	#include "USART.h"

	#define BUT1 		(1<<PC1)
	#define PIN_BUT 	PINC

	#define PAUSE		500

	#define DEVICE_ID	0x32	//For Device UDPS.

	#define LED_GO_ON	0x31
	#define LED_GO_OFF	0x32


const uint8_t array_crc8[]={
 0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
 157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
 35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
 190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
 70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
 219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
 101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
 248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
 140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
 17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
 175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
 50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
 202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
 87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
 233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
 116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
 };


void init()
{

	// Port B initialization //

	DDRB=0x00;

	PORTB=0xFF;

	///////////////////////////


	// Port D initialization //

	DDRD=0x00;

	PORTD=0x00;

	///////////////////////////

	// Port C initialization //

	DDRC=0x00;

	PORTC= BUT1;

	///////////////////////////

	//Watchdog initialization.

	asm volatile ("wdr");	//Watchdog Reset.
	
	/////////////////////////////////////////////////////////////////////

}


int main()
{
	
	init();

	usart0_init();
	
	uint8_t slave_id=~(PINB);

	char tx_buffer[4];

	tx_buffer[0]=slave_id;
	tx_buffer[1]=DEVICE_ID;

	sei();

	usart0_write(0x30);
	_delay_ms(1);
	usart0_write(0x30);
	_delay_ms(1);
	usart0_write(0x30);
	
	uint8_t but_true=0;

	while(1)
	{

		if(!(PIN_BUT & (BUT1)))
		{
			if(!(but_true))
			{
				but_true=1;

				tx_buffer[2]=LED_GO_ON;
				tx_buffer[3]=_crc8_update(tx_buffer, (sizeof(tx_buffer)-1));

				for(uint8_t i=0; i<sizeof(tx_buffer); i++)
					usart0_write(tx_buffer[i]);
				
				_delay_ms(PAUSE);

			}

			usart0_clear_tx_buffer();
		}

		else

		{
			but_true=0;
		}


		usart0_clear_tx_buffer();

	}

}


int _crc8_update(char *pTxBuffer, int Len)
{
	uint8_t crc8=0;
	
	for (uint8_t i=0; i<Len; i++)
	{
		crc8=crc8 ^ pTxBuffer[i];
		crc8=array_crc8[crc8];
	}

return crc8;			
}
