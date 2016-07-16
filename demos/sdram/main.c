/*
 * sdram.c - SDRAM controller example
 *
 * Copyright (C) 2013-2016 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This example was written to test using the initialization code
 * in ../util so ../util/sdram.o it also does the console output a bit
 * more colorfully.
 */

#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/cm3/scb.h>
#include "../util/util.h"

#ifndef NULL
#define NULL	(void *)(0)
#endif

void hard_fault_handler(void);

/*
 * Write a simple hard fault handler. Ideally you won't need it but in my case
 * while debugging it came in handy. 
 * If you hit it, you can find out which line of code you were on using addr2line
 * Example: (with TEST_FAULT defined)
 *    $ arm-none-eabi-addr2line -e sdram.elf 0x800070e
 *    <current directory>/sdram.c:374
 *
 * Which happens to be the code line just AFTER the one where the assignment is 
 * made to *addr in the main function.
 */
void
hard_fault_handler(void)
{
	unsigned int *tos;

	/* Get the stack pointer from 'handler' SP (aka MSP) */
	asm("	MRS	%0, MSP" 
			: "=r" (tos));
	tos = tos + 2; /* function prefix pushes R4, LR */

	printf("[Hard fault handler]\n");
	printf("\tR0 = 0x%x\n", (int)*(tos));
	printf("\tR1 = 0x%x\n", (int) *(tos + 1));
	printf("\tR2 = 0x%x\n", (int) *(tos + 2));
	printf("\tR3 = 0x%x\n", (int) *(tos + 3));
	printf("\tR12 = 0x%x\n", (int) *(tos + 4));
	printf("\tLR = 0x%x\n", (int) *(tos + 5));
	printf("\tPC = 0x%x\n", (int) *(tos + 6));
	printf("\txPSR = 0x%x\n", (int) *(tos + 7));
	printf("\tBFAR = 0x%08x\n", (*((volatile unsigned int *)(0xE000ED38))));
	printf("\tCFSR = 0x%08x\n", (*((volatile unsigned int *)(0xE000ED28))));
	printf("\tHFSR = 0x%08x\n", (*((volatile unsigned int *)(0xE000ED2C))));
	printf("\tDFSR = 0x%08x\n", (*((volatile unsigned int *)(0xE000ED30))));
	printf("\tAFSR = 0x%08x\n", (*((volatile unsigned int *)(0xE000ED3C))));
	printf("\tSCB_SHCSR = 0x%08x\n", (unsigned int) SCB_SHCSR);
	while (1) ;
}

/*
 * This code are some routines that implement a "classic"
 * hex dump style memory dump. So you can look at what is
 * in the RAM, alter it (in a couple of automated ways)
 */
uint8_t *dump_line(uint8_t *, uint8_t *);
uint8_t *dump_page(uint8_t *, uint8_t *);

/*
 * dump a 'line' (an address, 16 bytes, and then the
 * ASCII representation of those bytes) to the console.
 * Takes an address (and possiblye a 'base' parameter
 * so that you can offset the address) and sends 16
 * bytes out. Returns the address +16 so you can
 * just call it repeatedly and it will send the
 * next 16 bytes out.
 */
uint8_t *
dump_line(uint8_t *addr, uint8_t *base)
{
	uint8_t *line_addr;
	uint8_t b;
	uint32_t tmp;
	int i;

	line_addr = addr;
	tmp = (uint32_t)line_addr - (uint32_t) base;
	printf(console_color(WHITE));
	printf("%08X | ", (unsigned int) tmp);
	printf(console_color(GREEN));
	for (i = 0; i < 16; i++) {
		printf("%02X ", (uint8_t) *(line_addr + i));
		if (i == 7) {
			printf("  ");
		}
	}
	printf("%s| ", console_color(YELLOW));
	for (i = 0; i < 16; i++) {
		b = *line_addr++;
		printf("%c", (((b > 126) || (b < 32)) ? '.' : (char) b));
	}
	printf("%s\n", console_color(NONE));
	return line_addr;
}


