/*****************************************************************************
*
* AVRPROG compatible boot-loader
* Version  : 0.85 (Dec. 2008)
* Compiler : avr-gcc 4.1.2 / avr-libc 1.4.6
* size     : depends on features and startup ( minmal features < 512 words)
* by       : Martin Thomas, Kaiserslautern, Germany
*            eversmith@heizung-thomas.de
*            Additional code and improvements contributed by:
*           - Uwe Bonnes
*           - Bjoern Riemer
*           - Olaf Rempel
*
* License  : Copyright (c) 2006-2008 M. Thomas, U. Bonnes, O. Rempel
*            Free to use. You have to mention the copyright
*            owners in source-code and documentation of derived
*            work. No warranty! (Yes, you can insert the BSD
*            license here)
*
* Tested with ATmega8, ATmega16, ATmega162, ATmega32, ATmega324P,
*             ATmega644, ATmega644P, ATmega128, AT90CAN128
*
* - Initial versions have been based on the Butterfly bootloader-code
*   by Atmel Corporation (Authors: BBrandal, PKastnes, ARodland, LHM)
*
****************************************************************************
*
*  See the makefile and readme.txt for information on how to adapt 
*  the linker-settings to the selected Boot Size (BOOTSIZE=xxxx) and 
*  the MCU-type. Other configurations futher down in this file.
*
*  With BOOT_SIMPLE, minimal features and discarded int-vectors
*  this bootloader has should fit into a a 512 word (1024, 0x400 bytes) 
*  bootloader-section. 
*
****************************************************************************/
//BOOTSIZE=1024

/* FREQENCY */
#define F_CPU 14745600

/* UART BAUD RATE 19200 */
#define BAUDRATE 19200

/* ����� ������� �������� UART (��� U2C)*/
#define UART_DOUBLESPEED

#define DEVTYPE     DEVTYPE_BOOT

/*
 * ����� ����� ��� ������ ����� � ���������
 * ����� ����� � ��������� ���� ����� ��� ������� ��� ������ �������� ��� �� �����
 */
#define	IVERT_BLPIN	0
#define BLPORT		PORTD
#define BLDDR		DDRD
#define BLPIN		PIND
#define BLPNUM		PIND5

/*
 * ����� ����� ��� ���������� ������ ����������
 * ��������� ����� - �� � ����������
 */

#define ENABLE_BOOT_LED
#define BIPORT		PORTC
#define BIDDR		DDRC
#define BIPIN		PINC
#define BIPNUM		PC1


#define ENABLE_BOOT_BUZ
#define DDR_BUZ		DDRD
#define BUZ			PD6
#define PORT_BUZ	PORTD


#define ENABLE_BOOT_RS485
#define DDR_RS485	DDRC
#define RE_DE		PC1
#define PORT_RS485	PORTC


#define DISABLE_WDT_AT_STARTUP

/*
 * Watchdog-reset is issued at exit 
 * define the timeout-value here (see avr-libc manual)
 */
#define EXIT_WDT_TIME   WDTO_250MS

/*
 * ����� ������ ����������
 * SIMPLE-Mode - ��������� �������� ����� ������ ��� ������
 *   ������� � �������� ��������� �������������� ����� ������ 
 *   (������ ������ ���� ������) ���� �� ������� �� �������������
 *   ��� ���� ������ ����� �� ������ ��������������� ��� ����-� ��������,
 * WAIT-mode Bootloader ������� ������� �� ����, ���� �� �� ���� � ������� ���������� �������
 *   (������� �������������) �� ���������� ������� � �������� ���������.
 */
#define START_SIMPLE
//#define START_WAIT

/* ������� ��� ����� � ��������� � START_WAIT */
#define START_WAIT_UARTCHAR 'S'

/* �������� ��� START_WAIT mode ( t = WAIT_TIME * 10ms ) */
#define WAIT_VALUE 500 /* ������: 500*10ms = 3000ms = 5sec !!!������ 5���. �� �����!!!*/

/*
 * enable/disable readout of fuse and lock-bits
 * (AVRPROG has to detect the AVR correctly by device-code
 * to show the correct information).
 */
//#define ENABLEREADFUSELOCK

/* enable/disable write of lock-bits
 * WARNING: lock-bits can not be reseted by bootloader (as far as I know)
 * Only protection no unprotection, "chip erase" from bootloader only
 * clears the flash but does no real "chip erase" (this is not possible
 * with a bootloader as far as I know)
 * Keep this undefined!
 */
