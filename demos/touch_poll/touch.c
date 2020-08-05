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

typedef enum {
	GEST_NONE,
	GEST_UP,
	GEST_DOWN,
	GEST_LEFT,
	GEST_RIGHT,
	GEST_ZOOM_IN,
	GEST_ZOOM_OUT,
	GEST_UNKNOWN
} touch_gesture;

#define STOP	1
/*
 * A simple touch structure
 */
struct touch_t {
	int	evt;
	int tid;
	int x, y;
	int weight, misc;
};

int read_reg(i2c_dev *, uint8_t reg);
int write_reg(i2c_dev *, uint8_t reg, uint8_t val);
void print_touch(struct touch_t *t, char * label);

void
print_touch(struct touch_t *t, char *label) {
	printf("%10s: X:%3d\tY:%3d\t[Evt, Tid]=[%d, %d]\n",
		label, t->x, t->y, t->evt, t->tid);
	printf("\tWeight: %d, Area %d\n", t->weight, t->misc);
}

i2c_dev *touch_device;
struct touch_data_t {
	struct touch_t tp[2];
	touch_gesture	g;
	int				n;
} touch_data[2];
int touch_ndx = 0;						/* index into touch_data */

/* "working" touch data */
struct touch_data_t * volatile cur_touch = &touch_data[0];

volatile int touch_detects = 0;
volatile int gesture_detects = 0;

/*
 * This interrupt it called when the touch device gets a touch
 *
 * Need to double buffer the touch points so that we don't get them
 * overwritten while the program is using them. So copies of
 * two touch_t's and touch_count. 
 *
 * get_touch returns a pointer and swaps pages. Then the 'active page'
 * keeps being overwritten until it is read and then the pages get swapped.
 */
void
exti9_5_isr(void)
{
	uint8_t buf[16];

	EXTI_PR = (1 << 5);
	buf[0] = 0;
	i2c_write(touch_device, buf, 1, 0);
	i2c_read(touch_device, buf, 16, 1);

	/* extract touch points */
	for (int i = 0; i < 2; i++) {
		int ndx = i*6 + 3;
		cur_touch->tp[i].evt = (buf[ndx] & 0xc0) >> 6;
		cur_touch->tp[i].x = ((buf[ndx] & 0x3f) << 8) | (buf[ndx+1] & 0xff);
		cur_touch->tp[i].tid = (buf[ndx+2] & 0xf0) >> 4;
		cur_touch->tp[i].y = ((buf[ndx+2] & 0xf) << 8) | (buf[ndx+3] & 0xff);
	}
	switch(buf[1]) {
		case 0:
			cur_touch->g = GEST_NONE;
			break; 
		case 0x10:
			cur_touch->g = GEST_UP;
			break;
		case 0x14:
			cur_touch->g = GEST_RIGHT;
			break;
		case 0x18:
			cur_touch->g = GEST_DOWN;
			break;
		case 0x1c:
			cur_touch->g = GEST_LEFT;
			break;
		case 0x48:
			cur_touch->g = GEST_ZOOM_IN;
			break;
		case 0x49:
			cur_touch->g = GEST_ZOOM_OUT;
			break;
		default:
			cur_touch->g = GEST_UNKNOWN;
			break;
	}
	cur_touch->n = buf[2];
	if (cur_touch->g != GEST_NONE) {
		gesture_detects++;
	}
	touch_detects++;
}

struct touch_data_t  *get_touch(int wait);

/*
 * Check the current touch structure, see if it has seen a touch.
 * If so, swap touch structures and return the current one.
 */
struct touch_data_t *
get_touch(int wait)
{
	struct touch_data_t *nxt = &touch_data[(touch_ndx + 1) & 1];
	struct touch_data_t *res = cur_touch;

	/* if n == 0, haven't seen any touching */
	if ((cur_touch->n == 0) && (wait == 0)) {
		return NULL;
	}
	while (cur_touch->n == 0) ;
	nxt->n = 0;
	cur_touch = nxt;
	return res;
}

/*
 * A helper function that reads the value from a register on the
 * touch controller chip.
 */
int
read_reg(i2c_dev *i2c, uint8_t reg)
{
	uint8_t buf[4];

	buf[0] = reg;
	if (i2c_write(i2c, buf, 1 , !STOP) < 0) {
		return -1;
	}
	if (i2c_read(i2c, buf, 1, STOP) < 0) {
		return -2;
	}
	return (int) buf[0];
}

