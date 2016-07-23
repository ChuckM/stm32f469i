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

/* Locally useful definitions */
#define FLASH_SECTOR_ERASE	0xd8
#define FLASH_SUB_SECTOR_ERASE	0x20
#define FLASH_QUAD_READ		0xeb
#define FLASH_WRITE_ENABLE	0x06
#define FLASH_WRITE_DISABLE	0x04
#define FLASH_READ_STATUS	0x05
#define FLASH_WRITE_PAGE	0x02
#define FLASH_RESET_ENABLE	0x66
#define FLASH_RESET_MEMORY	0x99

enum flash_reg {
	STATUS_REG, VOLATILE_REG, NONVOLATILE_REG,
	ENHANCED_VOLATILE_REG, FLAG_REG, LOCK_REG };

/*
 * Simple API for flash access (indirect API)
		void qspi_init(void);
		int qspi_read_flash(uint32_t addr, uint8_t *buf, int len);
		int qspi_write_flash(uint32_t addr, uint8_t *buf, int len);
		void qspi_erase_block(uint32_t addr);
 */

/*
 * Internal utility functions
 */
static void qspi_enable(uint8_t cmd);
static uint16_t read_flash_register(enum flash_reg r);
static void write_flash_register(enum flash_reg r, uint16_t value);
static int qspi_read_data(uint8_t *buf, int max_len);
static void qspi_write_data(uint32_t addr, uint8_t *buf, int len);

/*
 * This function sends a command to the FLASH
 * chip. It is used primarily for write enabling the
 * FLASH for programming or updating registers.
 */
static void
qspi_enable(uint8_t cmd)
{
	uint32_t ccr, sr;
	
	ccr = QUADSPI_SET(CCR, INST, cmd);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	QUADSPI_CCR = ccr;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	QUADSPI_FCR = 0x1f; /* reset the flags */
}


/*
 * Read one of the Flash chip registers.
 *
 * Most commonly the status register.
 */
static uint16_t
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

/*
 * Write a register on the flash chip used to write
 * the volatile register.
 */
static void
write_flash_register(enum flash_reg r, uint16_t value)
{
	uint32_t ccr;

	qspi_enable(FLASH_WRITE_ENABLE);
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

/*
 * Data reader for the flash, this pulls bytes from
 * the data register a byte at a time while the BUSY
 * flag is set in the status register. It returns
 * the number of bytes read.
 *
 *	buf is a pointer to a uint8_t array
 *	len is the size of that array (avoid overrunning it)
 */
static int
qspi_read_data(uint8_t *buf, int max_len)
{
	uint32_t sr;
	int	len;

	len = 0;
	/* manually transfer data from the QSPI peripheral, this
	 * loop runs while QUADSPI_SR_BUSY is set. It pulls 4 bytes
	 * at a time, until the last call, and then it can pull 1, 2
 	 * or 3 bytes depending on what was left in the FIFO.
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
 * Data writer for the flash. It does not check to insure the
 * area it is writing is erased. So if you haven't checked
 * for that you may not write what you expect (it can only
 * change bits from 1 to 0.
 *
 * The other quirk is I've set the flash to write sequentially
 * but the limit is 256 bytes *ASSUMING* they don't cross a
 * page boundary (lower 8 bits of the address are 0). The api
 * interface (qspi_write_flash()) will automatically fragment
 * the write to insure it meets that requirement.
 */
static void
qspi_write_data(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t ccr, sr;
	int tmp;
	int status;

	qspi_enable(FLASH_WRITE_ENABLE);
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	/* adjusting this to 0 fixed the write issue. */
	ccr |= QUADSPI_SET(CCR, DCYC, 0);
	ccr |= QUADSPI_SET(CCR, INST, 0x32);	/* write 256 bytes */
	/* For some reason 1-1-4 command */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_4LINE);
	QUADSPI_DLR = len - 1;
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
	do {
		status = read_flash_register(STATUS_REG);
	} while (status & 1); /* write in progress */
	if (tmp != len) {
		fprintf(stderr, "Warning: wrote %d bytes, expected to write %d\n", tmp, len);
	}
	QUADSPI_FCR = 0x1f;
}

/* External API for the FLASH chip
 *	-- init, read, write, and block erase.
 *
 * The simplest API in terms of using the chip like a storage
 * device.
 *
 *  -- init, map, unmap, bulk_erase
 *
 *  Make FLASH available as memory (or not)
 */

/*
 * Quad SPI initialization
 *
 * Note this is APPLICABLE to the STM32F469I-Discovery board
 * if you're porting this code, this mapping of GPIO pins to
 * FLASH pins will no doubt be different.
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
	qspi_enable(FLASH_RESET_ENABLE);
	qspi_enable(FLASH_RESET_MEMORY);
	/* note this is only for the flash chip on the 469I board ! */
	write_flash_register(VOLATILE_REG, 0xAB);
}