//#define WRITELOCKBITS

/*
 * define the following if the bootloader should not output
 * itself at flash read (will fake an empty boot-section)
 */
//#define READ_PROTECT_BOOTLOADER 

#define VERSION_HIGH '0'
#define VERSION_LOW  '8'

#define GET_LOCK_BITS           0x0001
#define GET_LOW_FUSE_BITS       0x0000
#define GET_HIGH_FUSE_BITS      0x0003
#define GET_EXTENDED_FUSE_BITS  0x0002

/* ������ �������� ������� ��� USART*/
#ifdef UART_DOUBLESPEED

	#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)((F_CPU) + ((uint32_t)baudRate * 4UL)) / ((uint32_t)(baudRate) * 8UL) - 1)

#else

	#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)((F_CPU) + ((uint32_t)baudRate * 8UL)) / ((uint32_t)(baudRate) * 16UL) - 1)

#endif


#include <stdint.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "chipdef.h"

uint8_t gBuffer[SPM_PAGESIZE];

#if defined(BOOTLOADERHASNOVECTORS)
	#warning "This Bootloader does not link interrupt vectors - see makefile"
	/* make the linker happy - it wants to see __vector_default */
	// void __vector_default(void) { ; }
	void __vector_default(void) { ; }
#endif

static void sendchar(uint8_t data)
{

#ifdef ENABLE_BOOT_RS485
	PORT_RS485|=(1<<RE_DE);
#endif

	while (!(UART_STATUS & (1<<UART_TXREADY))) asm volatile ("wdr");
	UART_DATA = data;
	_delay_ms(1);

#ifdef ENABLE_BOOT_RS485
	PORT_RS485&=~(1<<RE_DE);
#endif

}

static uint8_t recvchar(void)
{
	while (!(UART_STATUS & (1<<UART_RXREADY))) asm volatile ("wdr");
	return UART_DATA;
}

static inline void eraseFlash(void)
{
	// erase only main section (bootloader protection)
	uint32_t addr = 0;
	while (APP_END > addr) 
		{
		boot_page_erase(addr);		// Perform page erase
		boot_spm_busy_wait();		// Wait until the memory is erased.
		addr += SPM_PAGESIZE;
		asm volatile ("wdr");
		}
	boot_rww_enable();
}

static inline void recvBuffer(pagebuf_t size)
{
	pagebuf_t cnt;
	uint8_t *tmp = gBuffer;

	for (cnt = 0; cnt < sizeof(gBuffer); cnt++) 
		{
		*tmp++ = (cnt < size) ? recvchar() : 0xFF;
		}
}

static inline uint16_t writeFlashPage(uint16_t waddr, pagebuf_t size)
{
	uint32_t pagestart = (uint32_t)waddr<<1;
	uint32_t baddr = pagestart;
	uint16_t data;
	uint8_t *tmp = gBuffer;

	do 
		{
		data = *tmp++;
		data |= *tmp++ << 8;
		boot_page_fill(baddr, data);	// call asm routine.

		baddr += 2;			// Select next word in memory
		size -= 2;			// Reduce number of bytes to write by two
		} 
	while (size);				// Loop until all bytes written

	boot_page_write(pagestart);
	boot_spm_busy_wait();
	boot_rww_enable();		// Re-enable the RWW section

	return baddr>>1;
}

static inline uint16_t writeEEpromPage(uint16_t address, pagebuf_t size)
{
	uint8_t *tmp = gBuffer;

	do 
		{
		eeprom_write_byte( (uint8_t*)address, *tmp++ );
		address++;			// Select next byte
		size--;				// Decreas number of bytes to write
		}
	while (size);				// Loop until all bytes written

	// eeprom_busy_wait();

	return address;
}

static inline uint16_t readFlashPage(uint16_t waddr, pagebuf_t size)
{
	uint32_t baddr = (uint32_t)waddr<<1;
	uint16_t data;

	do 
	{

#ifndef READ_PROTECT_BOOTLOADER
#warning "Bootloader not read-protected"

	#if defined(RAMPZ)
		data = pgm_read_word_far(baddr);
	#else
		data = pgm_read_word_near(baddr);
	#endif

#else
		// don't read bootloader
		if ( baddr < APP_END ) 
		{
		#if defined(RAMPZ)
			data = pgm_read_word_far(baddr);
		#else
			data = pgm_read_word_near(baddr);
		#endif
		}
		else 
		{
		data = 0xFFFF; // fake empty
		}
#endif
		sendchar(data);			// send LSB
		sendchar((data >> 8));		// send MSB
		baddr += 2;			// Select next word in memory
		size -= 2;			// Subtract two bytes from number of bytes to read
	} 
	while (size);				// Repeat until block has been read
	return baddr>>1;
}

