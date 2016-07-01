/*
 * sdram.c - SDRAM Initialization code
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
 *
 * This provides initialization for the on board SDRAM
 */

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/cm3/scb.h>
#include "../util/util.h"

#ifndef NULL
#define NULL	(void *)(0)
#endif

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
	rcc_periph_clock_enable(RCC_FSMC);

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