/*
 * qspi_read_flash()
 *
 * Read data from the flash chip. This code reads 'len' bytes
 * from the flash starting at address 'addr'.
 *
 * If it gets an error (reads more or less than it expects) it
 * returns 1, otherwise it returns 0.
 */
int
qspi_read_flash(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t ccr;
	int bcnt;

	QUADSPI_DLR = len - 1;
	ccr = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IREAD);
	ccr |= QUADSPI_SET(CCR, DCYC, 10);
	ccr |= QUADSPI_SET(CCR, ABMODE, QUADSPI_CCR_MODE_NONE);
	ccr |= QUADSPI_SET(CCR, ABSIZE, 0);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_QUAD_READ);	/* Fast Quad read */
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_4LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);	/* 24 bit address */
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_4LINE);
	QUADSPI_CCR = ccr; /* go get a sector */
	QUADSPI_AR = addr;
	bcnt = qspi_read_data(buf, len);
	QUADSPI_FCR = 0x1f;
	return (bcnt != len);
}

/*
 * Erase a sub-sector of the flash chip. There are 4096
 * 4096 byte sub sectors (16MB total).
 */
void
qspi_erase_block(uint32_t block)
{
	uint32_t ccr, sr;
	uint8_t	status;

	qspi_enable(FLASH_WRITE_ENABLE); /* set the write latch */
	ccr  = QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_IWRITE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, INST, FLASH_SUB_SECTOR_ERASE);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	QUADSPI_CCR = ccr;
	QUADSPI_AR = (block << 12) & 0xffffff;
	do {
		sr = QUADSPI_SR;
	} while (sr & QUADSPI_SR_BUSY);
	do {
		status = read_flash_register(STATUS_REG);
	} while (status & 1); /* write in progress */
	QUADSPI_FCR = 0x1f;
}

/*
 * qspi_write_flash()
 *
 * This is the "high level" API for writing to the flash
 * chip. It breaks up the writes internally to insure
 * that they are less than or equal to 256 bytes and
 * that they do not cross page boundaries (which leads
 * to wrap around).
 */
int
qspi_write_flash(uint32_t addr, uint8_t *buf, int len)
{
	uint32_t cur_addr;
	int frac_len;

	cur_addr = addr;
	while (len > 0) {
		/* check the case where it crosses a page boundry */
		if ((cur_addr & ~0xff) != ((cur_addr + len - 1) & ~0xff)) {
			frac_len = ((cur_addr + 256) & ~0xff) - cur_addr;
			qspi_write_data(cur_addr, buf, frac_len);
			cur_addr = cur_addr + frac_len;
			len = len - frac_len;
			buf = buf + frac_len;
		} else {
			qspi_write_data(cur_addr, buf, len);
			len = 0;
		}
	}
	return 0; /* XXX have this return error if write_data fails */
}

/*
 * Map FLASH into the Cortex M address space
 *
 * This is accomplished by setting a 'read' instruction
 * into the CCR and putting the chip into memory mapped
 * mode.
 */
void
qspi_map_flash(void)
{
	uint32_t	ccr;
	write_flash_register(VOLATILE_REG, 0xA3); /* enable XIP mode */
	ccr = QUADSPI_SET(CCR, INST, FLASH_QUAD_READ); 	/* this will be our read mode */
	ccr |= QUADSPI_SET(CCR, FMODE, QUADSPI_CCR_FMODE_MEMMAP);
	ccr |= QUADSPI_SET(CCR, DCYC, 10);
	ccr |= QUADSPI_SET(CCR, DMODE, QUADSPI_CCR_MODE_4LINE);
	ccr |= QUADSPI_SET(CCR, ADMODE, QUADSPI_CCR_MODE_4LINE);
	ccr |= QUADSPI_SET(CCR, IMODE, QUADSPI_CCR_MODE_1LINE);
	ccr |= QUADSPI_SET(CCR, ADSIZE, 2);
	ccr |= QUADSPI_CCR_SIOO;
	QUADSPI_CCR = ccr;
}

/*
 * Unmap FLASH out of the memory space.
 *
 * This is accomplished by changing the CCR register to
 * not be in mapped mode.
 */
void
qspi_unmap_flash(void)
{
}