static inline uint16_t readEEpromPage(uint16_t address, pagebuf_t size)
{
	do 
	{
	asm volatile ("wdr");
	sendchar( eeprom_read_byte( (uint8_t*)address ) );
	address++;
	size--;				// Decrease number of bytes to read
	} 
	while (size);				// Repeat until block has been read

	return address;
}

#if defined(ENABLEREADFUSELOCK)
static uint8_t read_fuse_lock(uint16_t addr)
{
	uint8_t mode = (1<<BLBSET) | (1<<SPMEN);
	uint8_t retval;

	asm volatile
	(
		"movw r30, %3\n\t"		/* Z to addr */ \
		"sts %0, %2\n\t"		/* set mode in SPM_REG */ \
		"lpm\n\t"			/* load fuse/lock value into r0 */ \
		"mov %1,r0\n\t"			/* save return value */ \
		: "=m" (SPM_REG),
		  "=r" (retval)
		: "r" (mode),
		  "r" (addr)
		: "r30", "r31", "r0"
	);
	return retval;
}
#endif

static void send_boot(void)
{
	sendchar('A');
	sendchar('V');
	sendchar('R');
	sendchar('B');
	sendchar('O');
	sendchar('O');
	sendchar('T');
}

static void (*jump_to_app)(void) = 0x0000;

