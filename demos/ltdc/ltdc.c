/*
 * This code exercises the LTDC module.
 *
 * Copyright (c) 2016, Chuck McManis (cmcmanis@mcmanis.com)
 */

#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/ltdc.h>
#include <libopencm3/stm32/dsi.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scb.h>
#include <gfx.h>
#include "../util/util.h"

#define FB_ADDRESS	0xc0000000

void draw_pixel(int x, int y, uint16_t color);

uint32_t *current_buf = (uint32_t *)(0xc0000000);

/*
 * Mapping a 16 bit color to a 24 bit color
 * 15   14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
 * [R4][R3][R2][R1][R0][G5][G4][G3][G2][G1][G0][B4][B3][B2][B1][B0]
 *
 *  23  22  21  20  19  18  17  16  15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
 * [R4][R3][R2][R1][R0][F1][F2][F3][G5][G4][G3][G2][G1][G0][F1][F2][B4][B3][B2][B1][B0][F1][F2][F3] 
 *  0   0   0   0   0   1   1   1   0   0   0   0   0   0   1   1   0   0   0   0   0   1   1   1
 */
void
draw_pixel(int x, int y, uint16_t color)
{
	int	r, g, b;

	uint32_t *ptr = (uint32_t *)(FB_ADDRESS);
	uint32_t pixel = 0xff070307;
	r = ((color >> 11) & 0x1f) << 3;
	g = ((color >> 5) & 0x3f) << 2;
	b = (color & 0x1f) << 3;
	pixel = 0xff000000 | (r << 16) | (g << 8) | b;
	*(ptr + y * 800 + x) = pixel;
}

/*
 * A couple of helper macros to avoid copy pasta code
 * For multi-bit fields we can use these macros to create
 * a unified SET_<reg>_<field> version and a GET_<reg_<field> version.
 */
