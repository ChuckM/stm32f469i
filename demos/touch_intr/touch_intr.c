/*
 * Touchscreen example code
 *
 * This demo uses the touch controller on the 469i
 * Discovery board to demonstrate i2c programming.
 *
 * The controller is a Focal Tech FT6206GMA and
 * it is connected to pins PB8 and PB9 (SCL, SDA)
 * and it connects an interrupt line to PJ5. I2C Address 0x54 (decimal 84)
 *
 * This variant uses the EXTI peripheral and sets up interrupts
 * from PB9 to notify the program that a touch has been detected.
 *
 * Copyright (C) 2016 - 2020, Chuck McManis, all rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/syscfg.h>
#include <gfx.h>

#include <malloc.h>
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

#define SEND_I2C_STOP	1

/* device structure with I2C channel and address for the touch device */
i2c_dev *touch_device;

/*
 * Tracks how many events and gestures the code processes
 */
volatile int touch_detects = 0;
volatile int gesture_detects = 0;

static void
draw_pixel(void *buf, int x, int y, GFX_COLOR c)
{
	lcd_draw_pixel(buf, x, y, c.raw);
}

/*
 * relocate the heap to the DRAM, 10MB at 0xC0000000
 */
void
local_heap_setup(uint8_t **start, uint8_t **end)
{
	console_puts("Local heap setup\n");
	*start = (uint8_t *)(0xc0000000);
	*end = (uint8_t *)(0xc0000000 + (10 * 1024 * 1024));
}

/****
 *  local helper functions that make the code a bit cleaner
 *                                                      ****/
int read_reg(i2c_dev *, uint8_t reg);
int write_reg(i2c_dev *, uint8_t reg, uint8_t val);
void get_touch_data(i2c_dev *, touch_event *res);
void print_touch(touch_point *t, char * label);

/*
 * Snatch a touch from the device.
 */
void
get_touch_data(i2c_dev *t, touch_event *te)
{
	uint8_t buf[16];

	/* fetch the registers */
	buf[0] = 0;
	i2c_write(t, buf, 1, !SEND_I2C_STOP);
	i2c_read(t, buf, 16, SEND_I2C_STOP);

	/* extract touch points */
	for (int i = 0; i < 2; i++) {
		int ndx = i*6 + 3;
		te->tp[i].evt = (buf[ndx] & 0xc0) >> 6;
		te->tp[i].x = ((buf[ndx] & 0x3f) << 8) | (buf[ndx+1] & 0xff);
		te->tp[i].tid = (buf[ndx+2] & 0xf0) >> 4;
		te->tp[i].y = ((buf[ndx+2] & 0xf) << 8) | (buf[ndx+3] & 0xff);
	}
	switch(buf[1]) {
		case 0:
			te->g = GEST_NONE;
			break;
		case 0x10:
			te->g = GEST_UP;
			break;
		case 0x14:
			te->g = GEST_RIGHT;
			break;
		case 0x18:
			te->g = GEST_DOWN;
			break;
		case 0x1c:
			te->g = GEST_LEFT;
			break;
		case 0x48:
			te->g = GEST_ZOOM_IN;
			break;
		case 0x49:
			te->g = GEST_ZOOM_OUT;
			break;
		default:
			te->g = GEST_UNKNOWN;
			break;
	}
	te->n = buf[2];

	/* statistics accounting */
	if (te->g != GEST_NONE) {
		gesture_detects++;
	}
	touch_detects++;
}

/*
 * Read and return the value from a single register on the
 * touch controller chip.
 */
int
read_reg(i2c_dev *i2c, uint8_t reg)
{
	uint8_t buf[4];

	buf[0] = reg;
	if (i2c_write(i2c, buf, 1 , !SEND_I2C_STOP) < 0) {
		return -1;
	}
	if (i2c_read(i2c, buf, 1, SEND_I2C_STOP) < 0) {
		return -2;
	}
	return (int) buf[0];
}

/*
 * Write a value to a single register on the
 * touch controller chip.
 */
int
write_reg(i2c_dev *i2c, uint8_t reg, uint8_t value)
{
	uint8_t buf[4];

	buf[0] = reg;
	buf[1] = value;
	if (i2c_write(i2c, buf, 2 , SEND_I2C_STOP) < 0) {
		return -1;
	}
	return 1;
}

/*
 * Function that prints out the contents of a touch point
 * structure.
 */
void
print_touch(touch_point *t, char *label) {
	printf("%10s: X:%3d\tY:%3d\t[Evt, Tid]=[%d, %d]\n",
		label, t->x, t->y, t->evt, t->tid);
	printf("\tWeight: %d, Area %d\n", t->weight, t->misc);
}


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

