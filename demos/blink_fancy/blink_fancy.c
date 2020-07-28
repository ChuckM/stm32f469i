/*
 * blink_fancy.c
 *
 * A simple blinker that blinks multiple LEDs
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* Set STM32 to 168 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	/* Enable GPIOD, GPIOG, and GPIOK clock. */
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOK);
}

static void gpio_setup(void)
{
	/* Set GPIO6 (in GPIO port G) to 'output push-pull'. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
	/* Set GPIO4 and GPIO5 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4 | GPIO5);
	/* Set GPIO3 (in GPIO port K) to 'output push-pull'. */
	gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
}

int main(void)
{
	int i;

	clock_setup();
	gpio_setup();

	/* Set two LEDs for wigwag effect when toggling. */
	gpio_set(GPIOG, GPIO6);
	gpio_set(GPIOK, GPIO3);

	/* Blink the LEDs (PG13 and PG14) on the board. */
	while (1) {
		/* Toggle LEDs. */
		gpio_toggle(GPIOG, GPIO6);
		gpio_toggle(GPIOD, GPIO4 | GPIO5);
		gpio_toggle(GPIOK, GPIO3);
		for (i = 0; i < 6000000; i++) { /* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