#define LTDC_GET(reg, field, x) \
    (((x) >> LTDC_ ## reg ## _ ## field ##_SHIFT) & LTDC_ ## reg ##_ ## field ##_MASK)

#define LTDC_SET(reg, field, x) \
    (((x) & LTDC_ ## reg ##_## field ##_MASK) << LTDC_## reg ##_## field ##_SHIFT)

#define LTDC_MASK(reg, field) \
    ((LTDC_ ## reg ##_## field ##_MASK) << LTDC_## reg ##_## field ##_SHIFT)


void lcd_init(uint8_t *framebuffer);

#define LCD_FORMAT_LANDSCAPE

/*
 * This is the series of commands sent to the
 * LCD, it is extracted programmatically from the
 * source code that initializes the display in the
 * ST Micro examples. It has been modified in two
 * ways, there were two if statements, those have
 * been changed to #defines (LANDSCAPE and PIXELFORMAT)
 * secondly the "long" strings (more than 2) have
 * had the last byte in the original array moved
 * to the front. That makes our command sending function
 * easier as it doesn't have to special case the first
 * byte like ST does in their DSI_IO_WriteCmd function.
 */
const uint8_t __lcd_init_data[] = {
	2, 0x00, 0x00, /* S: ShortRegData1 */
	4, 0xFF, 0x80, 0x09, 0x01, /* L: lcdRegData1 */
	2, 0x00, 0x80, /* S: ShortRegData2 */
	3, 0xFF, 0x80, 0x09, /* L: lcdRegData2 */
	2, 0x00, 0x80, /* S: ShortRegData2 */
	2, 0xC4, 0x30, /* S: ShortRegData3 */
	1, 10, /* D: 10 */
	2, 0x00, 0x8A, /* S: ShortRegData4 */
	2, 0xC4, 0x40, /* S: ShortRegData5 */
	1, 10, /* D: 10 */
	2, 0x00, 0xB1, /* S: ShortRegData6 */
	2, 0xC5, 0xA9, /* S: ShortRegData7 */
	2, 0x00, 0x91, /* S: ShortRegData8 */
	2, 0xC5, 0x34, /* S: ShortRegData9 */
	2, 0x00, 0xB4, /* S: ShortRegData10 */
	2, 0xC0, 0x50, /* S: ShortRegData11 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	2, 0xD9, 0x4E, /* S: ShortRegData12 */
	2, 0x00, 0x81, /* S: ShortRegData13 */
	2, 0xC1, 0x66, /* S: ShortRegData14 */
	2, 0x00, 0xA1, /* S: ShortRegData15 */
	2, 0xC1, 0x08, /* S: ShortRegData16 */
	2, 0x00, 0x92, /* S: ShortRegData17 */
	2, 0xC5, 0x01, /* S: ShortRegData18 */
	2, 0x00, 0x95, /* S: ShortRegData19 */
	2, 0xC5, 0x34, /* S: ShortRegData9 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	3, 0xD8, 0x79, 0x79, /* L: lcdRegData5 */
	2, 0x00, 0x94, /* S: ShortRegData20 */
	2, 0xC5, 0x33, /* S: ShortRegData21 */
	2, 0x00, 0xA3, /* S: ShortRegData22 */
	2, 0xC0, 0x1B, /* S: ShortRegData23 */
	2, 0x00, 0x82, /* S: ShortRegData24 */
	2, 0xC5, 0x83, /* S: ShortRegData25 */
	2, 0x00, 0x81, /* S: ShortRegData13 */
	2, 0xC4, 0x83, /* S: ShortRegData26 */
	2, 0x00, 0xA1, /* S: ShortRegData15 */
	2, 0xC1, 0x0E, /* S: ShortRegData27 */
	2, 0x00, 0xA6, /* S: ShortRegData28 */
	3, 0xB3, 0x00, 0x01, /* L: lcdRegData6 */
	2, 0x00, 0x80, /* S: ShortRegData2 */
	7, 0xCE, 0x85, 0x01, 0x00, 0x84, 0x01, 0x00, /* L: lcdRegData7 */
	2, 0x00, 0xA0, /* S: ShortRegData29 */
	15, 0xCE, 0x18, 0x04, 0x03, 0x39, 0x00, 0x00, 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00, /* L: lcdRegData8 */
	2, 0x00, 0xB0, /* S: ShortRegData30 */
	15, 0xCE, 0x18, 0x02, 0x03, 0x3B, 0x00, 0x00, 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00, /* L: lcdRegData9 */
	2, 0x00, 0xC0, /* S: ShortRegData31 */
	11, 0xCF, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, /* L: lcdRegData10 */
	2, 0x00, 0xD0, /* S: ShortRegData32 */
	2, 0xCF, 0x00, /* S: ShortRegData46 */
	2, 0x00, 0x80, /* S: ShortRegData2 */
	11, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData11 */
	2, 0x00, 0x90, /* S: ShortRegData33 */
	16, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData12 */
	2, 0x00, 0xA0, /* S: ShortRegData29 */
	16, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData13 */
	2, 0x00, 0xB0, /* S: ShortRegData30 */
	11, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData14 */
	2, 0x00, 0xC0, /* S: ShortRegData31 */
	16, 0xCB, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData15 */
	2, 0x00, 0xD0, /* S: ShortRegData32 */
	16, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData16 */
	2, 0x00, 0xE0, /* S: ShortRegData34 */
	11, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData17 */
	2, 0x00, 0xF0, /* S: ShortRegData35 */
	11, 0xCB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* L: lcdRegData18 */
	2, 0x00, 0x80, /* S: ShortRegData2 */
	11, 0xCC, 0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData19 */
	2, 0x00, 0x90, /* S: ShortRegData33 */
	16, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02, /* L: lcdRegData20 */
	2, 0x00, 0xA0, /* S: ShortRegData29 */
	16, 0xCC, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData21 */
	2, 0x00, 0xB0, /* S: ShortRegData30 */
	11, 0xCC, 0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData22 */
	2, 0x00, 0xC0, /* S: ShortRegData31 */
	16, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01, /* L: lcdRegData23 */
	2, 0x00, 0xD0, /* S: ShortRegData32 */
	16, 0xCC, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* L: lcdRegData24 */
	2, 0x00, 0x81, /* S: ShortRegData13 */
	2, 0xC5, 0x66, /* S: ShortRegData47 */
	2, 0x00, 0xB6, /* S: ShortRegData48 */
	2, 0xF5, 0x06, /* S: ShortRegData49 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	4, 0xFF, 0xFF, 0xFF, 0xFF, /* L: lcdRegData25 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	17, 0xE1, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01, /* L: lcdRegData3 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	17, 0xE2, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01, /* L: lcdRegData4 */
	2, 0x11, 0x00, /* S: ShortRegData36 */
	1, 120, /* D: 120 */
#ifdef LCD_COLOR_FORMAT_RGB565
	2, 0x3A, 0x55, /* S: ShortRegData37 */
#else
	2, 0x3A, 0x77, /* S: ShortRegData38 */
#endif

/*
 * Debugging original is this: 2, 0x36, 0x60, S: ShortRegData39 
 */
#ifdef LCD_FORMAT_LANDSCAPE

	2, 0x36, 0x60, /* S: ShortRegData39 */
	5, 0x2A, 0x00, 0x00, 0x03, 0x1F, /* L: lcdRegData27 */
	5, 0x2B, 0x00, 0x00, 0x01, 0xDF, /* L: lcdRegData28 */
#endif
	2, 0x51, 0x7F, /* S: ShortRegData40 */
	2, 0x53, 0x2C, /* S: ShortRegData41 */
	2, 0x55, 0x02, /* S: ShortRegData42 */
	2, 0x5E, 0xFF, /* S: ShortRegData43 */
	2, 0x29, 0x00, /* S: ShortRegData44 */
	2, 0x00, 0x00, /* S: ShortRegData1 */
	2, 0x2C, 0x00, /* S: ShortRegData45 */
/* Insert end of list */
	0
};
void send_command(uint8_t n_arg, const uint8_t *args);
void init_display(void);

#define UINT	(unsigned int)

/*
 * This sends a command through the DSI to the
 * display, either it has 2 parameters which
 * can be sent with a single write, or it
 * has more than 2 in which case we push
 * them into the FIFO and then send a "long"
 * write with the number of args.
 *
 * If length is 1 it is a special case
 * we just delay for *args milliseconds.
 */
void
send_command(uint8_t n_arg, const uint8_t *args)
{
	int i;
	uint32_t tmp;
	char cmd;
	switch (n_arg) {
		case 0:
			cmd = 'E';
			break;
		case 1:
			cmd = 'D';
			break;
		case 2:
			cmd = 'S';
			break;
		default:
			cmd = 'L';
	}

#ifdef DEBUG_COMMANDS
	printf("    CMD: %c: ", cmd);
	for (i = 0; i < n_arg-1; i++) {
		printf("%02X ", (unsigned int) *(args + i));
	}
	printf("%02X\n", (unsigned int) *(args + n_arg - 1));
#endif

	/* Not a "real" command, it's a request to delay */
	if (n_arg == 1) {
		msleep(*args);
		return;
	}

	/* wait for previous command to drain */
	while ((DSI_GPSR & DSI_GPSR_CMDFE) == 0) ;
	
	/* if we only have two parameters, send a short write */
	/* ### SWAP MSB/LSB */
	if (n_arg == 2) {
		tmp = DSI_SET(GHCR, DT, 0x15) | DSI_SET(GHCR, DATA0, *args) |
				   DSI_SET(GHCR, DATA1, *(args+1));
#ifdef DEBUG_WRITES
		printf("        ... 0x%08X (me) ... ", UINT tmp);
		/* ## This is an exact copy of the code in DSI_ConfigPacketHeader */
		tmp = 0x15 | (*args) << 8 | (*(args + 1) << 16);
#endif
#ifdef DEBUG_COMMANDS
		printf("        ... GHCR: 0x%08X\n", UINT tmp);
#endif
		DSI_GHCR = tmp;
	} else {
		i = 0;
		while (i < n_arg ) {
			tmp = *(args + i++);
			if (i < n_arg) {
				tmp |= (*(args + i++) << 8);
			}
			if (i < n_arg) {
				tmp |= (*(args + i++) << 16);
			}
			if (i < n_arg) {
				tmp |= (*(args + i++) << 24);
			}
			DSI_GPDR = tmp;
#ifdef DEBUG_COMMANDS
			printf("        ... GPDR: 0x%08X\n", UINT tmp);
#endif
		}
		/* we assume that n_arg is always <= 255 */
		tmp = DSI_SET(GHCR, DT, 0x39) | DSI_SET(GHCR, WCLSB, n_arg);
#ifdef DEBUG_WRITES
		printf("        ... 0x%08X (me) ... ", UINT tmp);
		/* Duplicating ST code */
		tmp = 0x39 | n_arg << 8 | ((0) << 16);
#endif
#ifdef DEBUG_COMMANDS
		printf("        ... GHCR: 0x%08X \n", UINT tmp);
#endif
		DSI_GHCR = tmp;
	}
}

/*
 * Implements a really simple state engine to drive the initialization
 * sequence. I've collected all of the commands that are sent (combination
 * of the data sheet and the ST examples) and put them into an array
 * of the form 
 *		[length], [data0], ... [datan],
 * Since every command has at least two bytes, if length is 1 then
 * that means delay [data0] milliseconds. If length is 0 it means you
 * are at the end of the array and to stop.
 *
 * The array was generated with some perl code, trying to edit it manually
 * will probably bite you big time.
 */
void
init_display(void)
{
	const uint8_t *parms;
	parms = &__lcd_init_data[0];
	while (*parms != 0) {
		send_command(*parms, parms+1);
		parms += (*parms) + 1;
	}
}


/*
 * PD11 - controller reset
 * PH7 - xres (reset) on the LCD
 * PF5 - Backlight control (on/off)
 * PG3 - (input) "tearing effect"
 */
static void
gpio_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOH);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(GPIOH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
	gpio_set_output_options(GPIOH, GPIO_OTYPE_OD, GPIO_OSPEED_100MHZ, GPIO7);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);
	gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_mode_setup(GPIOG, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO3);
}

/*
 * Hsync and Vsync have 1 pre-subtracted because the values in the
 * registers are all 0 based and expect them to be one less than
 * the totals.
 */
#define MY_HSYNC	2	/* actually hsync-1 */
#define MY_HFP		1
#define MY_HBP		1
#define MY_HACT		800

#define MY_VSYNC	2	/* actually vsync-1 */
#define MY_VFP		1
#define MY_VBP		1
#define MY_VACT		480

void ltdc_layer_setup(uint8_t *fb);
/*
 * This should configure the LTDC controller to to display an 800 x 480 x 4 (ARGB8888)
 * region of memory, which if it goes as planned, will show up as the display contents.
 */
void 
ltdc_layer_setup(uint8_t *fb)
{
	/*
	###
    const int vsa = 2;
    const int vbp = 1;
    const int vfp = 1;
    const int hsa = 2;
    const int hbp = 1;
    const int hfp = 1;
    const int hact = 800;
    const int vact = 480;
 	*/

#if 0
	/* not sure this works */
	ltdc_setup_windowing(1, 1, 1, 800, 480);
#endif
	/* manual setup of the layer */
	LTDC_L1WHPCR = LTDC_SET(LxWHPCR, WHSTPOS, (MY_HSYNC + MY_HBP)) | 
				   LTDC_SET(LxWHPCR, WHSPPOS, (MY_HSYNC + MY_HBP + MY_HACT - 1));
	LTDC_L1WVPCR = LTDC_SET(LxWVPCR, WVSTPOS, (MY_VSYNC + MY_VBP)) |
			       LTDC_SET(LxWVPCR, WVSPPOS, (MY_VSYNC + MY_VBP + MY_VACT - 1));
	LTDC_L1PFCR = 0; // ARGB8888
	LTDC_L1CACR = 0xff; // Constant Alpha 0xff
	LTDC_L1DCCR = 0x00ff0000; // green as the default layer 1 color
	LTDC_L1BFCR = (0x6 << 8) | 0x7;
	LTDC_L1CFBAR = (uint32_t) fb;
	LTDC_L1CFBLR = LTDC_SET(LxCFBLR, CFBP, (800 * 4)) | LTDC_SET(LxCFBLR, CFBLL, (800 * 4) + 3);
	LTDC_L1CFBLNR = 480;
	LTDC_L1CR = LTDC_LxCR_LEN; /* no color key, no lookup table */

	//LTDC_L2CR = 0;

	/* immediate load from shadow */
	LTDC_SRCR = LTDC_SRCR_IMR;
}

/*
 * This function initializes the display and clears it
 */
void
lcd_init(uint8_t *fb)
{
	uint32_t tmp;
	uint16_t	n, q, r, p;

	printf("Reset LCD ... ");
	fflush(stdout);
	gpio_clear(GPIOH, GPIO7);
	msleep(20);
	gpio_set(GPIOH, GPIO7);
	msleep(10);
	printf("done.\n");
	printf("Set up PLLSAI ... ");
	fflush(stdout);
	/* set up the PLLSAI clock for the DSI peripheral */
	/* Revisit this, why not set it for 384Mhz VCO so that PLLSAI48 can be correct? */
	tmp = RCC_PLLSAICFGR;
	p = (tmp >> RCC_PLLSAICFGR_PLLSAIP_SHIFT) & RCC_PLLSAICFGR_PLLSAIP_MASK;
	q = (tmp >> RCC_PLLSAICFGR_PLLSAIQ_SHIFT) & RCC_PLLSAICFGR_PLLSAIQ_MASK;
	n = 417;
	r = 5;
	rcc_osc_off(RCC_PLLSAICFGR);
	/* mask out old values (q, p preserved above) */
	tmp &= ~(
			(RCC_PLLSAICFGR_PLLSAIN_MASK << RCC_PLLSAICFGR_PLLSAIN_SHIFT) |
			(RCC_PLLSAICFGR_PLLSAIP_MASK << RCC_PLLSAICFGR_PLLSAIP_SHIFT) |
			(RCC_PLLSAICFGR_PLLSAIQ_MASK << RCC_PLLSAICFGR_PLLSAIQ_SHIFT) |
			(RCC_PLLSAICFGR_PLLSAIR_MASK << RCC_PLLSAICFGR_PLLSAIR_SHIFT));
	/* store new values */
	RCC_PLLSAICFGR |= (
			((n & RCC_PLLSAICFGR_PLLSAIN_MASK) << RCC_PLLSAICFGR_PLLSAIN_SHIFT) |
			((p & RCC_PLLSAICFGR_PLLSAIP_MASK) << RCC_PLLSAICFGR_PLLSAIP_SHIFT) |
			((q & RCC_PLLSAICFGR_PLLSAIQ_MASK) << RCC_PLLSAICFGR_PLLSAIQ_SHIFT) |
			((r & RCC_PLLSAICFGR_PLLSAIR_MASK) << RCC_PLLSAICFGR_PLLSAIR_SHIFT));
	RCC_DCKCFGR = (RCC_DCKCFGR & ~(RCC_DCKCFGR_PLLSAIDIVR_MASK << RCC_DCKCFGR_PLLSAIDIVR_SHIFT)) |
				  (RCC_DCKCFGR_PLLSAIDIVR_DIVR_2 << RCC_DCKCFGR_PLLSAIDIVR_SHIFT);
	rcc_osc_on(RCC_PLLSAI);
	rcc_wait_for_osc_ready(RCC_PLLSAI);
	printf(" done.\n");

	rcc_periph_clock_enable(RCC_DSI);
	rcc_periph_clock_enable(RCC_LTDC);

	printf("Start DSI initialization...\n");
	printf("    Turn on the DSI regulator ... ");
	fflush(stdout);
	/* turn on the voltage regulator to the PHY */
	DSI_WRPCR |= DSI_WRPCR_REGEN;
	while ((DSI_WISR & DSI_WISR_RRS) == 0) ;
	printf(" done.\n");

	/* set up the DSI PLL */
	printf("    Setting up the PHY PLL ...\n");
	printf("    Verify DSI is off ... ");
	fflush(stdout);
	printf("%s\n", ((DSI_CR & 1) == 0) ? "OFF" : "ON");
	
	DSI_WRPCR |= DSI_SET(WRPCR, NDIV, 125) |
			    DSI_SET(WRPCR, IDF, DSI_WRPCR_IDF_DIV_2) |
				DSI_SET(WRPCR, ODF, DSI_WRPCR_IDF_DIV_1);
	DSI_WRPCR |= DSI_WRPCR_PLLEN;
	while ((DSI_WISR & DSI_WISR_PLLLS) == 0) ;
	printf("    Done.\n");

	printf("    Setup DSI PHY part ...");
	fflush(stdout);
	/* program the PHY */
	/* When should one enable these? Before ? After? Mid point? */
 	/* The ST code starts of by enabling them */
	DSI_PCTLR = DSI_PCTLR_CKE | DSI_PCTLR_DEN;
	DSI_CLCR = DSI_CLCR_DPCC;
	DSI_PCONFR |= DSI_SET(PCONFR, NL, DSI_PCONFR_NL_2LANE); /* two lanes */

	DSI_CCR = DSI_SET(CCR, TXECKDIV, 4);
	DSI_WPCR0 &= ~(DSI_MASK(WPCR0, UIX4));
	/* 500Mhz Phy clock, bit period is 8 * .25nS */
	DSI_WPCR0 |= DSI_SET(WPCR0, UIX4, 8);
	printf(" done\n");

	/* reset errors */
	DSI_IER0 = 0;
	DSI_IER1 = 0;
	printf("Initialization done\n");
	
	printf("Put DSI into Command mode ...\n");
	DSI_MCR |= DSI_MCR_CMDM;
	DSI_WCFGR = DSI_WCFGR_DSIM | DSI_SET(WCFGR, COLMUX, 5); 
	/* color mode (this is DSI -> display) of RGB 888 */
	DSI_LCOLCR = 5; /* RGB888 mode */

	DSI_LVCIDR = 0; /* channel 0 */
	DSI_GVCIDR = 0; /* channel 0 ### */


	/* command size is one line of pixels (800 in this case) */
	DSI_LCCR |= DSI_SET(LCCR, CMDSIZE, 800); 
	DSI_CMCR |= DSI_CMCR_TEARE;
	/* enable interrupts for tear and refresh */
	/* xxx TODO: verify nvic has the dsi and ltdc interrupt
	   vectors populated.
		DSI_WIER_TEIE
		DSI_WIER_ERIE 
    */
	printf("Done\n");
	/* set all the commands to be low power type */
	DSI_CMCR |= DSI_CMCR_DLWTX | DSI_CMCR_DSR0TX | DSI_CMCR_DSW1TX | DSI_CMCR_DSW0TX |
				DSI_CMCR_GLWTX | DSI_CMCR_GSR2TX | DSI_CMCR_GSR1TX | DSI_CMCR_GSR0TX |
				DSI_CMCR_GSW2TX | DSI_CMCR_GSW1TX | DSI_CMCR_GSW0TX ;
	/* Note MRDPS and ARE are taken from the initialization structure in the ST code
	 * but they are never set. Since the structure is declared in the BSS it is
	 * supposed to be cleared but this is a bug waiting to happen on ST's part
 	 */


	/* Start initializing the LTDC */
	printf("Initializing the LTDC ... ");
	fflush(stdout);
	/* set all polarity bits to 0 ### */
	LTDC_GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL | LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);
	/* TODO docs say this should be sync - 1 (so 0 ?) */
	LTDC_SSCR = (MY_HSYNC) << 16 | MY_VSYNC; /* Horiz / Vertical sync are '1' */
	/* TODO: this then trickles down, is it 1  + 1 - 1 = 1, or 2 ? */
	LTDC_BPCR = LTDC_SET(BPCR, AHBP, (MY_HSYNC + MY_HBP - 1)) |
				LTDC_SET(BPCR, AVBP, (MY_VSYNC + MY_VBP - 1)) ;
	LTDC_AWCR = LTDC_SET(AWCR, AAW, (MY_HSYNC + MY_HBP + MY_HACT - 1)) | 
			    LTDC_SET(AWCR, AAH, (MY_VSYNC + MY_VBP + MY_VACT - 1));
	LTDC_TWCR = LTDC_SET(TWCR, TOTALW, (MY_HSYNC + MY_HFP + MY_HACT + MY_HBP - 1)) | 
				LTDC_SET(TWCR, TOTALH, (MY_VSYNC + MY_VFP + MY_VACT + MY_VBP - 1));
	LTDC_BCCR = 0xff8080; /* non-black */
	/* xxx TODO: enable interrupts? 
		Transfer error and framing error (LTDC_IER_TERRIE, LTDC_IER_FUIE)
	*/
	/* Turn it on */
	LTDC_GCR |= LTDC_GCR_LTDCEN;
	printf("Done\n");
	ltdc_layer_setup(fb);

	/* now turn on the DSI Wrapper */
	DSI_CR |= DSI_CR_EN;
	DSI_WCR |= DSI_WCR_DSIEN;
	/* At this point we should be plumbed, and the real work begins to initialize the display
	itself */
	printf("Starting initialization of display ...\n ");
	fflush(stdout);
	init_display();
	printf("\ndone.\n");
	/* reset the commands to all be high power only now? */
	DSI_CMCR &= ~( DSI_CMCR_DLWTX | DSI_CMCR_DSR0TX | DSI_CMCR_DSW1TX | DSI_CMCR_DSW0TX |
				   DSI_CMCR_GLWTX | DSI_CMCR_GSR2TX | DSI_CMCR_GSR1TX | DSI_CMCR_GSR0TX |
				   DSI_CMCR_GSW2TX | DSI_CMCR_GSW1TX | DSI_CMCR_GSW0TX ) ;
	DSI_PCR |= DSI_PCR_BTAE; /* flow control */
	DSI_WCR |= DSI_WCR_LTDCEN; /* Turn on the LTDC (display should be lit) */
}


#define show_status(cond, ok, notok)	if ((cond)) \
			{ printf("%s%s%s, ", console_color(GREEN), ok, console_color(NONE)); } else \
			{ printf("%s%s%s, ", console_color(YELLOW), notok, console_color(NONE)); }

#define wrap_disable	DSI_WCR &= ~DSI_WCR_DSIEN
#define wrap_enable		DSI_WCR |= DSI_WCR_DSIEN


int
main(void)
{
	int i, r, c, x, y;
	uint32_t	*buf = (uint32_t *)(FB_ADDRESS);
	uint32_t	col;
	uint32_t	addr;
	uint32_t	pixel;
	uint32_t	old_status;

	fprintf(stderr, "LTDC/DSI Demo Program\n");
	gpio_init();
	/* 
	 * If we did this right, this fills the initial
	 * image frame buffer with a white grid on a black
	 * background.
	 */
	gfx_init(draw_pixel, 320, 240, GFX_FONT_LARGE);
	gfx_fillScreen(GFX_COLOR_WHITE);
	gfx_setTextColor(GFX_COLOR_BLUE, GFX_COLOR_BLACK);
	gfx_setCursor(100, 100);
	gfx_setTextSize(2);
	gfx_puts((unsigned char *)"What the FUCK is Going On!!!");
	gfx_fillRoundRect(50, 50, 50, 50, 5, GFX_COLOR_GREEN);
	
	

	lcd_init((uint8_t *)FB_ADDRESS);

/*
	ltdc_layer_setup((uint8_t *)buf);
	wrap_enable;
*/
	DSI_WCR |= (DSI_WCR_DSIEN | DSI_WCR_LTDCEN);

	
	printf("Anything interesting?\n");
	col = DSI_WCFGR;
	printf("DSI Wrapper configuration : 0x%08X\n", (unsigned int) col);

	col = DSI_WISR;
	old_status = col;
	printf("DSI Wrapper interrupt status register (0x%08X) : ", (unsigned int) col);
	show_status((col & DSI_WISR_RRS) != 0, "Regulator ready", "Regulator not ready");
	show_status((col & DSI_WISR_PLLLS) != 0, "PLL Locked", "PLL NOT locked");
	show_status((col & DSI_WISR_BUSY) == 0, "Idle", "Busy");
	show_status((col & DSI_WISR_TEIF) == 0, "TE seen", "No TE seen");
	printf("\n");
//	DSI_WCR |= DSI_WCR_LTDCEN;
	printf("DSI_WCFGR = 0x%08X\n", UINT DSI_WCFGR);
	printf("DSI_LCOLCR = 0x%08X\n", UINT DSI_LCOLCR);
	
	while (1) {
		char xc;
		uint8_t *bbuf;

//		hex_dump(FB_ADDRESS, (uint8_t *)(FB_ADDRESS), 256);
		while ((xc = console_getc(0)) == 0) {
			col = DSI_WISR;
			if (col != old_status) {
				printf("Updated status:\n");
				printf("DSI Wrapper interrupt status register (0x%08X) : ", (unsigned int) col);
				show_status((col & DSI_WISR_RRS) != 0, "Regulator ready", "Regulator not ready");
				show_status((col & DSI_WISR_PLLLS) != 0, "PLL Locked", "PLL NOT locked");
				show_status((col & DSI_WISR_BUSY) == 0, "Idle", "Busy");
				show_status((col & DSI_WISR_TEIF) == 0, "TE seen", "No TE seen");
				printf("\n");
				old_status = col;
			}
		}

		switch (xc) {
			case 'r':
				printf("Red buffer\n");
				for (i = 0; i < 800 * 480; i++) {
					*(buf+i) = 0xffff0000;
				}
				break;
			case 'g':
				printf("Green buffer\n");
				for (i = 0; i < 800 * 480; i++) {
					*(buf+i) = 0xff00ff00;
				}
				break;
			case 'b':
				printf("Blue buffer\n");
				for (i = 0; i < 800 * 480; i++) {
					*(buf+i) = 0xff0000ff;
				}
				break;
			case '1':
				printf("One test\n");
				for (i = 0; i < 800 * 480; i++) {
					*(buf+i) = 0xff000000;
				}
				for (r = 100; r < 200; r++) {
					for (c = 100; c < 200; c++) {
						*(buf + r*800 + c) = 0xffffff00;
					}
				}
				break;
			case '2':
				printf("Two test\n");
				for (r = 0; r < 480; r++) {
					for (c = 0; c < 800; c++) {
						pixel = 0xff000000;
						if (c & 0x10 ) {
							pixel = 0xffff0000;
						}
						*(buf + r * 800 + c) = pixel;
					}
				}
				break;
			case '3':
				printf("Two vertical lines\n");
				for (r = 0; r < 480; r++) {
					for (c = 0; c < 800; c++) {
						
						if (c < 16) {
							pixel = 0xffff0000; /* red */
						} else if ((c > 399) && (c < 416)) {
							pixel = 0xff00ff00; /* green */
						} else {
							pixel = 0xffffffff; /* white */
						}
						*(buf + r * 800 + c) = pixel;
					}
				}
				break;
			case '4':
				printf("grid\n");
				for (x = 0; x < 800; x++) {
					for (y = 0; y < 480; y++) {
						pixel = 0xff000000;
						if (((x & 0x1f) == 0) || ((y & 0x1f) == 0)) {
							pixel = 0xffffffff;
						}
						*(buf + y * 800  + x) = pixel;
					}
				}
				break;
			case '5':
				printf("GFX Failures\n");
				gfx_fillScreen((uint16_t) (0xffff));
			    /* test rectangle full screen size */
			    gfx_drawRoundRect(0, 0, 100, 60, 5, (uint16_t) 0xff00);
			    gfx_fillRect(17, 3, 66, 14, (uint16_t) 0x0ff0);
			    gfx_setCursor(19, 13);
			    /* this text doesn't write the 'background' color */
			    gfx_setTextColor((uint16_t) 0x0, (uint16_t) 0x0);
			    gfx_puts((unsigned char *) "Graphics");

			    /* change font size on the fly */
			    gfx_setFont(GFX_FONT_SMALL);
			    gfx_setTextColor((uint16_t) GFX_COLOR_YELLOW, (uint16_t) GFX_COLOR_CYAN);
				gfx_setCursor(8, 32);
				/* multiplicative scaling for 'bigger' text */
				gfx_setTextSize(2);
				gfx_puts((unsigned char *) "Testing");
				gfx_setTextSize(1);
				/* text rotation (four possible angles) */
				gfx_setTextRotation(GFX_ROT_270);
				gfx_setCursor(10, 55);
				gfx_puts((unsigned char *) "Geom");
				/* draw filled and outline versions of shapes */
				gfx_drawCircle(16, 38, 5, (uint16_t) GFX_COLOR_MAGENTA);
				gfx_fillRect(25, 33, 10, 10, (uint16_t) GFX_COLOR_YELLOW);
				gfx_drawTriangle(38, 43, 48, 43, 42, 33, (uint16_t) GFX_COLOR_BLUE);

				gfx_fillTriangle(11, 55, 21, 55, 16, 45, (uint16_t) GFX_COLOR_RED);
				gfx_drawRect(25, 45, 10, 10, (uint16_t) GFX_COLOR_GREEN);
				gfx_fillCircle(43, 50, 5, (uint16_t) GFX_COLOR_GREY);

				/* check for proper edge filling on adjacent triangles */
   				gfx_fillTriangle(52, 33, 77, 33, 52, 55, (uint16_t) GFX_COLOR_RED);
				gfx_fillTriangle(52, 55, 77, 33, 77, 55, (uint16_t) GFX_COLOR_GREEN);

   				/* rotate the other way */
				gfx_setTextRotation(GFX_ROT_90);
				gfx_setCursor(80, 33);
				gfx_puts((unsigned char *) "Test");
				break;

			case 'i':
				printf("Increment buffer\n");
				for (i = 0, bbuf = (uint8_t *)(FB_ADDRESS); i < 800 * 4 * 480; i++) {
					*(bbuf+i) = (uint8_t) i;
				}
				break;
			case 'e':
				/* memory exporer */
				addr = FB_ADDRESS;
				while (1) {
					hex_dump(addr, (uint8_t *)(addr), 256);
					printf("CMD> ");
					fflush(stdout);
					c = console_getc(1);
					if (c == '\033') {
						break;
					}
					switch (c) {
						case '\r':
						case 'n':
							addr += 256;
							break;
						case 'p':
							addr -= 256;
							break;
						case 'j':
							printf("Jump to address: ");
							fflush(stdout);
							addr = console_getnumber();
					}
					printf("\n");
					if ((addr+256) > 0xc0ffffff) {
						addr = 0xc0ffff00;
					}
					if ((addr) < 0xc0000000) {
						addr = 0xc0000000;
					}
				}
				break;
			case ' ':
				printf("Invert buffer\n");
				for (i = 0; i < 800 * 480; i++) {
					*(buf+i) ^= 0xffffffff;
				}
				break;
			case 't':
				printf("Test Pattern ...\n");
#if 0
				DSI_MCR = 0;
				DSI_VMCR |= DSI_VMCR_PGE;
#endif
				break;
			default:
				break;
		}
		printf("kicking the LTDC ...\n");
		DSI_WCR |= DSI_WCR_LTDCEN;
	}
				
}
