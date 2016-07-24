/*
 * qspi.c - Quad SPI example code
 *
 * Copyright (c) 2016, Chuck McManis (cmcmanis@mcmanis.com)
 *
 */
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/quadspi.h>
#include <libopencm3/stm32/dma.h>
#include <gfx.h>
#include "../util/util.h"

void qspi_read_id(uint8_t *res);

#define TEST_ADDR	0x1100

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
	QUADSPI_CR = QUADSPI_SET(CR, PRESCALE, 1) | QUADSPI_SET(CR, FTHRES, 3) | QUADSPI_CR_SSHIFT;
	QUADSPI_DCR = QUADSPI_SET(DCR, FSIZE, 23) | QUADSPI_SET(DCR, CSHT, 1) | QUADSPI_DCR_CKMODE;
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
	printf("%s QUADSPI_SR: ", console_color(CYAN));
	printf("[%s%d%s] ", console_color(YELLOW),
		(unsigned int) QUADSPI_GET(SR, FLEVEL, s), console_color(CYAN));
	printf("%s ", (s & QUADSPI_SR_BUSY) ? "BSY" : "---");
	printf("%s ", (s & QUADSPI_SR_TOF) ? "TOF" : "---");
	printf("%s ", (s & QUADSPI_SR_SMF) ? "SMF" : "---");
	printf("%s ", (s & QUADSPI_SR_FTF) ? "FTF" : "---");
	printf("%s ", (s & QUADSPI_SR_TCF) ? "TCF" : "---");
	printf("%s ", (s & QUADSPI_SR_TEF) ? "TEF" : "---");
	printf("%s ", console_color(NONE));
	fflush(stdout);
}

int qspi_read_data(uint8_t *buf, int max_len);

/* read data from the QSPI into the buffer and count the
 * number of bytes read. Return that value.
 *	buf is a pointer to a uint8_t array
 *	len is the size of that array (avoid overrunning it)
 */
int
qspi_read_data(uint8_t *buf, int max_len)
{
	uint32_t sr;
	int	len;

	len = 0;
	/* manually transfer data from the QSPI peripheral, this 
	 * loop runs while QUADSPI_SR_BUSY is set. It pulls 1 byte
	 * at a time.
	 */
	do {
		sr = QUADSPI_SR;
		if (sr & QUADSPI_SR_FTF) {
			*buf = QUADSPI_BYTE_DR;
			buf++; len++;
			if (len >= max_len) {
				break;
			}
		}
	} while (sr & QUADSPI_SR_BUSY); 
	return (len);
}

/*
 * The flash has a command READ_ID (0x9e)
 */
void
qspi_read_id(uint8_t *res)
{
	uint32_t	tmp;
	int			len;
	
	/* Configure automatic polling mode to wait for memory ready */

	QUADSPI_DLR = 19;	/* use len-1 so 20 bytes is (20-1 == 19) */

	/* Commands are sent through CCR */
	tmp = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IREAD); /* indirect read */
	tmp |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	tmp |= QUADSPI_SET(CCR, ABSIZE, 0);
	tmp |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_NONE);
	tmp |= QUADSPI_SET(CCR, INST, 0x9e);
	tmp |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	tmp |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_CCR = tmp;
	len = qspi_read_data(res, 80);
	print_status(QUADSPI_SR);
	printf("\nRead ID returned %d bytes\n", len);
	QUADSPI_FCR = 0x1f;
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
	int bcnt;

	printf("Read Sector: Initial Status is : ");
	print_status(QUADSPI_SR);
	printf("\n");

	QUADSPI_DLR = len - 1;
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IREAD);
	ccr |= QUADSPI_SET(CCR, DCYC, 10);
	ccr |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	ccr |= QUADSPI_SET(CCR, ABSIZE, 0);
	ccr |= QUADSPI_SET(CCR, INST, 0xeb);	/* Fast Quad read */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_4LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_4LINE);
	QUADSPI_CCR = ccr; /* go get a sector */
	QUADSPI_AR = addr;
	printf("After writing CCR status: ");
	print_status(QUADSPI_SR);
	printf("\n");
	bcnt = qspi_read_data(buf, len);
	printf("After fetching %d bytes: ", bcnt);
	print_status(QUADSPI_SR);
	printf("\n");
	QUADSPI_FCR = 0x1f;
}