/*
 * A helper function that writes a value to a register on the 
 * touch controller chip.
 */
int
write_reg(i2c_dev *i2c, uint8_t reg, uint8_t value)
{
	uint8_t buf[4];

	buf[0] = reg;
	buf[1] = value;
	if (i2c_write(i2c, buf, 2 , !STOP) < 0) {
		return -1;
	}
	return 1;
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
	int		count;
	char	*chip_id;

	printf("\033[1;1H\033[J\n");
	fprintf(stderr, "I2C Example Code\n");

	rcc_periph_clock_enable(RCC_GPIOJ);
	gpio_mode_setup(GPIOJ, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5);

	/* enable interrupts from PJ5 */
	rcc_periph_clock_enable(RCC_SYSCFG);
	/* set the source to Port J (0x9), pin 5 */
	SYSCFG_EXTICR2 = (0x9 << 4);

	printf("SYSCFG_EXTICR2 @0x%0x = %0lx\n", (SYSCFG_BASE + 0xc), 
						SYSCFG_EXTICR2);

	/* enable interrupts */
	EXTI_IMR |= (1 << 5);
	/* interrupt on falling edge */
	EXTI_RTSR |= (1 << 5);
	EXTI_FTSR |= (1 << 5);
	/* Turn on interrupts */
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);

	if (EXTI_PR & (1 << 5)) {
		printf("Reset pending interrupt\n");
		EXTI_PR = (1 << 5);
	}

	/*
 	 * send a test interrupt (this crashes the code?)
 	 */
//	EXTI_SWIER = (1 << 5);

//	printf("\033[1;1H\033[J\n");
//	fprintf(stderr, "I2C Example Code\n");
	touch_device = i2c_init(1, I2C_400KHZ, 0x54);
	if (touch_device == NULL) {
		fprintf(stderr, "Unable to initialize i2c\n");
		while (1);
	}
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
	printf("Chip ID returned: 0x%0x (%s)\n", res, chip_id);
	res = read_reg(touch_device, FT6206_FIRMWARE);
	if (res < 0) {
		printf("    Firmware: <read error>\n");
	} else {
		printf("    Firmware: 0x%02x\n", res);
	}
	lib[0] = read_reg(touch_device, FT6206_LIBV_LOW);
	lib[1] = read_reg(touch_device, FT6206_LIBV_HIGH);
	if ((lib[0] < 0) || (lib[1] < 0)) {
		printf("    Library: <read error>\n");
	} else {
		printf("    Library: %d.%d\n", lib[0], lib[1]);
	}
	res = read_reg(touch_device, FT6206_STATE);
	if (res < 0) {
		printf("    State: <read error>\n");
	} else {
		printf("    State: 0x%02x\n", res);
	}
	res = read_reg(touch_device, FT6206_REPORT);
	printf("     Mode: %s\n", (res == 0) ? "Polling" : "Trigger");
	/* This effectively kicks it off */
	write_reg(touch_device, FT6206_THRESH, 0x80);
	
	count = 0;
	while (1) {
		struct touch_data_t *td;
#if 0
		uint8_t buf[16];
#endif

		td = get_touch(1);
		count++;
		printf("\033[8;1H\033[J\n");
		printf("\nTotal touch events : %d\n\n", touch_detects);
		printf("\nTotal gesture events : %d\n\n", gesture_detects);
#if 0
		printf("Chip registers:\n");
		buf[0] = 0;
		i2c_write(touch_device, buf, 1, 0);
		i2c_read(touch_device, buf, 16, 1);
		/* extract touch points */
		for (int i = 0; i < 2; i++) {
			int ndx = i*6 + 3;
			t[i].evt = (buf[ndx] & 0xc0) >> 6;
			t[i].x = ((buf[ndx] & 0x3f) << 8) | (buf[ndx+1] & 0xff);
			t[i].tid = (buf[ndx+2] & 0xf0) >> 4;
			t[i].y = ((buf[ndx+2] & 0xf) << 8) | (buf[ndx+3] & 0xff);
			t[i].weight = buf[ndx+4] & 0xff;
			t[i].misc = buf[ndx+5] & 0xf0 >> 4;
		}
		printf("\033[J\n");
#endif
		print_touch(&(td->tp[0]), "Touch 1");
		if ((td->n & 0x3) == 2) {
			print_touch(&(td->tp[1]), "Touch 2");
		} else {
			printf("\033[J\n");
		}
	}
}
