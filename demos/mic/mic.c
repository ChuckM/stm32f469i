/*
 * mic.c - Microphone Example
 *
 * You will notice there are *very* few (like I could not find one)
 * examples of people using the MEMs microphones on this evaluation
 * board.
 *
 * This example seeks to fix that.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include "../util/util.h"

void sample_start(void);
void sample_end(void);

/*
 * relocate the heap to the DRAM, 10MB at 0xC0000000
 */
void
local_heap_setup(uint8_t **start, uint8_t **end)
{
	console_puts("Local heap setup\n");
	*start = (uint8_t *)(0xc0000000);
	*end = (uint8_t *)(0xc0000000 + (10 * 1024 * 1024));
}

void
sample_start(void)
{
	/* enable the I2S port */
	SPI_I2SCFGR(SPI3) |= SPI_I2SCFGR_I2SE;
	/* start sending it clocks */
	TIM4_CR1 |= TIM_CR1_CEN;
}

void
sample_end(void)
{
	/* stop sending it clocks */
	TIM4_CR1 = TIM4_CR1 & (~TIM_CR1_CEN);
	/* enable the I2S port */
	SPI_I2SCFGR(SPI3) = SPI_I2SCFGR(SPI3) & (~SPI_I2SCFGR_I2SE);
	gpio_clear(GPIOB, GPIO12);
}

/* Ok to get down to brass tacks.
 *
 * Timer 4 is used to drive the microphones and the I2S #3 peripheral
 * in order to read bits in.
 *
 * So mapping out the pins and their functions here, this is for the
 * MEMS setup on the board:
 * 
 * PB3 -> (AF6) I2S #3, CK
 * PD13 -> (AF2) TIM4 Channel 2 (connected to Mic clock)
 * PD12 -> (AF2) TIM4 Channel 1 (connected to PB3) (disables SWO)
 * PD6 -> (AF5) I2S #3, SD
 */
int
main(void)
{
	uint16_t	*raw_bits;
	uint8_t		*pcm_samples;
	uint32_t	reg;
	int		iter;

	printf("\nMicrophone sampling example\n");
	rcc_periph_clock_enable(RCC_TIM4);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_SPI3);

	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_clear(GPIOB, GPIO12);
	/* Set up the pins as alternate function, and pick AF2 (timer) */
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE,
					GPIO12 | GPIO13 | GPIO6);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
/* this is the WS pin */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO15);
	gpio_set_af(GPIOA, GPIO_AF6, GPIO15);

	gpio_set_af(GPIOD, GPIO_AF2, GPIO12 | GPIO13);
	gpio_set_af(GPIOD, GPIO_AF5, GPIO6);
	gpio_set_af(GPIOB, GPIO_AF6, GPIO3);


	TIM4_CCMR1 = TIM_CCMR1_OC2CE | TIM_CCMR1_OC2M_TOGGLE |
				 TIM_CCMR1_OC1CE | TIM_CCMR1_OC1M_TOGGLE;
	/*	do I need to set TIM4_CCER ? Yes I do.*/
	TIM4_CCER = TIM_CCER_CC2E | TIM_CCER_CC1E;
	/* Connect timer to HCLK (168Mhz) */
	RCC_DCKCFGR |= RCC_DCKCFGR_TIMPRE;
	/* Set ARR and CCRx */
	TIM4_ARR = (168/6) - 1; /* 6Mhz toggle rate, 3Mhz frequency */
	TIM4_CCR2 = 0;
	TIM4_CCR1 = 0; /* Ch 1 and Ch2 in phase */
	TIM4_PSC = 0;

/* missing defines */
#define SPI_I2SCFGR_I2SCFG_SHIFT	8
#define SPI_I2SCFGR_I2SCFG_MASK		0x3
#define SPI_I2SCFGR_I2SSTD_SHIFT	4
#define SPI_I2SCFGR_I2SSTD_MASK		0x3

	reg = SPI_I2SCFGR(SPI3);
	printf("Initial configuration: 0x%0X\n", (unsigned int) reg);
/*
 * Showing 0xD10 in this register
 *	0000 1101 0001 0000
 *       |||| | || ||| \ chlen = 0
 *       |||| | || |++----DATLEN = 0x00 (16 bit)
 *       |||| | || +------CKPOL = 0
 *       |||| | ++--------I2SSTD = 01 (MSB)
 *       |||| +-----------PCMSYNC = 0
 *       ||++-------------I2SCFG = 01 (Slave Recv)
 *       |+---------------I2SE = 1 (enabled)
 *       +----------------I2SMOD = 1 (I2S Mode)
 * Device goes "busy" but never actually gives us a data byte.
 *
 * EXP 1: Change I2S std to Phillips I2S (00)
 *		Result: No change
 * EXP 2: Change CKPOL
 *		Result: No Change
 * EXP 3: What now? Clock speed change (no change)
 * EXP 4: Trying to 'kick' WS to start it off
 *
 * What pins can be I2S3 WS ? 
 *	PA4 (with AF6)
 *  PA15 (with AF6)
 *
 * So we've configured PA15 as I2S3 WS pin but we don't do
 * anything with it (it should float high I believe) 
 * It didn't seem to do anything
 *
 * EXP 5: Back to 3Mhz clock and we'll manually try to 
 * kick it off by making WS go high. SUCCESS! Sort of
 *
 * EXP 6: Can we start it by doing a gpio_set of GPIO15 ?
 * apparently that works too. But unreliable.
 *
 * EXP 7: Connecting PB12 to PA15 (WS) and kicking it that way.
 *
 */
	SPI_I2SCFGR(SPI3) = SPI_I2SCFGR_I2SMOD |
			(SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE << SPI_I2SCFGR_I2SCFG_SHIFT) |
			(SPI_I2SCFGR_I2SSTD_MSB_JUSTIFIED << SPI_I2SCFGR_I2SSTD_SHIFT);
	
	/* 375K 16 bit buckets */
	raw_bits = (uint16_t *)malloc(375000 * 2);
	if (raw_bits == NULL) {
		printf("Malloc failed for raw bits\n");
		while (1) ;
	} else {
		printf("raw_bits located at 0x%0x\n", (unsigned int) raw_bits);
	}
	/* 12K 8 bit pcm samples */
	pcm_samples = (uint8_t *)malloc(12000);
	if (pcm_samples == NULL) {
		printf("Malloc failed for PCM samples\n");
		while (1);
	} else {
		printf("pcm_samples located at 0x%0x\n", (unsigned int) pcm_samples);
	}
	iter = 0;
	while (1) {
		char buf[128];
		int	ndx;

		printf("Iteration #%d: Ready to sample, press enter to start\n", iter);
		fgets(buf, 128, stdin);
		sample_start();
		gpio_clear(GPIOB, GPIO12);
		gpio_set(GPIOB, GPIO12);
		for (ndx = 0; ndx < 375000; ndx++) {
			do {
				reg = SPI_SR(SPI3);
				gpio_toggle(GPIOB, GPIO12);
			} while ((reg & SPI_SR_RXNE) == 0);
			raw_bits[ndx] = SPI_DR(SPI3);
		}
		sample_end();
		printf("\nDone.\n");
		iter++;
	}
}
