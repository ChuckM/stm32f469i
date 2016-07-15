/*
 * qspi.c - Quad SPI example code
 *
 * Copyright (c) 2016, Chuck McManis (cmcmanis@mcmanis.com)
 *
 */
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/qspi.h>
#include <libopencm3/stm32/dma.h>
#include "../util/util.h"

void qspi_init(void);
void qspi_read_id(uint8_t *res);

#define QSPI_BASE_ADDRESS	((uint8_t *)(0x90000000))

/* one 4K sector from the FLASH chip */
uint8_t sector_data[4096];

/*
 * Quad SPI initialization
 *
 * On this board the following pins are used:
 *	GPIO	Alt Func	Pin Function
 *	 PF6	: AF9  :   QSPI_BK1_IO3
 *	 PF7	: AF9  :   QSPI_BK1_IO2
 *	 PF9	: AF10 :   QSPI_BK1_IO1
 *   PF8	: AF10 :   QSPI_BK1_IO0
 *  PF10	: AF9  :   QSPI_CLK
 *	 PB6	: AF10 :   QSPI_BK1_NCS
 *
 *	This is alt function 10
 *	ST uses a Micro N25Q128A13EF840F 128Mbit QSPI NoR Flash
 *	(so yes 16MB of flash space)
 */
void
qspi_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOB);
	/* PF6, PF7, and PF10 are Alt Function 9, PF8 and PF9 are Alt Function 10 */
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,
						GPIO6 | GPIO7 | GPIO8 | GPIO9 | GPIO10);
	gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
						GPIO6 | GPIO7 | GPIO8 | GPIO9 | GPIO10);
	gpio_set_af(GPIOF, GPIO_AF10, GPIO8 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF9, GPIO6 | GPIO7 | GPIO10);

	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO6);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO6);
	gpio_set_af(GPIOB, GPIO_AF10, GPIO6);

	/* enable the RCC clock for the Quad SPI port */
	rcc_periph_clock_enable(RCC_QUADSPI);
	

	/* quadspi_setup(prescale, shift, fsel?
		 CR - Prescale, Sshift, FSEL, DFM
			Prescale (ClockPrescaler) = 1
			SSHIFT (SampleShifting) = 1 (half cycle)
			FSEL (FlashID) = 0 flash chip 1 selected
			DFM (DualFlash) = 0 (only 1 flash chip)
		DCR - Flash Size, Chipselect high time, Clock Mode
			FSIZE (FlashSize) =  24 (16 MB)
			CSHT (ChipSelectHighTime) = 1 (2 cycles)
			CKMODE (ClockMode) = 0 (clock mode 0)
	*/
	QUADSPI_CR = QUADSPI_SET(CR, PRESCALE, 1) | QUADSPI_CR_SSHIFT;
	QUADSPI_DCR = QUADSPI_SET(DCR, FSIZE, 24) | QUADSPI_SET(DCR, CSHT, 1) | QUADSPI_DCR_CKMODE;
	/* enable it qspi_enable() */
	QUADSPI_CR |= QUADSPI_CR_EN;
}

void print_status(uint32_t s);

/*
 * Simple helper function to translate the status bits
 * for easy understanding and debugging.
 */
void
print_status(uint32_t s)
{
	printf("QSPI Status: ");
	if (s & QUADSPI_SR_BUSY) {
		printf("busy, ");
	}
	if (s & QUADSPI_SR_TOF) {
		printf("timeout, ");
	}
	if (s & QUADSPI_SR_SMF) {
		printf("match, ");
	}
	if (s & QUADSPI_SR_FTF) {
		printf("FIFO threshold, ");
	}
	if (s & QUADSPI_SR_TCF) {
		printf("xfer complete, ");
	}
	if (s & QUADSPI_SR_TEF) {
		printf("xfer error, ");
	}
	printf("FIFO Level = %d\n", (unsigned int) QUADSPI_GET(SR, FLEVEL, s));
}

/*
 * The flash has a command READ_ID (0x9e)
 * which should return 0x20, 0xba, 0x18, <len>, <extended ID0>, <extended ID1>
 * (that is 6 bytes of data)
 */
