/*
 * blink_systick.c
 *
 * A bit more useful, this blinker sets up the standard "SysTick" timer
 * of the Cortex-M series so that you can use it to do more precise
 * delay times.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

/*
 * Simple SysTick handler, maintains a monotonically increasing
 * number of 'ticks' from reset. If you're doing 1 kHz ticks (this
 * code does) then it overflows every 49 days if you're wondering.
 */

volatile uint32_t system_millis;
void sys_tick_handler(void)
{
	system_millis++;
}

/* "Sleep" (aka busy wait) for 'delay' milliseconds */
static void msleep(uint32_t delay)
{
	uint32_t wake = system_millis + delay;
	while (wake > system_millis);
}

/*
 * systick_setup(void)
 *
 * This function sets up the 1khz "system tick" count. The SYSTICK counter is a
 * standard feature of the Cortex-M series.
 */
static void systick_setup(void)
{
	/* clock rate / 1000 to get 1mS interrupt rate */
	systick_set_reload(168000);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();
	/* this done last */
	systick_interrupt_enable();
}

/* Set STM32 system clock to 168 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	/* Enable GPIOD clock. */
	rcc_periph_clock_enable(RCC_GPIOD);
}

static void gpio_setup(void)
{
	/* Set GPIO4-5 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			GPIO4 | GPIO5);
}

int main(void)
{
	clock_setup();
	gpio_setup();
	systick_setup();

	/* Set two LEDs for wigwag effect when toggling. */
	gpio_set(GPIOD, GPIO4);

	/* Blink the LEDs (PG13 and PG14) on the board. */
	while (1) {
		gpio_toggle(GPIOD, GPIO4 | GPIO5);
		msleep(100);
	}

	return 0;
}
