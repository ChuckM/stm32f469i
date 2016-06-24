/*
 * sdram.c - SDRAM controller example
 *
 * Copyright (C) 2013-2016 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This version for the SDRAM on the STM32F469I-Disco board.
 * Notes:
 *	Clock
 *		The SDRAM clock (SDCLK) is derived from the system clock
 * 		(HCLK) which is typically equal to SysCLK.
 *		So it can either be HCLK/2 or HCLK/3
 *		This code uses retarget.c which configs a 168Mhz Sysclk
 *  	At 168Mhz, SD CLK can be either 84Mhz, or 54Mhz.
 *		This requires an OSPEED of 100Mhz (rather than 50Mhz)
 *	Chip
 *		The chip used on the disco board is a MT48LC4M32B2
 *		That is a 4MB chip organized by 1Mx32 in 4 banks.
 *		It is the "fast" one (167Mhz) so running it at 84Mhz is easy
 *		Its pipelined, which makes it a bit faster in random access.
 *		Bits A14 and A15 are connected to Bank Select
 *		0 - 1MB address is muxed out 4K (12 bit) Row (A0-A11) 
 * 									256 (8 bit)  Column (A0-A7)
 */

#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/cm3/scb.h>
#include "../util/util.h"

#define SDRAM_BASE_ADDRESS ((uint8_t *)(0xC0000000))

void sdram_init(void);

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
 * This is just syntactic sugar but it helps, all of these
 * GPIO pins get configured in exactly the same way.
 * 
 * I've updated it from my early SDRAM code so that it can
 * be dynamically sized. Sometimes you need GPIOB sometimes
 * not. Now it is self contained.
 *
 * All SDRAM GPIOS
 *   GPIOC,	0
 * 	* SDNWE		PC0
 * 
 *   GPIOD, 	0,1,8,9,10,14,15
 * 	* D2	PD0
 * 	* D3	PD1
 * 	* D13	PD8
 * 	* D14	PD9
 * 	* D15	PD10
 * 	* D0	PD14
 * 	* D1	PD15
 * 
 *   GPIOE,	0,1,7,8,9,10,11,12,13,14,15
 * 	* NBL0	PE0
 * 	* NBL1	PE1
 * 	* D4	PE7
 * 	* D5	PE8
 * 	* D6	PE9
 * 	* D7	PE10
 * 	* D8	PE11
 * 	* D9	PE12
 * 	* D10	PE13
 * 	* D11	PE14
 * 	* D12	PE15
 * 
 *   GPIOF,	0,1,2,3,4,5,11,12,13,14,15
 * 	* A0	PF0
 * 	* A1	PF1
 * 	* A2	PF2
 * 	* A3	PF3
 * 	* A4	PF4
 * 	* A5	PF5
 * 	* SDNRAS	PF11
 * 	* A6	PF12
 * 	* A7	PF13
 * 	* A8	PF14
 * 	* A9	PF15
 * 
 *   GPIOG,	0,1,4,5,8,15
 * 	* A10	PG0
 * 	* A11	PG1
 * 	* A14	PG4	
 * 	* A15	PG5
 * 	* SDCLK		PG8
 * 	* SDNCAS	PG15
 * 
 *   GPIOH,	2,3,8,9,10,11,12,13,14,15
 * 	* SDCKE0	PH2
 * 	* SDNE0		PH3
 * 	* D16	PH8
 * 	* D17	PH9
 * 	* D18	PH10
 * 	* D19	PH11
 * 	* D20	PH12
 * 	* D21	PH13
 * 	* D22	PH14
 * 	* D23	PH15
 * 
 *   GPIOI,	0,1,2,3,4,5,6,7,9,10
 * 	* D24	PI0
 * 	* D25	PI1
 * 	* D26	PI2
 * 	* D27	PI3
 * 	* NBL2	PI4
 * 	* NBL3	PI5
 * 	* D28	PI6
 * 	* D29	PI7
 * 	* D30	PI9
 * 	* D31	PI10
 */
static struct pin_defines {
	uint32_t	gpio;
	enum rcc_periph_clken	clk;
	uint16_t	pins;
} sdram_pins[] = {
	/* GPIOC,	0 */
	{GPIOC, RCC_GPIOC, GPIO0 },
	/* GPIOD, 	0,1,8,9,10,14,15 */
	{GPIOD, RCC_GPIOD, GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15},
	/* GPIOE,	0,1,7,8,9,10,11,12,13,14,15 */
	{GPIOE, RCC_GPIOE, GPIO0 | GPIO1 | GPIO7 | GPIO8 | GPIO9 | GPIO10 |
			GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15 },
	/* GPIOF,	0,1,2,3,4,5,11,12,13,14,15 */
	{GPIOF, RCC_GPIOF, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO11 |
			GPIO12 | GPIO13 | GPIO14 | GPIO15 },
	/* GPIOG,	0,1,4,5,8,15 */
	{GPIOG, RCC_GPIOG, GPIO0 | GPIO1 | GPIO4 | GPIO5 | GPIO8 | GPIO15},
	/* GPIOH,	2,3,8,9,10,11,12,13,14,15 */
	{GPIOH, RCC_GPIOH, GPIO2 | GPIO3 | GPIO8 | GPIO9 | GPIO10 | GPIO11 |
			GPIO12 | GPIO13 | GPIO14 | GPIO15 },
	/* GPIOI,	0,1,2,3,4,5,6,7,9,10 */
	{GPIOI, RCC_GPIOI, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 |
			GPIO6 | GPIO7 | GPIO9 | GPIO10},
	/* Indicates the end of the list */
	{0x00,(enum rcc_periph_clken) 0 , 0x00}
};