int main(void)
{
	DDRC|= (1<<PC3);

	uint16_t address = 0;
	uint8_t device = 0, val;

#ifdef ENABLE_BOOT_RS485
	DDR_RS485|= (1<<RE_DE);
	PORT_RS485&=~(1<<RE_DE);
#endif

#ifdef ENABLE_BOOT_LED	// LED ON
	BIPORT |= (1<<BIPNUM);	
	BIDDR  |= (1<<BIPNUM);
#endif

#ifdef ENABLE_BOOT_BUZ //Buzzer ON
		DDR_BUZ|=(1<<BUZ);
#endif

#ifdef DISABLE_WDT_AT_STARTUP
	#ifdef WDT_OFF_SPECIAL
		#warning "using target specific watchdog_off"
		bootloader_wdt_off();
	#else
		cli();
		wdt_reset();
		wdt_enable(WDTO_2S);
	#endif
#endif
	
#ifdef START_POWERSAVE
	uint8_t OK = 1;
#endif

	BLDDR  &= ~(1<<BLPNUM);		// set as Input
	BLPORT |= (1<<BLPNUM);		// Enable pullup

	// Set baud rate
	UART_BAUD_HIGH = (UART_CALC_BAUDRATE(BAUDRATE)>>8) & 0xFF;
	UART_BAUD_LOW = (UART_CALC_BAUDRATE(BAUDRATE) & 0xFF);

#ifdef UART_DOUBLESPEED
	UART_STATUS = ( 1<<UART_DOUBLE );
#endif

	UART_CTRL = UART_CTRL_DATA;
	UART_CTRL2 = UART_CTRL2_DATA;
	
#if defined(START_POWERSAVE)
	/*
		This is an adoption of the Butterfly Bootloader startup-sequence.
		It may look a little strange but separating the login-loop from
		the main parser-loop gives a lot a possibilities (timeout, sleep-modes
	    etc.).
	*/
	for(;OK;) 
	{

		asm volatile ("wdr");

	#if(IVERT_BLPIN)
		if (!(BLPIN & (1<<BLPNUM)))
	#else
		if ((BLPIN & (1<<BLPNUM)))
	#endif
		{
		asm volatile ("wdr");
		// jump to main app if pin is not grounded
		BLPORT &= ~(1<<BLPNUM);	// set to default

		#ifdef UART_DOUBLESPEED
			UART_STATUS &= ~( 1<<UART_DOUBLE );
		#endif


		#ifdef ENABLE_BOOT_LED	// LED OFF
		BIPORT &= ~(1<<BIPNUM);	
		BIDDR  &= ~(1<<BIPNUM);
		#endif

		#ifdef ENABLE_BOOT_BUZ
		DDR_BUZ&=~(1<<BUZ);
		#endif

		jump_to_app();		// Jump to application sector

		} 
		else 
		{
		val = recvchar();
		/* ESC */
			if (val == 0x1B) 
			{
				// AVRPROG connection
				// Wait for signon
				while (val != 'S')
				{
					val = recvchar();
					asm volatile ("wdr");
				}

				send_boot();			// Report signon
				OK = 0;
			} 
			else 
			{
			asm volatile ("wdr");
			sendchar('?');
			}
	    }
		// Power-Save code here
	}

#elif defined(START_SIMPLE)

#if(IVERT_BLPIN)
	if (!(BLPIN & (1<<BLPNUM)))
#else
	if ((BLPIN & (1<<BLPNUM)))
#endif
	{
		asm volatile ("wdr");
		// jump to main app if pin is not grounded
		BLPORT &= ~(1<<BLPNUM);		// set to default	
			
	#ifdef UART_DOUBLESPEED
		UART_STATUS &= ~( 1<<UART_DOUBLE );
	#endif

		#ifdef ENABLE_BOOT_LED	// LED OFF
		BIPORT &= ~(1<<BIPNUM);	
		BIDDR  &= ~(1<<BIPNUM);
		#endif

		#ifdef ENABLE_BOOT_BUZ
		DDR_BUZ&=~(1<<BUZ);
		#endif

		jump_to_app();			// Jump to application sector
	}

#elif defined(START_WAIT)

	uint16_t cnt = 0;

	while (1) {

		asm volatile ("wdr");

		if (UART_STATUS & (1<<UART_RXREADY))
			if (UART_DATA == START_WAIT_UARTCHAR)
				break;

		if (cnt++ >= WAIT_VALUE) {
			BLPORT &= ~(1<<BLPNUM);		// set to default


			#ifdef ENABLE_BOOT_LED	// LED OFF
			BIPORT &= ~(1<<BIPNUM);	
			BIDDR  &= ~(1<<BIPNUM);
			#endif

			#ifdef ENABLE_BOOT_BUZ
			DDR_BUZ&=~(1<<BUZ);
			#endif

			jump_to_app();			// Jump to application sector
		}

		_delay_ms(10);
	}
	send_boot();

#elif defined(START_BOOTICE)
#warning "BOOTICE mode - no startup-condition"

#else
#error "Select START_ condition for bootloader in main.c"
#endif

#ifdef ENABLE_BOOT_BUZ //Buzzer ON

		asm volatile ("wdr");
		_delay_ms(1000);
		asm volatile ("wdr");
		_delay_ms(1000);
		asm volatile ("wdr");
		Buzzer_Enter0();
		asm volatile ("wdr");

#endif

	while(1) 
	{
		asm volatile ("wdr");
		val = recvchar();
		// Autoincrement?
		if (val == 'a') 
		{
			sendchar('Y');			// Autoincrement is quicker

		//write address
		} 
		else if (val == 'A') 
		{
			address = recvchar();		//read address 8 MSB
			address = (address<<8) | recvchar();
			sendchar('\r');

		// Buffer load support
		} 
		else if (val == 'b') 
		{
			sendchar('Y');					// Report buffer load supported
			sendchar((sizeof(gBuffer) >> 8) & 0xFF);	// Report buffer size in bytes
			sendchar(sizeof(gBuffer) & 0xFF);

		// Start buffer load
		} 
		else if (val == 'B') 
		{
			pagebuf_t size;
			size = recvchar() << 8;				// Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Load memory type ('E' or 'F')
			recvBuffer(size);

			if (device == DEVTYPE) 
			{
				if (val == 'F') 
				{
				address = writeFlashPage(address, size);
				} 
				else if (val == 'E') 
				{
				address = writeEEpromPage(address, size);
				}
				sendchar('\r');
			} 
			else 
			{
			sendchar(0);
			}

		// Block read
		} 
		else if (val == 'g') 
		{
			pagebuf_t size;
			size = recvchar() << 8;				// Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Get memtype

			if (val == 'F') 
			{
			address = readFlashPage(address, size);
			} 
			else if (val == 'E') 
			{
			address = readEEpromPage(address, size);
			}

		// Chip erase
 		} 
		else if (val == 'e') 
		{
		if (device == DEVTYPE) 
			{
			eraseFlash();
			}
		sendchar('\r');

		// Exit upgrade
		} 
		else if (val == 'E') 
		{

		#ifdef ENABLE_BOOT_RS485
			PORT_RS485&=~(1<<RE_DE);
			DDR_RS485&=~(1<<RE_DE);
		#endif

		asm volatile ("wdr");
		#ifdef ENABLE_BOOT_BUZ //Buzzer ON
			Buzzer_End0();
		#endif
		asm volatile ("wdr");

		wdt_enable(EXIT_WDT_TIME); // Enable Watchdog Timer to give reset
		sendchar('\r');
		while(1);

		#ifdef WRITELOCKBITS
			#warning "Extension 'WriteLockBits' enabled"
			// TODO: does not work reliably
			// write lockbits
			} 
			else if (val == 'l') 
			{
			if (device == DEVTYPE) 
				{
				// write_lock_bits(recvchar());
				boot_lock_bits_set(recvchar());	// boot.h takes care of mask
				boot_spm_busy_wait();
				}
			sendchar('\r');
		#endif
		// Enter programming mode
		} 
		else if (val == 'P') 
		{
		sendchar('\r');

		// Leave programming mode
		} 
		else if (val == 'L') 
		{
		sendchar('\r');
		// return programmer type
		} 
		else if (val == 'p') 
		{
		sendchar('S');		// always serial programmer

		#ifdef ENABLEREADFUSELOCK
			#warning "Extension 'ReadFuseLock' enabled"
			// read "low" fuse bits
			} 
			else if (val == 'F') 
			{
			sendchar(read_fuse_lock(GET_LOW_FUSE_BITS));

			// read lock bits
			} 
			else if (val == 'r') 
			{
			sendchar(read_fuse_lock(GET_LOCK_BITS));

			// read high fuse bits
			} 
			else if (val == 'N') 
			{
			sendchar(read_fuse_lock(GET_HIGH_FUSE_BITS));
			// read extended fuse bits
			} 
			else if (val == 'Q') 
			{
			sendchar(read_fuse_lock(GET_EXTENDED_FUSE_BITS));
			#endif

		// Return device type
		} 
		else if (val == 't') 
		{
		sendchar(DEVTYPE);
		sendchar(0);
		// clear and set LED ignored
		} 
		else if ((val == 'x') || (val == 'y')) 
		{
		recvchar();
		sendchar('\r');

		// set device
		} 
		else if (val == 'T') 
		{
		device = recvchar();
		sendchar('\r');
		// Return software identifier
		} 
		else if (val == 'S') 
		{
		send_boot();

		// Return Software Version
		} 
		else if (val == 'V') {
		sendchar(VERSION_HIGH);
		sendchar(VERSION_LOW);

		// Return Signature Bytes (it seems that 
		// AVRProg expects the "Atmel-byte" 0x1E last
		// but shows it first in the dialog-window)
		} 
		else if (val == 's') 
		{
		sendchar(SIG_BYTE3);
		sendchar(SIG_BYTE2);
		sendchar(SIG_BYTE1);

		/* ESC */
		} 
		else if(val != 0x1b) 
		{
		sendchar('?');
		}

	
	} 

	

	return 0;
}

