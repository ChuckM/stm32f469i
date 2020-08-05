/*
 * Look at using the i2c functions in the library
 *
 * This example talks to the XY screen touch controller
 * (its a peripheral already on the board) and prints
 * the co-ordinates pressed on the serial port.
 */

#include <stdint.h>
#include <stdlib.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#include "../util/util.h"

static int __i2c_event(i2c_dev *chan, uint8_t events);

/* map units to registers */
static uint32_t __i2c_clock[4] = {0, RCC_I2C1, RCC_I2C2, RCC_I2C3};
static uint32_t __i2c_base[4] = {0, I2C1, I2C2, I2C3};

#define MY_I2C_TIMEOUT	100000

/*
 * Super simple helper that I use a lot to wait for an event
 * but not "forever" (the i2c bus is not particularly reliable)
 *
 * Returns true (1) if the event was seen.
 */
static int
__i2c_event(i2c_dev *chan, uint8_t events)
{
	uint32_t dev = __i2c_base[chan->i2c];
	int	timeout;

	for (timeout = 0; timeout < MY_I2C_TIMEOUT; timeout++) {
		if (I2C_SR1(dev) & events) {
			break;
		}
	}
	return (timeout < MY_I2C_TIMEOUT) ? 1 : 0;
}

/*
 * Initialize an I2C port.
 *
 * If input 'fast' is true, initialize it for
 * 400Khz operation, if it is false, initialize
 * for "regular" or 100Khz operation.
 *
 * Note: addr is expected to be the value that goes into
 * bits 7-1 (i.e. pre-shifted left by 1)
 */
i2c_dev * 
i2c_init(int i2c, uint8_t addr, uint8_t baud)
{
	uint32_t dev = __i2c_base[i2c];
	uint32_t fpclk;
	i2c_dev *res;

	if ((i2c < 1) || (i2c > 3)) {
		return NULL;
	}

	res = calloc(1, sizeof(i2c_dev));
	if (res == NULL) {
		return NULL;
	}
	res->i2c = i2c;
	res->addr = addr;

	/**** TODO: This is hard coded for the touch screen, fix that
	 */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOJ);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO8 | GPIO9);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, 
				GPIO8 | GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

	rcc_periph_clock_enable(__i2c_clock[i2c]);
	fpclk = rcc_apb1_frequency / 1000000;

	/* disable the peripheral to set clocks */
	I2C_CR1(dev) = I2C_CR1(dev) & ~(I2C_CR1_PE); 
    /* during init this check is fast enough */
	if ((fpclk < 2) || (fpclk > 42)) {
		return NULL; /* can't run */
	}
	I2C_CR2(dev) = (fpclk & 0x3f); 

	/*
 	 * Compute the clock delay, 
	 *   in normal mode this is (Freq(APB1) / 100Khz) / 2 
	 *	 in fast mode its either
	 *		- (Freq(APB1) / 400khz) / 3  (Duty 0)
	 *	 	- (Freq(APB1) / 400khz) / 25 (Duty 1)
	 * Duty 1 is the 9:16 ratio instead of 1:2 "regular"
	 * duty cycle mode (Duty = 0). For now we only support
	 * regular duty cycle mode until I can figure out when 
	 * you would need 9:16 mode.
	 *
	 */
	if (baud == I2C_400KHZ) {
		/* fpclk x (1000 / 400) / 3 == f x 5 / 6 */
		I2C_CCR(dev) = 0x8000 | (((fpclk * 5) / 6) & 0xfff);
	} else {
		/* fpclk x (1000 / 100) / 2 == fpclk x 5 */
		I2C_CCR(dev) = (fpclk * 5) & 0xfff;
	}
	I2C_TRISE(dev) = (fpclk + 1) & 0x3f;

	/* enable the peripheral */
	I2C_CR1(dev) |= I2C_CR1_PE;

	return res;
}

