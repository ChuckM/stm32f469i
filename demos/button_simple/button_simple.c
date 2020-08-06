/*
 * button_simple.c
 *
 * This is the simplest button demo, it talks to the 'wakeup' (aka Blue)
 * button that is attached to PA0 on the STM32F469I-DISCO board.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "../util/util.h"

int
main(void)
{
	int press_count = 0;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);

	fprintf(stderr, "Simple Button Demo\n");
	printf("Press the BLUE button. The LEDs should blink in sequence.\n");
	/* turn off all LEDs */
	all_leds_off();
	while (1) {
		/* wait for the press */
		while (gpio_get(GPIOA, GPIO0) == 0) ;
		press_count++;
		switch (press_count % 4) {
			default:
				off_led(GREEN_LED);
				on_led(BLUE_LED);
				break;
			case 1:
				off_led(BLUE_LED);
				on_led(RED_LED);
				break;
			case 2:
				off_led(RED_LED);
				on_led(ORANGE_LED);
				break;
			case 3:
				off_led(ORANGE_LED);
				on_led(GREEN_LED);
				break;
		}
		/* wait for the release */
		while (gpio_get(GPIOA, GPIO0) != 0) ;
		printf("Button pressed %4d times.\n", press_count);
	}
}
		