#define FLASH_SECTOR_ERASE	0xd8
#define FLASH_SUB_SECTOR_ERASE	0x20
#define FLASH_WRITE_ENABLE	0x06
#define FLASH_WRITE_DISABLE	0x04
#define FLASH_READ_STATUS	0x05
#define FLASH_WRITE_PAGE	0x02
#define FLASH_RESET_ENABLE	0x66
#define FLASH_RESET_MEMORY	0x99

void qspi_enable(uint8_t cmd);
void
qspi_enable(uint8_t cmd)
{
	uint32_t ccr, sr;
	char *op;
	
	switch (cmd) {
		case FLASH_WRITE_ENABLE:
			op = "Write Enable";
			break;
		case FLASH_WRITE_DISABLE:
			op = "Write Disable";
			break;
		case FLASH_RESET_MEMORY:
			op = "Reset FLASH";
			break;
		case FLASH_RESET_ENABLE:
			op = "Enable RESET";
			break;
		default:
			printf("Unrecognized command 0X%x\n", (unsigned int) cmd);
			op = "Unknown";
			return;
	}
	ccr = QUADSPI_SET(CCR, INST, cmd);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	QUADSPI_CCR = ccr;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	printf("QSPI Enable (%s) Status%s ", op,
		(sr & QUADSPI_SR_TEF)? " [Error] :" : ":");
	print_status(sr);
	printf("\n");
	QUADSPI_FCR = 0x1f; /* reset the flags */
}

enum flash_reg {
	STATUS_REG, VOLATILE_REG, NONVOLATILE_REG, 
	ENHANCED_VOLATILE_REG, FLAG_REG, LOCK_REG };

uint16_t read_flash_register(enum flash_reg r);
/*
 * Read the Flash chip status register.
 */
uint16_t
read_flash_register(enum flash_reg r)
{
	uint32_t ccr, sr;
	uint32_t	res = 0;

	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IREAD);
	switch(r) {
		case LOCK_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0xe8);
			break;
		case ENHANCED_VOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x65);
			break;
		case VOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x85);
			break;
		case NONVOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0xb5);
			break;
		case FLAG_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x70);
			break;
		case STATUS_REG:
		default:
			ccr |= QUADSPI_SET(CCR, INST, 0x05);
			break;
	}
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	if (r == NONVOLATILE_REG) {
		QUADSPI_DLR = 1;	/* 16 bits */
	} else {
		QUADSPI_DLR = 0;	/* 8 bits */
	}
	QUADSPI_CCR = ccr;
	do {
		res = QUADSPI_DR;
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	QUADSPI_FCR = 0x1f;
	return (res & 0xffff);
}
void write_flash_register(enum flash_reg r, uint16_t value);
/*
 * Write a register on the flash chip
 */
void
write_flash_register(enum flash_reg r, uint16_t value)
{
	uint32_t ccr;
	uint32_t status;

	qspi_enable(FLASH_WRITE_ENABLE);
	status = read_flash_register(STATUS_REG);
	if ((status & 0x2) == 0) {
		printf("write_flash_register: %sError%s - Unable to set write latch!\n", 
			console_color(YELLOW), console_color(NONE));
	}
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	switch(r) {
		default:
		case STATUS_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x01);
			break;
		case LOCK_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0xe5);
			break;
		case FLAG_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x50);
			break;
		case NONVOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0xb1);
			break;
		case VOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x81);
			break;
		case ENHANCED_VOLATILE_REG:
			ccr |= QUADSPI_SET(CCR, INST, 0x61);
			break;
	}
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_1LINE);
	if (r == NONVOLATILE_REG) {
		QUADSPI_DLR = 1;	/* 16 bits */
	} else {
		QUADSPI_DLR = 0;	/* 8 bits */
	}
	QUADSPI_CCR = ccr;
	QUADSPI_DR = value;
	while (QUADSPI_SR & QUADSPI_SR_BUSY);
	QUADSPI_FCR = 0x1f;
}

char * chip_status(uint8_t status);
/*
 * print out the operational status of the FLASH chip
 * from its status byte. (ignores the protection bits)
 */
