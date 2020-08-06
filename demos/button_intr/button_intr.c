/*
 * button_intr.c
 *
 * This is an interrupt driven button example, it talks to the 'wakeup'
 * (aka Blue) button that is attached to PA0 on the STM32F469I-DISCO board.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "../util/util.h"

/* indicates the button was pressed */
volatile int button_press = 0;
/* indicates the current state of the button */
volatile uint32_t button_down = 0;
volatile uint32_t press_time;

void
exti0_isr(void)
{
	/*
	 * If the GPIO is "high" it means the button transitioned
	 * from not pushed to pushed.
	 */
	if (gpio_get(GPIOA, GPIO0)) {
		button_down = mtime();
	} else {
		press_time = mtime() - button_down;
		/* if it was held < 2 mS it probably wasn't an intentional press */
		if (press_time > 1) {
			button_press++;
		}
		button_down = 0;
	}

	EXTI_PR = 1; /* reset the pending flag */
}

int
main(void)
{
	int press_count = 0;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);

	/*
	 * Set "SOURCE" for the EXTI0 interrupt to port A. This "connects"
	 * PA0 to the EXT0 interrupt.
	 *
	 * Since the default value of SYSCFG_CTL[0 .. 4] is 0, all port A
	 * pins are routed to EXTI pins by default. So for this particular
	 * example, you don't ACTUALLY need to enable the SYSCFG clock and
	 * do anything. However, if you are using a pin from any other port
	 * (which means a non-zero value in one of the SYSCFG_CTL registers,
	 * then you DO have to enable the SYSCFG clock. So it's good habit
	 * to turn it on unless you're really trying to save nanoamps of power.
	 */
#ifndef LOOK_MA_NO_SYSCFG
	rcc_periph_clock_enable(RCC_SYSCFG);

	SYSCFG_EXTICR1 = (0) << 0;
#endif
	
	/*
	 * Now set up the EXTI peripheral to look for both falling edges
	 * and rising edges (so that we can track button state).
	 */
	EXTI_RTSR |= 1; /* rising edge detect on EXTI0 */
	EXTI_FTSR |= 1; /* falling edge detect on EXTI0 */
	EXTI_IMR |= 1;	/* Allow interrupts from EXTI0 */
	
	/*
	 * Finally, tell the NVIC peripheral to enable EXTI0 interrupts.
	 */
	nvic_enable_irq(NVIC_EXTI0_IRQ);

	fprintf(stderr, "Interrupt Driven Button Demo\n");
	printf("Press the BLUE button. The LEDs should blink in sequence.\n");
	/* turn off all LEDs */
	all_leds_off();
	while (1) {
		/* wait for the press */
		while (button_press == 0) ;
		press_count++;
		button_press = 0; /* remember we've processed it */
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
		printf("Last Button press was %4ld milleseconds.\n", press_time);
	}
}
		