/* same parameters as ST used for this DRAM chip */
static struct sdram_timing timing = {
	.trcd = 6,		/* RCD Delay */
	.trp = 2,		/* RP Delay */
	.twr = 2,		/* Write Recovery Time */
	.trc = 6,		/* Row Cycle Delay */
	.tras = 4,		/* Self Refresh Time */
	.txsr = 6,		/* Exit Self Refresh Time */
	.tmrd = 2,		/* Load to Active Delay */
};

/*
 * Initialize the SD RAM controller.
 */
void
sdram_init(void)
{
	struct pin_defines *pd = &sdram_pins[0];
	uint32_t cr_tmp, tr_tmp; /* control, timing registers */

	/*
	* First all the GPIO pins that end up as SDRAM pins
	*/
	while (pd->gpio != 0) {
		/* enable the GPIO's clock */
		rcc_periph_clock_enable(pd->clk);
		/* Set them to alternate function */
		gpio_mode_setup(pd->gpio, GPIO_MODE_AF, GPIO_PUPD_NONE, pd->pins);
		/* Output speed 100Mhz */
		gpio_set_output_options(pd->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, pd->pins);
		/* And Alternate function #12 */
		gpio_set_af(pd->gpio, GPIO_AF12, pd->pins);
		pd++;
	}

	/* Enable the SDRAM Controller */
	rcc_periph_clock_enable(RCC_FMC);

	/* Note the STM32F469I-DISCO board has the ram attached to bank 1 */
	/* Timing parameters computed for a 168Mhz clock */
	/* These parameters are specific to the SDRAM chip on the board */

	cr_tmp  = FMC_SDCR_RPIPE_NONE;
	cr_tmp |= FMC_SDCR_RBURST;
	cr_tmp |= FMC_SDCR_SDCLK_2HCLK;
	cr_tmp |= FMC_SDCR_CAS_3CYC;
	cr_tmp |= FMC_SDCR_NB4;
	cr_tmp |= FMC_SDCR_MWID_32b;
	cr_tmp |= FMC_SDCR_NR_12;		/* 12 rows x 8 columns = 1MB x 4 banks = 4MB */
	cr_tmp |= FMC_SDCR_NC_8;

	/* We're programming BANK 1 */
#ifdef SHOW_CR_CONTENTS
	printf("SDRAM Control register: 0x%x\n", (int) cr_tmp);
	printf("\tRBURST = 0x%d\n", (int) (cr_tmp >> 12) & 0x1);
	printf("\tSDCLK = 0x%d\n", (int) (cr_tmp >> 10) & 0x3);
	printf("\tWP = 0x%d\n", (int) (cr_tmp >> 9) & 0x1);
	printf("\tCAS = 0x%d\n", (int) (cr_tmp >> 7) & 0x3);
	printf("\tNB = 0x%d\n", (int) (cr_tmp >> 6) & 1);
	printf("\tMWID = 0x%d\n", (int) (cr_tmp >> 4) & 3);
	printf("\tNR = 0x%d\n", (int) (cr_tmp >> 2) & 3);
	printf("\tNC = 0x%d\n", (int) cr_tmp & 3);
#endif

	FMC_SDCR1 = cr_tmp;

	tr_tmp = sdram_timing(&timing);
	FMC_SDTR1 = tr_tmp;


	/* Now start up the Controller per the manual
	 *	- Clock config enable
	 *	- PALL state
	 *	- set auto refresh
	 *	- Load the Mode Register
	 */
	sdram_command(SDRAM_BANK1, SDRAM_CLK_CONF, 1, 0);
	msleep(1); /* sleep at least 100uS */
	sdram_command(SDRAM_BANK1, SDRAM_PALL, 1, 0);
	sdram_command(SDRAM_BANK1, SDRAM_AUTO_REFRESH, 8, 0);
	tr_tmp = SDRAM_MODE_BURST_LENGTH_1				|
				SDRAM_MODE_BURST_TYPE_SEQUENTIAL	|
				SDRAM_MODE_CAS_LATENCY_3		|
				SDRAM_MODE_OPERATING_MODE_STANDARD	|
				SDRAM_MODE_WRITEBURST_MODE_SINGLE;
	sdram_command(SDRAM_BANK1, SDRAM_LOAD_MODE, 1, tr_tmp);

	/*
	 * Refresh rate, 64ms / 4096 = 15.62uS
	 * SD Clock is 84Mhz (168/2) so 15.62uS * 84x10e6 = 1312.5 "clocks"
	 * Subtract 20 clocks so it will catch up if its held off by CPU access
 	 * to the same memory 1312 - 20 = 1292 clocks. 
	 */
	FMC_SDRTR = 1292 << 1;
	/* et Voila' DRAM memory at 0xC0000000 */
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
	printf("%08x | ", (unsigned int) tmp);
	for (i = 0; i < 16; i++) {
		printf("%02x ", (uint8_t) *(line_addr + i));
		if (i == 7) {
			printf("  ");
		}
	}
	printf("| ");
	for (i = 0; i < 16; i++) {
		b = *line_addr++;
		printf("%c", (((b > 126) || (b < 32)) ? '.' : (char) b));
	}
	printf("\n");
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

	printf("Initializing SDRAM controller ... \n");
	sdram_init();
	printf("Done.\nOriginal data:\n");
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