char *
chip_status(uint8_t status)
{

	switch( status & 0x3 ) {
		case 0:
			return ("Idle");
			break;
		case 1:
			return ("Write in Progress");
			break;
		case 2:
			return ("Write Latch Enabled");
			break;
		case 3:
			return ("Write Latch Enabled, Write in Progress");
			break;
	}
	return "idle";
}

void qspi_sector_erase(uint32_t addr);

/*
 * Erase a sub-sector of the flash chip. There are 4096
 * 4096 byte sub sectors (16MB total).
 */
void 
qspi_sector_erase(uint32_t addr)
{
	uint32_t ccr, sr;
	uint8_t	status;

	printf("Erase Sub-Sector: %d\n", (int) (addr >> 12));
	qspi_enable(FLASH_WRITE_ENABLE); /* set the write latch */
	printf("QUADSPI Status after WRITE ENABLE: "); print_status(QUADSPI_SR);
	status = read_flash_register(STATUS_REG);
	printf("\nFLASH chip status after WRITE ENABLE: (0x%x) %s\n", (unsigned int) status,
		chip_status(status));

	ccr  = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_SUB_SECTOR_ERASE);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_CCR = ccr;
	QUADSPI_AR = addr;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	status = read_flash_register(STATUS_REG);
	printf("Status after SECTOR ERASE: 0x%x, %s\n", (unsigned int) status, chip_status(status));
	do {
		status = read_flash_register(STATUS_REG);
	} while (status & 1); /* write in progress */
	QUADSPI_FCR = 0x1f;
	printf("Sub-sector Erase complete\n");
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
	int tmp;
	int status;

	printf("Write Sector\n");
	printf("  Initial QSPI Status : ");
	print_status(QUADSPI_SR);
	printf("\n");
	qspi_enable(FLASH_WRITE_ENABLE);
	status = read_flash_register(STATUS_REG);
	printf("Write enable status: %s%s%s\n", 
		console_color(YELLOW), chip_status(status), console_color(NONE));
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	/* adjusting this to 0 fixed the write issue. */
	ccr |= QUADSPI_SET(CCR, DCYC, 0); 
	ccr |= QUADSPI_SET(CCR, INST, 0x32);	/* write 256 bytes */
	/* For some reason 1-1-4 command */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_4LINE);
	QUADSPI_DLR = 255;
	QUADSPI_AR = addr;
	QUADSPI_CCR = ccr; /* go write a page */
	tmp = 0;
	do {
		sr = QUADSPI_SR;
		if (sr & QUADSPI_SR_TCF) {
			break;
		}
		tmp++;
		QUADSPI_BYTE_DR = *buf++;
	} while (QUADSPI_SR & QUADSPI_SR_BUSY);

	status = read_flash_register(STATUS_REG);
	printf("Status after WRITE PAGE: 0x%x\n", (unsigned int) status);
	printf("Wrote %d bytes, SPI status ", (int) tmp);
	print_status(QUADSPI_SR);
	printf("\n");
	do {
		status = read_flash_register(STATUS_REG);
	} while (status & 1); /* write in progress */
	if (tmp != len) {
		fprintf(stderr, "Warning: wrote %d bytes, expected to write %d\n", tmp, len);
	}
	printf("  ... QSPI Status at the end : "); print_status(QUADSPI_SR);
	printf("\n");
	QUADSPI_FCR = 0x1f;
}

void print_config(uint16_t reg, char *);
void print_vol_config(uint16_t reg, char *);
void
print_vol_config(uint16_t reg, char *label)
{
	printf("%s Config:\n", label);
	printf("   Dummy Clock Cycles : %d\n", (reg >> 4) & 0xf);
	printf("   XIP : %s\n", (reg & 0x8) ? "Disabled [Default]" : "Enabled");
	printf("   Wrap : ");
	switch ((int)(reg & 0x3)) {
		case 0:
			printf("16 byte boundary aligned\n");
			break;
		case 1:
			printf("32 byte boundary aligned\n");
			break;
		case 2:
			printf("64 byte boundary aligned\n");
			break;
		case 3:
			printf("sequential\n");
			break;
	}
}

