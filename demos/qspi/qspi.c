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
 *
 * From the Data sheet for the flash, fast read needs
 * 7 dummy cycles at 86Mhz, 8 at 95Mhz. Since we're at
 * 168Mhz/2 = 84Mhz we're using 7. But if we went to 180/2
 * we would set it to 8.
 */
void
qspi_read_sector(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t ccr;
	uint32_t tmp;
	int i, bcnt;

	printf("Read Sector: Initial Status is\n");
	print_status(QUADSPI_SR);

	/* wait for port to be non busy */
	while (QUADSPI_SR & QUADSPI_SR_BUSY) ;
	QUADSPI_CR &= ~QUADSPI_CR_SSHIFT; /* turn off sample shift */

	QUADSPI_DLR = len - 1;
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IREAD);
	ccr |= QUADSPI_SET(CCR, DCYC, 9);
	ccr |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	ccr |= QUADSPI_SET(CCR, ABSIZE, 0);
	ccr |= QUADSPI_SET(CCR, INST, 0x03);	/* Fast Quad read */
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
	while ((QUADSPI_SR & QUADSPI_SR_TCF) == 0) {
		/* flen = QUADSPI_GET(SR, FLEVEL, QUADSPI_SR); */
/*		printf("%d-", flen);  */
		tmp = QUADSPI_DR;
		for (i = 0; i < 4; i++) {
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

#define FLASH_SECTOR_ERASE	0xd8
#define FLASH_WRITE_ENABLE	0x06
#define FLASH_WRITE_DISABLE	0x04
#define FLASH_READ_STATUS	0x05
#define FLASH_WRITE_PAGE	0x02

void qspi_enable(uint8_t cmd);
void qspi_enable(uint8_t cmd) {
	uint32_t ccr, sr;
	
	switch (cmd) {
		case FLASH_WRITE_ENABLE:
			printf("Enable Flash for write.\n");
			break;
		case FLASH_WRITE_DISABLE:
			printf("Disable writing to the flash.\n");
			break;
		default:
			printf("Unrecognized command 0X%x\n", (unsigned int) cmd);
			return;
	}
	ccr = QUADSPI_SET(CCR, INST, cmd);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IWRITE);
	QUADSPI_CCR = ccr;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	if (sr & QUADSPI_SR_TEF) {
		printf("Done (Error)\n");
		QUADSPI_FCR = sr; /* reset the flags */
	} else {
		printf("Done.\n");
	}
}

uint8_t read_flash_status(void);
/*
 * Read the Flash chip status register.
 */
uint8_t
read_flash_status(void)
{
	uint32_t ccr, sr;
	uint32_t	res = 0;

	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IREAD);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_READ_STATUS);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_DLR = 0; /* one byte to be read */
	QUADSPI_CCR = ccr;
	do {
		sr = QUADSPI_SR;
		if (QUADSPI_GET(SR, FLEVEL, sr)) {
			res = QUADSPI_DR;
		}
	} while (sr & QUADSPI_SR_BUSY);
	return (res & 0xff);
}


void qspi_sector_erase(uint32_t addr);

void 
qspi_sector_erase(uint32_t addr)
{
	uint32_t ccr, sr;
	uint8_t	status;

	printf("Erase Sector: %d\n", (int) addr / 4096);
	qspi_enable(FLASH_WRITE_ENABLE); /* set the write latch */
	status = read_flash_status();
	printf("Status after WRITE ENABLE: 0x%x\n", (unsigned int) status);
	ccr  = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IWRITE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_SECTOR_ERASE);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_CCR = ccr;
	QUADSPI_AR = addr;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	status = read_flash_status();
	printf("Status after SECTOR ERASE: 0x%x\n", (unsigned int) status);
	do {
		status = read_flash_status();
	} while (status & 1); /* write in progress */
	printf("Done.\n");
}
void qspi_write_page(uint32_t addr, uint8_t *buf, int len);

/*
 * Write a sector.
 *
 * The Quad SPI peripheral starts when you write the
 * address register. 
 *
 * From the Data sheet for the flash, fast read needs
 * 7 dummy cycles at 86Mhz, 8 at 95Mhz. Since we're at
 * 168Mhz/2 = 84Mhz we're using 7. But if we went to 180/2
 * we would set it to 8.
 */
void
qspi_write_page(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t ccr, sr;
	uint32_t tmp;
	int i, status;

	printf("Write Sector\n");
	qspi_enable(FLASH_WRITE_ENABLE);

	QUADSPI_DLR = len - 1;
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_MODE_IWRITE);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_WRITE_PAGE);	/* write 256 bytes */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	printf("CCR value = 0x%08X\n", (unsigned int) ccr);
	printf("CR value = 0x%08X\n", (unsigned int) QUADSPI_CR);
	QUADSPI_DLR = 255;
	QUADSPI_CCR = ccr; /* go write a page */
	QUADSPI_AR = addr;
	printf("After writing CCR status:\n");
	print_status(QUADSPI_SR);
	for (i = 0; i < 256; i+= 4) {
		tmp = *(buf + i) & 0xff;
		tmp |= (*(buf + 1 + i) & 0xff) << 8;
		tmp |= (*(buf + 2 + i) & 0xff) << 16;
		tmp |= (*(buf + 3 + i) & 0xff) << 24;
		QUADSPI_DR = tmp;
	}
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	status = read_flash_status();
	printf("Status after WRITE PAGE: 0x%x\n", (unsigned int) status);
	do {
		status = read_flash_status();
	} while (status & 1); /* write in progress */
	printf("Done.\n");
}

/* one 4K sector from the FLASH chip */
uint8_t sector_data[4096];
uint8_t test_sector_data[4096];
char *test_phrase = "The Quick brown fox jumped over 1,234,567,890 foxes. ";

int
main(void)
{
	uint8_t id_string[80];
	char *t, *b;
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
	/* fill the test sector with some data */
	for (i = 0; i < 10; i++) {
		t = (char *) &sector_data[i * 53];
		b = test_phrase;
		while (*b != 0) {
			*t++ = *b++;
		}
	}
	printf("Test sector data\n");
	hex_dump(0x90000000U, sector_data, 256);
	qspi_read_sector(0, &test_sector_data[0], 4096);
	printf("Test sector before writing\n");
	hex_dump(0x90000000U, test_sector_data, 256);
	printf("Erasing sector 0\n");
	qspi_sector_erase(0);
	printf("Writing test data \n");
	qspi_write_page(0, &sector_data[0], 256);
#if 0
	qspi_read_sector(0, &test_sector_data[0], 4096);
	printf("Read back test data\n");
	hex_dump(0x90000000U, test_sector_data, 256);
#endif
	while (1) {
		toggle_led(RED_LED);
		msleep(500);
	}
}