void
qspi_read_id(uint8_t *res)
{
	uint32_t	old_status, tmp, len;
	int			i;
	
	/* Configure automatic polling mode to wait for memory ready */

#define QUADSPI_CCR_MODE_IREAD	1
#define QUADSPI_CCR_MODE_IWRITE	0
#define QUADSPI_CCR_MODE_APOLL	2
#define QUADSPI_CCR_MODE_MEM	3

	QUADSPI_DLR = 19;	/* use len-1 so 6 bytes is (6-1 == 5) */

	/* Commands are sent through CCR */
	tmp = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IREAD); /* indirect read */
	tmp |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	tmp |= QUADSPI_SET(CCR, ABSIZE, 0);
	tmp |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_NONE);
	tmp |= QUADSPI_SET(CCR, INST, 0x9e);
	tmp |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	tmp |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_CCR = tmp;
	/* external indication we're doing something :-) */
	off_led(GREEN_LED);
	on_led(BLUE_LED);
	old_status = QUADSPI_SR;
	print_status(old_status);
	while (QUADSPI_SR & QUADSPI_SR_BUSY) {
		/*
		 * This bit of code lets you watch how
		 * the status register changes over time.
		 * Basically busy stays set until you've
		 * drained all the bytes you asked for.
		 */
		tmp = QUADSPI_SR;
		if (tmp != old_status) {
			old_status = tmp;
			print_status(old_status);
		}
		len = QUADSPI_GET(SR, FLEVEL, tmp);
		/* pull bytes from QUADSPI_DR 4 at a time */
		while (len > 0) {
			tmp = QUADSPI_DR;
			printf("DATA: 0x%08x\n", (unsigned int) tmp);
			for (i = 0; i < 4; i++) {
				*res = (tmp & 0xff);
				res++; len--; tmp >>= 8;
				if (tmp == 0) {
					break;
				}
			}
		}
	}
	off_led(BLUE_LED);
	on_led(GREEN_LED);
	QUADSPI_FCR = QUADSPI_FCR_CTCF;
	/* all done green on, blue off */
}

void qspi_read_sector(uint32_t addr, uint8_t *buf, int len);

/*
 * Read a sector from the flash chip (4K bytes)
 *
 * The Quad SPI peripheral starts when you write the
 * address register. 
 */
void
qspi_read_sector(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t ccr;
	uint32_t tmp;
	int xfer, flen, bcnt;

	printf("Read Sector: Initial Status is\n");
	print_status(QUADSPI_SR);

	/* wait for port to be non busy */
	while (QUADSPI_SR & QUADSPI_SR_BUSY) ;

	QUADSPI_DLR = len - 1;
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IREAD);
	ccr |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	ccr |= QUADSPI_SET(CCR, ABSIZE, 0);
	ccr |= QUADSPI_SET(CCR, INST, 0x03);	/* simple read */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	printf("CCR value = 0x%08X\n", (unsigned int) ccr);
	printf("CR value = 0x%08X\n", (unsigned int) QUADSPI_CR);
	QUADSPI_CCR = ccr; /* go get a sector */
	QUADSPI_AR = addr;
	printf("After writing CCR status:\n");
	print_status(QUADSPI_SR);
	bcnt = 0;
	while (QUADSPI_SR & QUADSPI_SR_BUSY) {
		flen = QUADSPI_GET(SR, FLEVEL, QUADSPI_SR);
		printf("%d-", flen);
		xfer = (flen >= 4) ? 4 : flen;
		tmp = QUADSPI_DR;
		while (xfer--) {
			*buf = tmp & 0xff;
			bcnt++;
			tmp >>= 8;
			buf++;
		}
	}
	printf("\n");
	printf("After fetching %d bytes:\n", bcnt);
	print_status(QUADSPI_SR);
	QUADSPI_FCR = QUADSPI_FCR_CTCF;
}

int
main(void)
{
	uint8_t id_string[80];
	int i;

	fprintf(stderr, "QUAD SPI Example/Demo code\n");
	printf("Initializing QUAD SPI port\n");
	qspi_init();
	printf("Done\n");
	while (QUADSPI_GET(SR, FLEVEL, QUADSPI_SR) > 0) {
		(void) QUADSPI_DR;	/* drain the FIFO */
	}

	/* Lets see if we can read out the memory ID attached to the QSPI Port */
	printf("Reading Flash ID\n");
	qspi_read_id(&id_string[0]);
	printf("Done\n");

	printf("ID Contents : ");
	for (i = 0; i < 6; i++) {
		printf("0x%02X ", id_string[i]);
	}
	printf("\n");
	for (i = 0; i < 256; i++) {
		sector_data[i] = (uint8_t) i;
	}
	hex_dump(0x90000000U, sector_data, 256);
	qspi_read_sector(0, &sector_data[0], 4096);
	hex_dump(0x90000000U, sector_data, 256);

	while (1) {
		toggle_led(RED_LED);
		msleep(500);
	}
}