void
print_config(uint16_t reg, char *nv)
{
	printf("%s Config:\n", nv);
	printf("   Dummy Clock Cycles : %d\n", (int)((reg >> 12) & 0xf));
	printf("   XIP Mode: ");
	switch ((int)((reg >> 9) & 0x7)) {
		case 0:
				printf("Fast Read\n");
				break;
		case 1:
				printf("Dual Output Fast Read\n");
				break;
		case 2:
				printf("Dual I/O Fast Read\n");
				break;
		case 3:
				printf("Quad Output Fast Read\n");
				break;
		case 4:
				printf("Quad I/O Fast Read\n");
				break;
		case 7:
				printf("Disabled [default]\n");
				break;
		default:
				printf("Reserved\n");
				break;
	}
	printf("   Output Driver Strength: ");
	switch ((int)((reg >> 6) & 0x7)) {
		case 0:
		case 4:
				printf("Reserved\n");
				break;
		case 1:
				printf("90 Ohms\n");
				break;
		case 2:
				printf("60 Ohms\n");
				break;
		case 3:
				printf("45 Ohms\n");
				break;
		case 5:
				printf("20 Ohms\n");
				break;
		case 6:
				printf("15 Ohms\n");
				break;
		case 7:
				printf("30 Ohms [default]\n");
				break;
	}
	printf("   Reset/Hold : %s\n", (reg & 0x10) ? "Enabled [Default]" : "Disabled");
	printf("   Quad I/O protocol : %s\n", (reg & 0x8) ? "Disabled [Default]" : "Enabled");
	printf("   Dual I/O protocol : %s\n", (reg & 0x4) ? "Disabled [Default]" : "Enabled");
	
}

void print_flags(uint16_t f);
void
print_flags(uint16_t flags)
{
	printf("Flag status (0X%04X):\n", flags);
	printf("   Program/Erase Controller: %s\n",
		(flags & 0x80) ? "Ready" : "Busy");
	printf("   Erase Suspend: %s\n",
		(flags & 0x40) ? "In Effect" : "Not Active");
	printf("   Erase : %s\n",
		(flags & 0x20) ? "FAILURE" : "Clear");
	printf("   Program: %s\n",
		(flags & 0x10) ? "FAILURE" : "Clear");
	printf("   Vpp: %s\n",
		(flags & 0x8) ? "Disabled[Default]" : "Enabled");
	printf("   Program Suspend: %s\n",
		(flags & 0x4) ? "In Effect" : "Not Active");
	printf("   Protection: %s\n",
		(flags & 0x2) ? "FAILURE" : "Clear");
}

/* one 256 byte snapshots from the FLASH chip */
uint8_t sector_data[256];
uint8_t test_sector_data[256];

#define SWIDTH 16
#define SHEIGHT 16 

void draw_pixel(int, int, uint16_t);

void
draw_pixel(int x, int y, uint16_t color)
{
	sector_data[y*SWIDTH + x] = color & 0xff;
}

static const char HEX_CHARS[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F'
};

