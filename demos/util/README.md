Utility Functions
-----------------

Not part of the library, these utility functions provide some
helper functions for the examples that simplify things.

**clock.c** - sets up the clock to 168Mhz and and enables the
	SysTick interrupt with 1khz interrupts.

**console.c** - a set of convienience routines for using the debug
	serial port (USART3) which is availble as /dev/ttyACM0 on
	the linux box, as a console port.

**retarget.c** - implements the minimum set of character I/O functions
	and automatically plumbs them so that you can use standard
	functions like printf() in your programs. Note that it has weak
    calls to sdram_init and led_init which means that if you include
    the .o files for sdram or leds before the retarget.o object it
    will automatically link in that code for you and initialize it prior
    to entering the main() function. I'll add additional init functions
    as I go along so you can easily customize what utility functions
    you want in your code.

**sdram.c** - initialize the SDRAM chip on the board (16MB!) of RAM will
    be available at 0XC000000.

**leds.c** - add some functions that can know about the on board LEDs (red,
    green, blue, and orange) can can turn them on, off, or toggle them.
