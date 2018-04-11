/*
 * Copyright (C) 2015-2016 Chuck McManis <cmcmanis@mcmanis.com>
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "../util/util.h"


int led_state = 0;

void rotate_leds(void);

void
rotate_leds(void)
{
	led_state++;
	gpio_set(GPIOG, GPIO6);
	gpio_set(GPIOD, GPIO4 | GPIO5);
	gpio_set(GPIOK, GPIO3);
/*
	* LED1	PG6	(green)
	* LED2	PD4	(orange)
	* LED3	PD5	(red)
	* LED4	PK3	(blue)
*/
	switch (led_state % 6) {
		case 0:
			gpio_clear(GPIOG, GPIO6);
			break;
		case 1:
		case 5:
			gpio_clear(GPIOD, GPIO4);
			break;
		case 2:
		case 4:
			gpio_clear(GPIOD, GPIO5);
			break;
		default:
			gpio_clear(GPIOK, GPIO3);
			break;
	}
}

int main(void)
{
	double flash_rate = 3.14159;
	char buf[128];
	uint32_t done_time;

	/* ../util/retarget.c has automatically set up the clock rate, systick
	 * and made the virtual COM port on the board into the "console" or
	 * standard output and input.
	 *
	 * When the output is sent to standard error, it is highlighted in a
	 * different color (if you have a color xterm)
	 */
	fprintf(stderr, "\nSTM32F469-Discovery : ");
	printf("Retargeting LIBC example.\n");

	/* This is the LED on the board */
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOK);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5 | GPIO4);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
	gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);

	led_state = 0; /* useful when restarting in gdb */

	/* You can create a running dialog with the console */
	printf("We're going to start blinking the LED, between 0 and 300 times per\n");
	printf("second.\n");
	while (1) {
		printf("Please enter a flash rate: ");
		fflush(stdout);
		fgets(buf, 128, stdin);
		buf[strlen(buf) - 1] = '\0';
		flash_rate = atof(buf);
		if (flash_rate == 0) {
			printf("Was expecting a number greater than 0 and less than 300.\n");
			printf("but got '%s' instead\n", buf);
		} else if (flash_rate > 300.0) {
			printf("The number should be less than 300.\n");
		} else {
			break;
		}
	}
	/* MS per flash */
	done_time = (int) (500 / flash_rate);
	printf("\nThe closest we can come will be %f flashes per second\n", 500.0 / done_time);
	printf("With %d MS between states\n", (int) done_time);
	printf("Press ^C to restart this demo.\n");
	while (1) {
		msleep(done_time);
		rotate_leds();
	}
}