int
main(void)
{
	uint32_t addr;
	uint8_t id_string[80];
	uint16_t reg;
	unsigned int i;
	uint8_t	status;

	/* This code fills the test sector data with
     * some information. In this case PG 00 which
	 * is easy to see it is correct (or not) in
	 * the memory dump. 
	 */
	gfx_init(draw_pixel, SWIDTH, SHEIGHT, GFX_FONT_SMALL);
	gfx_fillScreen((uint16_t) ' ');
	gfx_setTextColor((uint16_t) '*', (uint16_t) ' ');
	gfx_setCursor(0, 7);
	gfx_puts((unsigned char *)"PG");
	gfx_setCursor(0, 15);
	/* pull out page number from TEST_ADDR value as hex digits */
	id_string[0] = HEX_CHARS[((TEST_ADDR >> 12 ) & 0xf)];
	id_string[1] = HEX_CHARS[((TEST_ADDR >> 8 ) & 0xf)];
	id_string[2] = 0;
	gfx_puts((unsigned char *) &id_string[0]);

	fprintf(stderr, "QUAD SPI Example/Demo code\n");
	printf("Test address for this run is : %s0x%06x%s\n",
		console_color(YELLOW), (unsigned int) TEST_ADDR, console_color(NONE));
	printf("Initializing QUAD SPI port ... ");
	fflush(stdout);
	qspi_init();
	printf("Done\n");
	for (i = 0; i < sizeof(id_string); i++) {
		id_string[i] = 0;
	}
	qspi_enable(FLASH_RESET_ENABLE);
	qspi_enable(FLASH_RESET_MEMORY);
	reg = read_flash_register(FLAG_REG);
	print_flags(reg);
	do {
		status = read_flash_register(STATUS_REG);
	} while (status & 1); /* write in progress */

	/* Lets see if we can read out the memory ID attached to the QSPI Port */
	printf("Reading Flash ID ... ");
	fflush(stdout);
	qspi_read_id(&id_string[0]);
	printf("Done\n");
	printf("ID Contents : ");
	printf("  Manufacturer: 0x%02x\n", (int) id_string[0]);
	printf("  Memory type, Capacity: 0x%02x, 0x%02x\n", 
			(int) id_string[1], (int) id_string[2]);
	printf("  Serial #: ");
	for (i = 6; i < 20; i++) {
		printf("%02X", id_string[i]);
	}
	printf("\n");
	/* read the NV Config register */
	reg = read_flash_register(NONVOLATILE_REG);
	print_config(reg, "Non-Volatile");
	reg = read_flash_register(VOLATILE_REG);
	printf("Volatile Register: 0X%02X\n", (int) reg);
	print_vol_config(reg, "Volatile");
	printf("Updating volatile register with 10 dummy cycles\n");
	write_flash_register(VOLATILE_REG, 0xab);
	reg = read_flash_register(VOLATILE_REG);
	print_vol_config(reg, "Volatile (updated)");


	printf("Press a key to continue\n");
	(void) console_getc(1);
	printf("Test sector data\n");
	hex_dump(TEST_ADDR, sector_data, 256);
	printf("Press a key to continue.\n");
	(void) console_getc(1);
	qspi_read_sector(TEST_ADDR, &test_sector_data[0], 256);
	printf("%sTest sector before writing%s\n", console_color(MAGENTA), console_color(NONE));
	hex_dump(TEST_ADDR, test_sector_data, 256);
	printf("Press a key to continue.\n");
	(void) console_getc(1);
	qspi_sector_erase(TEST_ADDR);
	qspi_read_sector(TEST_ADDR, &test_sector_data[0], 256);
	printf("%sTest sector after erasing%s\n", console_color(MAGENTA), console_color(NONE));
	hex_dump(TEST_ADDR, test_sector_data, 256);
	printf("Press a key to continue.\n");
	(void) console_getc(1);
	printf("Writing test data \n");
	qspi_write_page(TEST_ADDR, &sector_data[0], 256);
	qspi_read_sector(TEST_ADDR, &test_sector_data[0], 256);
	printf("%sTest sector after writing%s\n", console_color(MAGENTA), console_color(NONE));
	hex_dump(TEST_ADDR, test_sector_data, 256);
	printf("Press a key to continue.\n");
	(void) console_getc(1);
	addr = TEST_ADDR;
	while (1) {
		char c;
		c = console_getc(1);
		if ((c == '-') || (c == '_')) {
			addr = addr - 0x100;
			if (addr & 0x80000000) {
				addr = 0;
			}
		} else if ((c == '+') || (c == '=')) {
			addr = addr + 0x100;
			if (addr > 0xffff00) {
				addr = 0xffff00;
			}
		} else if (c == 'm') {
			gfx_fillScreen((uint16_t) ' ');
			gfx_setTextColor((uint16_t) '*', (uint16_t) ' ');
			gfx_setCursor(0, 7);
			gfx_puts((unsigned char *)"PG");
			gfx_setCursor(0, 15);
			/* pull out page number from TEST_ADDR value as hex digits */
			id_string[0] = HEX_CHARS[((addr >> 12 ) & 0xf)];
			id_string[1] = HEX_CHARS[((addr >> 8 ) & 0xf)];
			id_string[2] = 0;
			gfx_puts((unsigned char *) &id_string[0]);
			qspi_write_page(addr, &sector_data[0], 256);
		} else if (c == 'e') {
			qspi_sector_erase(addr);
		}
		qspi_read_sector(addr, &test_sector_data[0], 256);
		hex_dump(addr, test_sector_data, 256);
		toggle_led(RED_LED);
	}
}
