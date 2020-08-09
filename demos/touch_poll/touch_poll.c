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
 * This variant polls GPIO PJ5 to see if there is currently a touch going
 * on and the data should be fetched.
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

#define SEND_I2C_STOP	1

/* device structure with I2C channel and address for the touch device */
i2c_dev *touch_device;

/*
 * Tracks how many events the code processes
 */
volatile int touch_detects = 0;

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
	te->n = buf[2];

	/* statistics accounting */
	touch_detects++;
}

/*
 * Read and return the value from a single register on the
 * touch controller chip.
 * NB: They assume success, it's only an example
 */
int
read_reg(i2c_dev *i2c, uint8_t reg)
{
	uint8_t buf[4];

	buf[0] = reg;
	i2c_write(i2c, buf, 1 , !SEND_I2C_STOP); 
	i2c_read(i2c, buf, 1, SEND_I2C_STOP);
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
	i2c_write(i2c, buf, 2 , SEND_I2C_STOP);
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

/*
 * get_touch( int wait_for_touch )
 *
 * Primary API to the touch panel controller.
 *
 * If you pass 0 to it, it returns immediately with NULL if there hasn't
 * been a touch. If there has been a touch, it fetches the touch controllers
 * registers and puts them into a static structure it is holding and returns
 * a pointer to that structure.
 */
touch_event *
get_touch(int wait)
{
	static touch_event res;

	/* if n == 0, don't see a touch */
	if ((gpio_get(GPIOJ, GPIO5)) && (wait == 0)) {
		return NULL;
	}
	while (gpio_get(GPIOJ, GPIO5)) ;

	/* fetch the registers */
	get_touch_data(touch_device, &res);

	return &res;
}

/*
 * This demo demonstrates the use of the FocalTech
 * FT6206 Touch Controller that is part of the display
 * on this board.
 *
 * The controller is connected to pins PB8 and PB9 (SCL, SDA)
 * and it connects an interrupt line to PJ5.
 *
 * The example writes out touch data to the serial port at 57600 baud.
 */
int
main(void)
{
	int		res, lib[2];
	char	*chip_id;

	printf("\033[1;1H\033[J\n");
	fprintf(stderr, "I2C Example Code (polling)\n");

	/* initialize the i2c_device handle for the touch panel controller */
	touch_device = i2c_init(1, I2C_400KHZ, 0x54);
	if (touch_device == NULL) {
		fprintf(stderr, "Unable to initialize i2c\n");
		while (1);
	}

	/*** polling version
	 *
	 * In the polling version we just set this to an input and we're
	 * done
	 *
	 *  Set up pin PJ5 as an input
	 *                                                             ***/
	rcc_periph_clock_enable(RCC_GPIOJ);
	gpio_mode_setup(GPIOJ, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5);

	/***
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
 	 * Set the threshold somewhere. If its too "touchy" to you can
 	 * increase this number.
 	 */
	write_reg(touch_device, FT6206_THRESH, 0x80);
	
	while (1) {
		touch_event *td;

		/* Get a touch event, spin wait if no touching */
		td = get_touch(1);

		/* move cursor to start of data area, clear to end of screen */
		printf("\033[8;1H\033[J\n");
		printf("  Total touch events : %d\n", touch_detects);
		/* This clears the previous second touch printout */
		printf("\033[J\n");
		print_touch(&(td->tp[0]), "Touch 1");
		/* can only do "up to two" touches so this is either 1 or 2 */
		if (td->n > 1) {
			print_touch(&(td->tp[1]), "Touch 2");
		}
	}
}
