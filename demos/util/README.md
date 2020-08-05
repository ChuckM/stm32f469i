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

## I2C Clock calculation

I2C is powered by the `APB1` clock on the STM32F4 chips. The value  `T(pclk1)`
is the time in seconds "per clock" or 1 / `APB1`'s clock rate.

In this example `APB1` is running at 42 MHz. Note that `APB1` is nominally the 
"slow" peripheral bus on these parts and always limited by the data sheet
to typically one fourth of the max system clock, which on the STM32F469 is
180 MHz. So the max speed of this bus is 45 MHz.

This repository sets the clock speed of the System Clock to 168 MHz, and that
divided by four is 42 MHz.

In the i2c standard, there are a couple of standard baud rates, these are
100 kHz (standard mode) and 400 kHz (fast mode). Later versions of the
standard support 1 MHz and 3.4 MHz mode but these peripherals don't easily
get to those clock rates so this discussion ignores them.

There are two differences between standard mode and fast mode. The first, as
mentioned above, is their baud rate (100 kHz vs 400 kHz). The other is the
symmetry of the clock waveform. 

In standard mode, the clock is symmetric (50% duty cycle). In fast mode the
clock is assymetic with 33%/66% for (high/low) duration. This is done
because i2c devices read data on the rising edge of the clock and by
extending the width of the low clock, additional timing margin is given to
the device to take action. It also helps deal with line capacitance by
increasing the time the line is held at the ground state.

For this example, we want a baud rate of 400 kHz (this is the fastest rate
we can talk to the touch controller on this board.
 
As this is fast mode, the duty cycle for the I2C clock 1:2. There is an
option to make it 9:16 as well if the bus needs the extra margin.

`T(low)` is the time spent with the clock at 0, `T(high)` is the time spent
with the clock at 1. `T(low)` is twice the length of `T(high)` for a 1:2
symmetry on the clock.

The `I2C_CCR` register takes a constant that sets all these time periods
and this is how we compute it.

```
    T(high) = CCR * T(plck1), T(low) = 2 * CCR * T(plck1)
```

With this we can solve for CCR as follows:

```
           1
         ------ = T(high) + T(low)                        [1]
          Baud

           1
         ------ = CCR * T(plck1) + 2 * CCR * T(plck1)     [2]
          Baud

           1          CCR         2 * CCR
         ------ =  ---------- +  ----------               [3]
          Baud     Freq(APB1)    Freq(APB1)

          Freq(APB1)
          ---------- = 3 * CCR                            [4]
             Baud

          Freq(APB1)
          ---------- = CCR                                [5]
           3 * Baud
```

Filling in the known values for `APB1` and Baud:

```
            42000000
           ---------- = 35                               [6]
             1200000 
```

In the code I initialize a variable `fpclk` to `Freq(APB1) / 1MHz`. We can
take a factor of 100,000 out of both the numerator and denominator of [6]
we can re-write this to:

```
        I2C_CCR = fpclk * 10 / 12; 
```
Sticking with integer math, 10 / 12 reduces to 5 / 6 so simplest form is:
```
        CCR = fpclk * 5 / 6;
```


