/*
 * timer.c - Some timer examples.
 *
 * The timers on the STM32F4 are pretty rich in functionality,
 * no more just a set it and get an interrupt you can do all
 * sorts of fancy things with them up to and including cascading
 * triggers and DMAs. 
 *
 * Unfortunately I need some of that trickyness to deal with the
 * fact that the STM32F469 does *NOT* have the DFSDM peripheral but
 * the discovery board does have MEMS microphones. So to talk to the
 * microphone we have to get a bit tricky.
 *
 * You will notice there are *very* few (like I could not find one)
 * examples of people using the MEMs microphones on this evaluation
 * board.
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

/* Ok to get down to brass tacks.
 * There isn't an easy way to experiment with the timer (clock)
 * and input (data) pins on the MEMs microphone so to verify our
 * understanding and to try out different strategies we are going
 * to do some experiments with similar peripherals that come out
 * to the EXT connector.
 *
 * To see anything exciting you kind of need to hook an oscilloscope
 * up to these pins.
 * PB4 (`TIM3_CH1`) is on pin 5
 * PB5 (`TIM3_CH2`) is on pin 9 (PD13 can be TIM4_CH2)
 * PC6 (`I2S2_MCK`) is on pin 6 of EXT
 * PC1 (`I2S2_SD`) is on pin 14
 * PB13 (`I2S2_CK`, `SPI2_SCK`) is on pin 10 (PD6 can be 
 *								 `I2S3_SD` or `SPI3_MOSI`)
 * PA8 (`USART1_CK`) is on pin 3 (PD6 can be `USART2_RX`)
 */
int
main(void)
{
	printf("\nThis will be the TIMER 3 Example\n");
	rcc_periph_clock_enable(RCC_TIM3);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOA);
	/* Now to look at the timer bit rate and the sync USART clock rate */
	rcc_periph_clock_enable(RCC_USART1);

	/* And to look at SPI2 clock rates */
	rcc_periph_clock_enable(RCC_SPI2);

	/* Set up the pins as alternate function, and pick AF2 (timer) */
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, 
							GPIO4 | GPIO5 | GPIO13);
	gpio_set_af(GPIOB, GPIO_AF2, GPIO4 | GPIO5);
	/* GPIO 13 is SPI2 clock */
	gpio_set_af(GPIOB, GPIO_AF5, GPIO13);

	/* This is the USART1 Clock line */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO8);

	/* Select the Timer Clock */
	/* By default we get the APB2 clock */
	printf("Set up timer for CH1 & CH2 output compare ... \n");
	/* Enable Output Compare 2, set mode to Toggle 
	 *	OC2M_TOGGLE does what it says, toggles
	 * Experiment: What does Frozen do? nothing (never changes)
	 *		
	 */
	TIM3_CCMR1 = TIM_CCMR1_OC2CE | TIM_CCMR1_OC2M_TOGGLE |
				 TIM_CCMR1_OC1CE | TIM_CCMR1_OC1M_TOGGLE;
	/*	do I need to set TIM4_CCER ? Yes I do.*/
	TIM3_CCER = TIM_CCER_CC2E | TIM_CCER_CC1E;
	/* Make timer 3 count down, and enable it */
	/* Question: If the timer counts down, does it matter what
	 * the CCR2 value is? 
	 * 	Experiment 1: ARR 10,000, CCR2 5000
 	 *			square wave at 4.5kHz
	 *	Experiment 2: CCR2 1000
	 *		Just changing CCR2 did not change the base frequency
	 *		of the square wave.
	 *	Experiement 3: ARR 1,000, CCR2 0
	 *		This made the square wave 10x faster at 45.00kHz
	 *  Experiment 4: Making the counter to count "up" (ARR 1,000, CCR2 0)
	 *		Oddly, that changed nothing.
	 *  Experiment 5: Making the counter to count "up" (ARR 1,000, CCR2 2,000)
	 *		Trying a CCR2 that is "out of range" ?
	 *		And as expected the toggle never fires.
	 * So what is the 'base clock' here that we are dividing by?
	 * System clock is 180Mhz
	 * Computing backwards 180,000Khz / 45.00Khz = 4000, so the base
	 * clock must be 45Mhz. That suggests a divisor of 4500 would give
	 * us a 1Khz clock, lets see if that works.
	 *		Results: 4,500 gave us 10Khz. Now since we are toggling
	 *			the clock is "actually" double that or 90Mhz.
	 *
	 * Experiment 6: Set the TM_PRE bit to see if we boost the speed
	 */
	RCC_DCKCFGR |= RCC_DCKCFGR_TIMPRE;
	/* EXp 6: Yes we do, double the performance TIMxCLK = HCLK */
	/* Set ARR and CCRx */
	TIM3_ARR = 31;
	TIM3_CCR2 = 0;
	TIM3_CCR1 = 10;
	TIM3_CR1 = TIM_CR1_CEN;
	printf("Timer should be running + increments, - decrements\n");

	printf("Now setting up the USART\n");
	USART_BRR(USART1) = 2 << 4; 
	USART_CR2(USART1) = USART_CR2_CLKEN | USART_CR2_LBCL;
	USART_CR1(USART1) = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
	/*
	 * Experiment 7:
	 * 	If we set up a USART to be synchronous, is its clock the
	 *	same as the timer output? Or alternatively if we used a
	 *	timer to clock the microphone could we use sync serial reads
	 *  to sample the microphone.
	 *
	 * Results of Experiment 7 are:
	 *   When TIM3_ARR == USART_BRR - 1 the clocks look
	 *	 identical, caveat that the serial port drops clocks
	 *	 between "characters" so 8 clocks, 2 spaces, 8 clocks.
	 *
	 *	By using the CRR value in the timer we can offset the timer
	 * 	and the data read. However, it is unclear how intentional we
	 *  can be about that.
	 *
	 *	Unanswered question for the microphone is what effect would
	 *	missing 2 bits out of 10 or 20% of the samples have on signal
	 *	quality.
	 */
	/* So SPI seems to work, HOWEVER on the board with the microphone
	 * the output of the microphone, when the pin is set to MOSI?
	 */
	printf("Now setting up SPI2\n");
#define SPI_FLAGS (SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_BAUDRATE_FPCLK_DIV_16)
	SPI_CR2(SPI2) = SPI_CR2_SSOE;
	SPI_CR1(SPI2) = SPI_FLAGS;
	SPI_CR1(SPI2) |= SPI_CR1_SPE;
	if (SPI_SR(SPI2) & SPI_SR_MODF) {
		SPI_CR1(SPI2) = SPI_FLAGS;
	}


	 /* Experiment 8:
	 *	How about that SPI2 clock? That should be very interesting.
 	 */
	while (1) {
		char c = console_getc(0);
		/* keep a continual series of USART clocks running */
		if (USART_SR(USART1) & USART_SR_TXE) {
			USART_DR(USART1) = 0xaa;
		}
		/* Keep a continual series of SPI clocks running */
		if (SPI_SR(SPI2) & SPI_SR_TXE) {
			SPI_DR(SPI2) = 0x55;
		}
		switch (c) {
			case '=':
			case '+':
				TIM3_ARR++;
				break;
			case '-':
			case '_':
				TIM3_ARR--;
				break;
			case '[':
				USART_BRR(USART1)--;
				break;
			case ']':
				USART_BRR(USART1)++;
				break;
			case ' ':
				printf("ARR = 0x%05x, BRR = 0x%05x\n", 
					(unsigned int) TIM3_ARR, (unsigned int) USART_BRR(USART1));
				break;
			default:
				break;
		}
	}
}