/*
 * Write data to i2c.
 *
 * Write bytes to the address 'addr' and return the number of
 * bytes written (including the addr byte). Returns -1 on error.
 *
 * Depending on the state of the stop flag (if it's True (!= 0) it
 * will send a STOP at the end of the write. Otherwise it does not.
 *
 * NB: If the interface times out unexpectedly it's considered an error
 * and we always send STOP to release the bus.
 *
 * Return value is number of bytes successfully sent or -1. If return != size
 * then it means stop was sent because a timeout occurred.
 */

int
i2c_write(i2c_dev *chan, uint8_t *buf, size_t size, int stop)
{
	uint32_t	dev = __i2c_base[chan->i2c];
	uint8_t addr = chan->addr;

	/* doesn't support writing 0 bytes */
	if ((size == 0) || (buf == NULL)) {
		return -1;
	}

	/* send start */
	I2C_CR1(dev) |= (I2C_CR1_START | I2C_CR1_PE);
	if (! __i2c_event(chan, I2C_SR1_SB)) {
		I2C_CR1(dev) |= I2C_CR1_STOP;
		return -1;
	}

	/* send target address with LSB == 0 (WR*) */
	I2C_DR(dev) = addr & 0xfe;
	if (!__i2c_event(chan, I2C_SR1_ADDR)) {
		I2C_CR1(dev) |= I2C_CR1_STOP;
		return -1;
	}

	(void) I2C_SR2(dev);	/* clears ADDR bit */

	for (size_t i = 0; i < size; i++) {
		if (!__i2c_event(chan, I2C_SR1_TxE | I2C_SR1_BTF)) {
			/* unexpected end of transaction */
			I2C_CR1(dev) |= I2C_CR1_STOP; /* always STOP */
			return i;
		}
		I2C_DR(dev) = *(buf + i);
	}

	/* wait for last byte to drain */
	if (__i2c_event(chan, I2C_SR1_TxE | I2C_SR1_BTF)) {
		if (stop) {
			I2C_CR1(dev) |= I2C_CR1_STOP;
		}
		return size;
	}
	
	/* error on last byte so force stop */
	I2C_CR1(dev) |= I2C_CR1_STOP;
	return size - 1;
}

/*
 * The basic read bytes function.
 *
 * this function returns the number of bytes read 
 * if it is successful, 0 otherwise.
 *
 * Send the address with the low bit set (master receiver mode)
 * and then ack bytes until we get buf_size bytes.
 *
 * If the slave can cut us off early by sending NAK.
 *
 * If stop is True (!= 0) then a stop is also sent.
 *
 * If an error occurs, stop is sent regardless.
 */
int
i2c_read(i2c_dev *chan, uint8_t *buf, size_t size, int stop)
{
	uint32_t dev = __i2c_base[chan->i2c];
	uint8_t addr = chan->addr;

	/* doesn't support reading no bytes */
	if ((size == 0) || (buf == NULL)) {
		return -1;
	}

	I2C_CR1(dev) |= (I2C_CR1_START |  I2C_CR1_PE);
	if (!__i2c_event(chan, I2C_SR1_SB)) {
		return -1;
	}

	/* send address with bit 0 set (read) */
	I2C_DR(dev) = addr | 0x1;

	if (!__i2c_event(chan, I2C_SR1_ADDR)) {
		return -1;
	}

	if (size == 1) {
		/* don't ACK if we're reading just one byte */
		I2C_CR1(dev) &= ~(I2C_CR1_ACK);
	} else {
		I2C_CR1(dev) |= I2C_CR1_ACK;
	}
	I2C_SR2(dev); /* Clear ADDR flag */

	/* read bytes, turn off ACK near the end */
	for (size_t i = 0; i < size; i++) {
		if (!__i2c_event(chan, I2C_SR1_RxNE | I2C_SR1_BTF)) {
			/* early timeout, always stop */
			I2C_CR1(dev) |= I2C_CR1_STOP;
			return i;
		}
		if (i == (size - 1)) {
			/* nack after reading this byte */
			I2C_CR1(dev) &= ~(I2C_CR1_ACK);
		}
		*(buf + i) = I2C_DR(dev);
	}

	if (stop) {
		I2C_CR1(dev) |= I2C_CR1_STOP;
	}

	return size;
}

