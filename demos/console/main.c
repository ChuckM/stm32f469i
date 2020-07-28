/*
 * Simple demo for the console code. A 'console' is a serial
 * port for controlling programs, it is set up as both standard
 * input and standard output for the program.
 */

/*
 * USART example (alternate console)
 */

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include "../util/util.h"

/*
 * Set up the GPIO subsystem with an "Alternate Function"
 * on some of the pins, in this case connected to a
 * USART.
 */
int main(void)
{
	char buf[128];
	int	len;

	clock_setup(168000000, 0); /* initialize our clock */

	console_setup(57600);

	/* At this point our console is ready to go so we can create our
	 * simple application to run on it.
	 */
	console_puts("\nUART Demonstration Application\n");
	while (1) {
		console_puts("Enter a string: ");
		len = console_gets(buf, 128);
		if (len > 1) {
			buf[len-1] = 0; /* kill trailing newline */
			console_puts("\nYou entered : '");
			console_puts(buf);
			console_puts("'\n");
		} else {
			console_puts("\nNo string entered\n");
		}
	}
}
