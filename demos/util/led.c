/*
 * Some utility functions for manipulating the LEDs on the board
 *
 * LEDs
 *		LED1  PG6 (green)
 *		LED2  PD4 (orange)
 *		LED3  PD5 (red)
 *		LED4  PK3 (blue)
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "../util/util.h"

void
on_led(LED_COLOR c)
{
	switch (c) {
		case RED_LED:
			return gpio_set(GPIOD, GPIO5);
		case GREEN_LED:
			return gpio_set(GPIOG, GPIO6);
		case BLUE_LED:
			return gpio_set(GPIOK, GPIO3);
		case ORANGE_LED:
			return gpio_set(GPIOD, GPIO4);
		default:
			break;
	}
}

void
off_led(LED_COLOR c)
{
	switch (c) {
		case RED_LED:
			return gpio_clear(GPIOD, GPIO5);
		case GREEN_LED:
			return gpio_clear(GPIOG, GPIO6);
		case BLUE_LED:
			return gpio_clear(GPIOK, GPIO3);
		case ORANGE_LED:
			return gpio_clear(GPIOD, GPIO4);
		default:
			break;
	}
}

void
toggle_led(LED_COLOR c)
{
	switch (c) {
		case RED_LED:
			return gpio_toggle(GPIOD, GPIO5);
		case GREEN_LED:
			return gpio_toggle(GPIOG, GPIO6);
		case BLUE_LED:
			return gpio_toggle(GPIOK, GPIO3);
		case ORANGE_LED:
			return gpio_toggle(GPIOD, GPIO4);
		default:
			break;
	}
}

void
led_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOK);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4 | GPIO5);
	gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
}