void Buzzer_End0(void)
{

		PORT_BUZ|=(1<<BUZ);
		_delay_ms(300);
		PORT_BUZ&=~(1<<BUZ);
		_delay_ms(100);
		PORT_BUZ|=(1<<BUZ);
		_delay_ms(200);
		PORT_BUZ&=~(1<<BUZ);
		PORT_BUZ|=(1<<BUZ);
		_delay_ms(100);
		PORT_BUZ&=~(1<<BUZ);
		_delay_ms(300);
		PORT_BUZ&=~(1<<BUZ);
		_delay_ms(100);
		PORT_BUZ|=(1<<BUZ);
		_delay_ms(300);
		PORT_BUZ&=~(1<<BUZ);

		DDR_BUZ&=~(1<<BUZ); //Buzzer OFF.

}

void Buzzer_Enter0(void)
{

		//PORTC|=PC3;

		PORT_BUZ|=(1<<BUZ);
		_delay_ms(150);
		PORT_BUZ&=~(1<<BUZ);
		_delay_ms(450);
		PORT_BUZ|=(1<<BUZ);
		_delay_ms(150);
		PORT_BUZ&=~(1<<BUZ);
		_delay_ms(450);
		PORT_BUZ|=(1<<BUZ);
		_delay_ms(300);
		PORT_BUZ&=~(1<<BUZ);

	
}
