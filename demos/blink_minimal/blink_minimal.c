/*
 * blink_minimal.c
 *
 * The simplest implementation of blink. Just some demonstration
 * code and not especially useful.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void gpio_setup(void)
{
	/* Enable GPIOD clock. */
	/* Manually: */
	/* RCC_AHB1ENR |= RCC_AHB1ENR_IOPGEN; */
	/* Using API functions: */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set GPIO13 (in GPIO port G) to 'output push-pull'. */
	/* Manually: */
	/* GPIOG_CRH = (GPIO_CNF_OUTPUT_PUSHPULL << 2); */
	/* GPIOG_CRH |= (GPIO_MODE_OUTPUT_2_MHZ << 2); */
	/* Using API functions: */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
}

/*
 * Three ways you could inefficiently cause and LED to blink on
 * the STM32F469i disco board.
 * Pick one of the three #defines below and uncomment it:
 */

// #define MANUAL
// #define SET_RESET
#define TOGGLE
int main(void)
{
	int i;

	gpio_setup();

	/* Blink the LED (PG13) on the board. */
	while (1) {
		/*
		 * Manually:
		 */
#ifdef MANUAL
		GPIOG_BSRR = GPIO13;		/* LED off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		GPIOG_BRR = GPIO13;		/* LED on */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
#endif

		/*
		 * Using API functions gpio_set()/gpio_clear(): 
		 */
#ifdef SET_RESET
		gpio_set(GPIOG, GPIO13);	/* LED off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		gpio_clear(GPIOG, GPIO13);	/* LED on */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
#endif

		/*
		 * Using API function gpio_toggle():
		 */
#ifdef TOGGLE
		gpio_toggle(GPIOG, GPIO6);	/* LED on/off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
#endif
	}

	return 0;
}
