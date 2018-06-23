/*
 * MEMs driver
 *
 * Copyright (C) 2017 Chuck McManis, All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <math.h>
#include <gfx.h>

#include "../util/util.h"
#include "mems.h"

/*
 * Enable this flag to configure as an I2S device, if it
 * is not defined we configure as a SPI device.
 */
// #define USE_I2S

/*
 * MEMS Setup
 * -------------------------------------------------------
 */

void
mems_init()
{
	rcc_periph_clock_enable(RCC_SPI3);
	rcc_periph_clock_enable(RCC_TIM4);
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13);
	/* Setting I/O pin to input  allows it to go up and down as expected */
/*
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
*/
	/* TEST XXX mode, lets just read the pin and see what we see */
	gpio_mode_setup(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO6);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	return;
	/* XXX END test mode */

	/* Experiement, set input and also set the AF pins */
	gpio_set_af(GPIOD, GPIO_AF5, GPIO6); /* SPI 3 */
	gpio_set_af(GPIOD, GPIO_AF2, GPIO13); /* TIM4 */

	/*
	 * Set up the timer to generate a PCLK/16 clock.
	 */
	RCC_DCKCFGR |= RCC_DCKCFGR_TIMPRE;
	/* Output on CH2, mode toggle */
	TIM4_CCMR1 = TIM_CCMR1_OC2CE | TIM_CCMR1_OC2M_TOGGLE;
	/* Enable this timer channel 2 */
	TIM4_CCER = TIM_CCER_CC2E;
	/* ARR is (Delay - 1) */
	TIM4_ARR = 15;
	/* Clock toggle is right at the point where it resets */
	TIM4_CCR2 = 0;
	TIM4_CR1 = TIM_CR1_CEN;

#ifndef USE_I2S
	/*
 	 * Set up SPI3 so that we can read data coming in
	 * from the microphone using a PCLK/16 clock rate
	 * Which in this case is 2.8125 Mhz (45Mhz / 16)
	 * Because the microphone data line is connected to MOSI
	 * (aka master OUT and slave IN) and we don't have a
	 * way to feed SPI a clock, we resort to "Bi Directional"
	 * mode as a master, with the MOSI pin becomes an input
	 * when we're 'read' mode.
	 * We also configure it for 16 bit I/O because the I2S
	 * forces 16 bit I/O and this makes them the same.
	 */

#define SPI_FLAGS (SPI_CR1_MSTR | SPI_CR1_SSI |\
				   SPI_CR1_BAUDRATE_FPCLK_DIV_16 |\
				   SPI_CR1_DFF | SPI_CR1_BIDIMODE)
	SPI_CR2(SPI3) = SPI_CR2_SSOE;
	SPI_CR1(SPI3) = SPI_FLAGS;
	SPI_CR1(SPI3) |= SPI_CR1_SPE;
	/* if we generate a mode failure, rewrite CR1 to reset */
	if (SPI_SR(SPI3) & SPI_SR_MODF) {
		SPI_CR1(SPI3) = SPI_FLAGS;
	}
	/* At this point every byte we 'transmit' on SPI3 will clock in 8 bits
	 * from the MEMS microphone */
#else
	SPI_I2SCFGR(SPI3) = SPI_I2SCFGR_I2SMOD |
						(SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE << 8) |
						(SPI_I2SCFGR_I2SSTD_LSB_JUSTIFIED << 4);
	SPI_I2SPR(SPI3) = 12 | SPI_I2SPR_MCKOE;
	SPI_I2SCFGR(SPI3) |= SPI_I2SCFGR_I2SE;
#endif
}

/*
 * bit banded addresses
 */

#define GPIO_BASE(x) (GPIOA + ((((uint32_t)(x) >> 4) & 0xf) << 10))
#define GPIO_BIT(x)	(x % 16)
#define GPIO_MASK(x) (1 << ((uint32_t)(x) % 16))

int
get_bit(int pin)
{
	/* Bit banded addresses let you read and write a single bit
	 * as it if were a byte.
	 * GPIOD base address is 0x40020c00
	 */
	int bit = GPIO_BIT(pin);
	int addr = ((GPIO_BASE(pin) + 0x10) - 0x40000000);
	if (bit > 7) {
		addr++;
		bit -= 8;
	}
	return *(int *)((addr * 32 + bit * 4) + 0x42000000U);
}

void
put_bit(int pin, int val)
{
	/* Bit banded addresses let you read and write a single bit
	 * as it if were a byte.
	 * GPIOD base address is 0x40020c00
	 */
	int bit = GPIO_BIT(pin);
	int addr = ((GPIO_BASE(pin) + 0x10) - 0x40000000);
	if (bit > 7) {
		addr++;
		bit -= 8;
	}
	*(int *)((addr * 32 + bit * 4) + 0x42000000U) = val;
	return;
}