/*
 * dump a 'page' like the function dump_line except this
 * does 16 lines for a total of 256 bytes. Back in the
 * day when you had a 24 x 80 terminal this fit nicely
 * on the screen with some other information.
 */
uint8_t *
dump_page(uint8_t *addr, uint8_t *base)
{
	int i;
	for (i = 0; i < 16; i++) {
		addr = dump_line(addr, base);
	}
	return addr;
}

/*
 * Let this be defined if you want to see the Hard Fault test
 * in action. A bit of code at the beginning here trys to write
 * to address 0x30000000 which has no memory associated with it.
 * so boom! You get a hard fault.
#define FAULT_TEST
*/

/*
 * This example initializes the SDRAM controller and dumps
 * it out to the console. You can do various things like
 * (FI) fill with increment, (F0) fill with 0, (FF) fill
 * with FF. NP (next page), PP (prev page), NL (next line),
 * (PL) previous line, and (?) for help.
 */
int
main(void)
{
	int i;
	uint8_t *addr;
	char	c;

	fprintf(stderr,"\nSDRAM Example.\n");
#ifdef FAULT_TEST
	printf("Testing our fault handlers \n");
	addr = (uint8_t *)(0x30000000); /* bad address */
	*addr = 0;
#endif

	printf("Original SDRAM data:\n");
	addr = SDRAM_BASE_ADDRESS; 
	(void) dump_page(addr, NULL);
	printf("Status register is 0x%x\n", (int) FMC_SDSR);
	printf("Now writing new values \n");
	addr = SDRAM_BASE_ADDRESS; 
	for (i = 0; i < 256; i++) {
		*(addr + i) = i;
	}
	printf("Modified data (with Fill Increment)\n");
	addr = SDRAM_BASE_ADDRESS;
	addr = dump_page(addr, NULL);
	while (1) {
		printf("CMD> ");
		fflush(stdout);
		switch (c = console_getc(1)) {
		case 'f':
		case 'F':
			printf("Fill ");
			fflush(stdout);
			switch (c = console_getc(1)) {
			case 'i':
			case 'I':
				printf("Increment\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = i;
				}
				dump_page(addr, NULL);
				break;
			case '0':
				printf("Zero\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = 0;
				}
				dump_page(addr, NULL);
				break;
			case 'f':
			case 'F':
				printf("Ones (0xff)\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = 0xff;
				}
				dump_page(addr, NULL);
				break;
			default:
				printf("Unrecognized Command, press ? for help\n");
			}
			break;
		case 'n':
		case 'N':
			printf("Next ");
			fflush(stdout);
			switch (c = console_getc(1)) {
			case 'p':
			case 'P':
				printf("Page\n");
				addr += 256;
				dump_page(addr, NULL);
				break;
			case 'l':
			case 'L':
				printf("Line\n");
				addr += 16;
				dump_line(addr, NULL);
				break;
			default:
				printf("Unrecognized Command, press ? for help\n");
			}
			break;
		case 'p':
		case 'P':
			printf("Previous ");
			fflush(stdout);
			switch (c = console_getc(1)) {
			case 'p':
			case 'P':
				printf("Page\n");
				addr -= 256;
				dump_page(addr, NULL);
				break;
			case 'l':
			case 'L':
				printf("Line\n");
				addr -= 16;
				dump_line(addr, NULL);
				break;
			default:
				printf("Unrecognized Command, press ? for help\n");
			}
			break;
		case '?':
		default:
			printf("Help\n");
			printf(" n p - dump next page\n");
			printf(" n l - dump next line\n");
			printf(" p p - dump previous page\n");
			printf(" p l - dump previous line\n");
			printf(" f 0 - fill current page with 0\n");
			printf(" f i - fill current page with 0 to 255\n");
			printf(" f f - fill current page with 0xff\n");
			printf(" ? - this message\n");
			break;
		}
	}
}
