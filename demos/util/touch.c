/*
 * touch.c -- interrupt driven touch screen driver
 * for the STM32F469I-DISCO
 * 
 * Copyright (c) 2020, Charles McManis, all rights reserved.
 */

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/syscfg.h>
#include "../util/util.h"


#define FT6206_DEVICE_ID	0xA8
#define FT6206_FIRMWARE		0xAF
#define FT6206_LIBV_LOW		0xA2
#define FT6206_LIBV_HIGH	0xA1
#define FT6206_STATE		0xBC
#define FT6206_THRESH		0x80
#define FT6206_REPORT		0xA4
#define FT6206_DELTA_UD		0x95
#define FT6206_DELTA_LR		0x94
#define FT6206_DELTA_Z		0x96


/* device structure with I2C channel and address for the touch device */
static i2c_dev *__touch_device;

/***
 * Touch controller ISR
 *
 * The touch controller INT line is tied to PJ5. The code uses
 * the EXTI peripheral to set up PJ5 to generate an interrupt
 * on it's falling edge.
 *
 * When the routine is entered, it resets the pending interrupt
 * and grabs all of the registers from the chip. This works because
 * the I2C code is polled and not interrupt driven. If you later decide
 * to make both use interrupts you must insure that the I2C channel has
 * higher priority (lower priority number) other wise when this code
 * talks to the device those interrupts won't fire and you'll just
 * wait forever in your code.
 *
 * State for touches is maintained in a touch_event structure. It is
 * double buffered so that if you're looking at the most recent
 * touch event, a new one coming in won't write over the data that you
 * are looking at.
 *
 * The flip flopping between the active buffer and the one in use is
 * done by the get_touch() function.
 *                                                                  ***/

static touch_event	touch_data[2];
static int 		touch_ndx = 0;			/* index into touch_data */

/*
 * "working" touch data
 * This pointer is used by the ISR to fill in the results of the
 * touch action. Before returning it to the caller of get_touch
 * get_touch swaps it out so the ISR starts using the "other"
 * touch data structure.
 */
static touch_event * volatile cur_touch = &touch_data[0];

/*
 * This interrupt it called when the touch device gets a touch
 */
void
exti9_5_isr(void)
{
	uint8_t buf[16];
	touch_event	*te = cur_touch;

	/* fetch the registers */
	buf[0] = 0;
	i2c_write(__touch_device, buf, 1, !SEND_I2C_STOP);
	i2c_read(__touch_device, buf, 16, SEND_I2C_STOP);

	/* extract touch points */
	for (int i = 0; i < 2; i++) {
		int tsx, tsy;
		int ndx = i*6 + 3;
		te->tp[i].evt = (buf[ndx] & 0xc0) >> 6;
		tsx = ((buf[ndx] & 0x3f) << 8) | (buf[ndx+1] & 0xff);
		te->tp[i].tid = (buf[ndx+2] & 0xf0) >> 4;
		tsy = ((buf[ndx+2] & 0xf) << 8) | (buf[ndx+3] & 0xff);
		te->tp[i].x = tsy;
		te->tp[i].y = 480 - tsx;
	}
	te->n = buf[2];

	/* acknowledge the interrupt */
	EXTI_PR = (1 << 5);
}

/*
 * get_touch( int wait_for_touch )
 *
 * Primary API to the touch panel controller.
 *
 * If you pass 0 to it, it returns immediately with NULL if there hasn't
 * been a touch. If there has been a touch, it swaps out the touch buffers
 * so you won't be processing a structure that the isr might be writing at
 * the same time and returns the "current" structure.
 */
touch_event *
get_touch(int wait)
{
	touch_event *nxt = &touch_data[(touch_ndx + 1) & 1];
	touch_event *res = cur_touch;

	/* not initialized or there was an error initializing it */
	if (__touch_device == NULL) {
		return NULL;
	}

	/* if n == 0, haven't seen any touching */
	if ((cur_touch->n == 0) && (wait == 0)) {
		return NULL;
	}
	/* ISR sets n as the last thing it does */
	while (cur_touch->n == 0) ;

	/* mark the next structure as 'unused' and swap */
	nxt->n = 0;
	cur_touch = nxt;
	
	/* return the current structure */
	return res;
}

/*
 * Initialize things for the touch device
 */
void
touch_init(uint8_t threshold)
{
	uint8_t	buf[2];

	/* initialize the i2c_device handle for the touch panel controller */
	__touch_device = i2c_init(1, I2C_400KHZ, 0x54);
	if (__touch_device == NULL) {
		return;
	}

	buf[0] = FT6206_DEVICE_ID;
	i2c_write(__touch_device, buf, 1 , !SEND_I2C_STOP);
	i2c_read(__touch_device, buf, 1, SEND_I2C_STOP);
	switch (buf[0]) {
		case 0x11:
			break;
		default:
			/* not the expected device */
			__touch_device = NULL;
			return;
	}

	/*** interrupt setup
	 *
	 *  Set up pin PJ5 as an input
	 *                                                             ***/
	rcc_periph_clock_enable(RCC_GPIOJ);
	gpio_mode_setup(GPIOJ, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5);

 	/* enable interrupts from PJ5 */
	rcc_periph_clock_enable(RCC_SYSCFG);
	/* set the source to Port J (0x9), pin 5 */
	SYSCFG_EXTICR2 = (0x9 << 4);

	/* enable interrupts */
	EXTI_IMR |= (1 << 5);
	/* interrupt on falling edge */
	EXTI_RTSR |= (1 << 5);
	EXTI_FTSR |= (1 << 5);
	/* Turn on interrupts */
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);

	/*
 	 * Sets the trigger threshold mid point? If it is "too touchy" you
 	 * can increase this number, if it is missing your touches then make
 	 * it lower.
 	 */
	buf[0] = FT6206_THRESH;
	buf[1] = threshold;
	i2c_write(__touch_device, buf, 2 , SEND_I2C_STOP);
	return;
}