touch_event	touch_data[2];
int 		touch_ndx = 0;			/* index into touch_data */

/*
 * "working" touch data
 * This pointer is used by the ISR to fill in the results of the
 * touch action. Before returning it to the caller of get_touch
 * get_touch swaps it out so the ISR starts using the "other"
 * touch data structure.
 */
touch_event * volatile cur_touch = &touch_data[0];

/*
 * This interrupt it called when the touch device gets a touch
 */
void
exti9_5_isr(void)
{
	get_touch_data(touch_device, cur_touch);

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
 * This demo demonstrates the use of the FocalTech
 * FT6206 Touch Controller that is part of the display
 * on this board.
 *
 * The controller is connected to pins PB8 and PB9 (SCL, SDA)
 * and it connects an interrupt line to PJ5.
 *
 * The demo displays a target where you touch the display and
 * provides additional information to the serial port at
 * 57600 baud.
 */
int
main(void)
{
	int		res, lib[2];
	char	*chip_id;
	GFX_CTX local_context;
	GFX_CTX *g;

	printf("\033[1;1H\033[J\n");
	fprintf(stderr, "I2C Example Code (interrupt driven)\n");

	/* initialize the i2c_device handle for the touch panel controller */
	touch_device = i2c_init(1, I2C_400KHZ, 0x54);
	if (touch_device == NULL) {
		fprintf(stderr, "Unable to initialize i2c\n");
		while (1);
	}

	/*** interrupt version
	 *
	 * In the interrupt version we have to do a bit more work and turn
	 * this GPIO pin into a source of interrupts. We do that with the
	 * EXTI controller's help.
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

	/*** Common code
	 *
	 * From here down, the code is the same in both the polling and
	 * interrupt versions.
	 *
	 * Start with a basic sanity check, make sure we see the FT6x06
	 * device and print out other version information.
	 *                                                             ***/
	printf("Reading device ID\n");
	res = read_reg(touch_device, FT6206_DEVICE_ID);
	switch (res) {
		case 0x11:
			chip_id = "FT6206";
			break;
		case 0xCD:
			chip_id = "FT6x36";
			break;
		default:
			chip_id = "UNKNOWN";
			break;
	}
	printf("   Chip ID : 0x%0x (%s)\n", res, chip_id);

	res = read_reg(touch_device, FT6206_FIRMWARE);
	printf("  Firmware : 0x%02x\n", res);

	lib[0] = read_reg(touch_device, FT6206_LIBV_LOW);
	lib[1] = read_reg(touch_device, FT6206_LIBV_HIGH);
	printf("   Library : %d.%d\n", lib[0], lib[1]);

	res = read_reg(touch_device, FT6206_STATE);
	printf("     State : 0x%02x\n", res);

	res = read_reg(touch_device, FT6206_REPORT);
	printf("      Mode : %s\n", (res == 0) ? "Polling" : "Trigger");

	/*
 	 * This effectively kicks it off. Prior to setting this threshold
 	 * is 0xFF (reset value) which basically turns off touch detection.
 	 */
	write_reg(touch_device, FT6206_THRESH, 0x80);
	
	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);

	g = gfx_init(&local_context, draw_pixel, 800, 480, GFX_FONT_LARGE, 
						(void *)FRAMEBUFFER_ADDRESS);
	gfx_move_to(g, 15, 15);
	gfx_set_text_color(g,GFX_COLOR_YELLOW, GFX_COLOR_BLACK);
	gfx_puts(g, "Touch Controller Demo");
	lcd_flip(0);

	while (1) {
		touch_event *td;

		/* Get a touch event, spin wait if no touching */
		td = get_touch(1);

		gfx_draw_point_at(g, td->tp[0].y, 480 - td->tp[0].x, GFX_COLOR_CYAN);
		gfx_draw_point_at(g, td->tp[1].y, 480 - td->tp[1].x, GFX_COLOR_YELLOW);
		lcd_flip(0);

		/* move cursor to start of data area, clear to end of screen */
		printf("\033[8;1H\033[J\n");
		printf("  Total touch events : %d\n", touch_detects);
		printf("Total gesture events : %d\n\n", gesture_detects);
		/* This clears the previous second touch printout */
		printf("\033[J\n");
		print_touch(&(td->tp[0]), "Touch 1");
		/* can only do "up to two" touches so this is either 1 or 2 */
		if (td->n > 1) {
			print_touch(&(td->tp[1]), "Touch 2");
		}
	}
}
