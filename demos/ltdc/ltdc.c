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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dsi.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scb.h>
#include <gfx.h>
#include "../util/util.h"

#define FB_ADDRESS	0xc0000000

void draw_pixel(int x, int y, uint16_t color);

uint32_t *current_buf = (uint32_t *)(0xc0000000);

void hard_fault_handler(void);
/*
 * Write a simple hard fault handler. Ideally you won't need it but in my case
 * while debugging it came in handy. 
 * If you hit it, you can find out which line of code you were on using addr2line
 * Example: (with TEST_FAULT defined)
 *    $ arm-none-eabi-addr2line -e sdram.elf 0x800070e
 *    <current directory>/sdram.c:374
 *
 * Which happens to be the code line just AFTER the one where the assignment is 
 * made to *addr in the main function.
 */
void
hard_fault_handler(void)
{
    printf("                    =HARD FAULT=\n");
    printf("Press a key to reset\n");
    while ((USART_SR(USART3) & USART_SR_RXNE) == 0) ;
    scb_reset_system();
}

/*
 * draw_pixel -
 * 	Converts a RGB565 color to an ARGB888 color and puts
 *	it into the frame buffer. It is passed as a callback
 *  to the simple graphics library.
 */
void
draw_pixel(int x, int y, uint16_t color)
{
	int	r, g, b;
	uint32_t dx, dy;
	uint32_t *ptr = (uint32_t *)(FB_ADDRESS);
	uint32_t pixel;


	dx = x; dy = y;
	r = ((color >> 11) & 0x1f) << 3;
	g = ((color >> 5) & 0x3f) << 2;
	b = (color & 0x1f) << 3;
	pixel = 0xff000000 | (r << 16) | (g << 8) | b;
	*(ptr + dy * 800U + dx) = pixel;
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
 * This sends a command through the DSI to the display, either it has 2 
 * parameters which can be sent with a single write, or it has more than 2
 * in which case we push them into the FIFO and then send a "long" write 
 * with the number of args.
 *
 * If length is 1 it is a special case we just delay for *(args) milliseconds.
 *
 * If DEBUG_COMMANDS is defined it writes out to the console the 32 bit values
 * sent to GHCR and GPDR. That can be used to see what was sent over the DSI bus
 * and compare it to other packages.
 */
void
send_command(uint8_t n_arg, const uint8_t *args)
{
	int i;
	uint32_t tmp;
#ifdef DEBUG_COMMANDS
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
 * And once you have isolated the packets to send (they are always the
 * same obviously) you can write a simple wrapper to force feed them
 * through the generic packet interface and send them off to the display
 * on the other side.
 *
 * I've extracted  all of the commands that are sent (combination
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
 * Enable the LCD Reset line.
 * PH7 - xres (reset) on the LCD
 */
static void
gpio_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOH);
	gpio_mode_setup(GPIOH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
	gpio_set_output_options(GPIOH, GPIO_OTYPE_OD, GPIO_OSPEED_100MHZ, GPIO7);
	/* tearing effect signal */
	rcc_periph_clock_enable(RCC_GPIOJ);
	gpio_mode_setup(GPIOJ, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO2);
}

/*
 * These are sort of "fake" parameters for the LTDC. Since it is
 * Going to force feed one frame to the DSI Wrapper when asked
 * it doesn't need a lot of timing.
 */
#define MY_HSYNC	2	/* Hsync */
#define MY_HFP		1
#define MY_HBP		1
#define MY_HACT		800

#define MY_VSYNC	2	/* Vsync */
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

	/* Sets up layer 1 as a "full screen" */
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

	/* immediate load from shadow */
	LTDC_SRCR = LTDC_SRCR_IMR;
	/* should be good to go */
}

/*
 * This function does all the heavy lifting. It initializes the DSI Host, the DSI
 * Wrapper, and the LTDC. Then kicks it off. When it exits the display should be
 * on and "clear" (all pixels set to black)
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
	/* 
	 * Set up the PLLSAI clock. This clock sets the VCO to 384Mhz
	 * (HSE / PLLM(8) * NDIV(384) = 384Mhz
	 * It sets its PLLSAI48CLK to 48Mhz VCO/PLLP = 384 / 8 = 48
	 * It sets the SAI clock to 11.29Mhz 
	 * It sets the LCD Clock to 48 Mhz as well (div 2 in PLLSAIR, then another 4 in DKCFGR)
 	 */
	tmp = RCC_PLLSAICFGR;
	p = 8;
	q = 34;
	n = 384;
	r = 2;
	rcc_osc_off(RCC_PLLSAICFGR);
	/* mask out old values (q, p preserved above) */
	tmp &= ~((RCC_PLLSAICFGR_PLLSAIN_MASK << RCC_PLLSAICFGR_PLLSAIN_SHIFT) |
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
				  (RCC_DCKCFGR_PLLSAIDIVR_DIVR_4 << RCC_DCKCFGR_PLLSAIDIVR_SHIFT);
	RCC_DCKCFGR |= RCC_DCKCFGR_48MSEL; /* 48Mhz clock comes from SAI */
	rcc_osc_on(RCC_PLLSAI);
	rcc_wait_for_osc_ready(RCC_PLLSAI);
	printf(" done.\n");

	/* Turn on the peripheral clocks for LTDC and DSI */
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
	
	/*
	 * This configures the clock to the PHY. It is set
	 * as (((HSE / input divisor) * NDIV) / output divisor)
	 * HSE is 8Mhz, input divisor is 2, output divisor is 1
	 * so ((8 / 2) * 125) / 1) = 500Mhz
	 */
	DSI_WRPCR |= DSI_SET(WRPCR, NDIV, 125) |
			    DSI_SET(WRPCR, IDF, DSI_WRPCR_IDF_DIV_2) |
				DSI_SET(WRPCR, ODF, DSI_WRPCR_ODF_DIV_1);
	DSI_WRPCR |= DSI_WRPCR_PLLEN;
	while ((DSI_WISR & DSI_WISR_PLLLS) == 0) ;
	printf("    Done.\n");

	printf("    Setup DSI PHY part ...");
	fflush(stdout);
	/* Now that the PHY has power (regulator) and clock
	 * we can program it.
 	 * The ST code starts of by enabling them so I do too.
	 */
	DSI_PCTLR = DSI_PCTLR_CKE | DSI_PCTLR_DEN;
	DSI_CLCR = DSI_CLCR_DPCC;
	DSI_PCONFR |= DSI_SET(PCONFR, NL, DSI_PCONFR_NL_2LANE); /* two lanes */

	/* Set the the "escape" clock divisor, RM0386 says it's 20Mhz 
	 * maximum, and it is fed by the Phy clock / 8 (62.5Mhz) so we
 	 * divide by 4 to make it 15.6Mhz (which is < 20Mhz)
	 */
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
	/* 
	 * Set the color mode in two places, in the wrapper device (WFCGR)
	 * and in the DSI Host (LCOLCR)
	 * DSI Wrapper color mode (this is LTDC -> DSI Host)
	 */
	DSI_WCFGR = DSI_WCFGR_DSIM | DSI_SET(WCFGR, COLMUX, 5); 
	/* DSI host color mode (this is DSI -> display) of RGB 888 */
	DSI_LCOLCR = 5; /* RGB888 mode */

	/* We want to send commands on channel 0 */
	DSI_LVCIDR = 0; /* LTDC uses channel 0 */
	DSI_GVCIDR = 0; /* Generic packets also use channel 0 */


	/* Largest command size is one line of pixels (800 in this case) */
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

	/* 
	 * Now the DSI Host is pretty much set up, so we need to initialize the LTDC.
	 * Note that if the DSI Wrapper is enabled (Bit DSIEN in DSI_WCR) you can't
	 * access the LTDC. So we have to work on it with the wrapper disabled
	 */
	printf("Initializing the LTDC ... ");
	fflush(stdout);
	/* set all polarity bits to 0 ### */
	LTDC_GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL | LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);
	LTDC_SSCR = (MY_HSYNC - 1) << 16 | (MY_VSYNC - 1);
	LTDC_BPCR = LTDC_SET(BPCR, AHBP, (MY_HSYNC + MY_HBP - 1)) |
				LTDC_SET(BPCR, AVBP, (MY_VSYNC + MY_VBP - 1)) ;
	LTDC_AWCR = LTDC_SET(AWCR, AAW, (MY_HSYNC + MY_HBP + MY_HACT - 1)) | 
			    LTDC_SET(AWCR, AAH, (MY_VSYNC + MY_VBP + MY_VACT - 1));
	LTDC_TWCR = LTDC_SET(TWCR, TOTALW, (MY_HSYNC + MY_HFP + MY_HACT + MY_HBP - 1)) | 
				LTDC_SET(TWCR, TOTALH, (MY_VSYNC + MY_VFP + MY_VACT + MY_VBP - 1));
	/*
	 * Default background color is non-black so you can see if if your layer setup
	 * doesn't completely cover the display.
	 */
	LTDC_BCCR = 0xff8080;

	/* Turn it on */
	LTDC_GCR |= LTDC_GCR_LTDCEN;
	printf("Done\n");
	
	/*
	 * Now program the Layer 1 registers to show our frame buffer (which is
	 * the first 1.5MB of SDRAM) as an 800 x 480 x 4 byte ARGB888 buffer.
	 */
	ltdc_layer_setup(fb);

	/* now turn on the DSI Host and the DSI  Wrapper */
	DSI_CR |= DSI_CR_EN;
	DSI_WCR |= DSI_WCR_DSIEN;

	/* 
	 * At this point we have the DSI Host, the DSI Phy,  the LTDC, and the
	 * DSI wrapper all set up. So we can now send commands through this
	 * "pipe" into the display connected to the DSI host. (this is very
	 * similar to other systems where you send all the setup commands
	 * through a SPI port. In this case though you have to use the
	 * Generic Packet interface.
	 */
	printf("Starting initialization of display ...\n ");
	fflush(stdout);
	init_display();
	printf("\ndone.\n");

	/* reset the commands to all be high power only now? */
	DSI_CMCR &= ~( DSI_CMCR_DLWTX | DSI_CMCR_DSR0TX | DSI_CMCR_DSW1TX | DSI_CMCR_DSW0TX |
				   DSI_CMCR_GLWTX | DSI_CMCR_GSR2TX | DSI_CMCR_GSR1TX | DSI_CMCR_GSR0TX |
				   DSI_CMCR_GSW2TX | DSI_CMCR_GSW1TX | DSI_CMCR_GSW0TX ) ;
	DSI_PCR |= DSI_PCR_BTAE; /* flow control */

	/* And now when you tell the DSI Wrapper to enable the LTDC it 
	 * un "pauses" it and the LTDC generates one frame of display.
	 * that frame is captured by the wrapper and fed into the LCD
	 * display and the LTDC is re-paused. (this is Adapted Command
	 * Mode (as opposed to "video" mode).
	 */
	DSI_WCR |= DSI_WCR_LTDCEN;
	/* this point what ever is in the frame buffer should be on
	 * the display. You can change the frame buffer but until you
	 * write this bit again it won't be loaded on to the display
	 */
}


#define show_status(cond, ok, notok)	if ((cond)) \
			{ printf("%s%s%s, ", console_color(GREEN), ok, console_color(NONE)); } else \
			{ printf("%s%s%s, ", console_color(YELLOW), notok, console_color(NONE)); }

/* Macros to turn off and on the DSI Wrapper */
#define wrap_disable	DSI_WCR &= ~DSI_WCR_DSIEN
#define wrap_enable		DSI_WCR |= DSI_WCR_DSIEN

void flip(void);

/*
 * Wait for DSI to be not busy then send it the frame
 */
void
flip(void)
{
	int cnt;

	while (DSI_WISR & DSI_WISR_BUSY) ;
	cnt = 0;
#if 0
	while (gpio_get(GPIOJ, GPIO2) == 0) {
		cnt++;
	}
#endif
	printf("Count = %d\n", cnt);
	DSI_WCR |= DSI_WCR_LTDCEN;
}

uint32_t color_bars[] = {
	0xff0000,
	0x00ff00,
	0x0000ff,
	0xffff00,
	0x00ffff,
	0xff00ff,
	0x7f0000,
	0x007f00,
	0x00007f,
	0x7f7f00,
	0x007f7f,
	0x7f007f
};

void simple_graphics(void);

/*
 * This fairly contrived bit exploits my simple graphics library
 * to put something "interesting" on the screen other than the
 * hacky grids or lines etc. It assumes that gfx_init() has been
 * called already.
 */
void
simple_graphics(void)
{
	/* pick the 7 x 9 font (8 x 12 font box) */
	gfx_setFont(GFX_FONT_LARGE);
	gfx_fillScreen(GFX_COLOR_BLACK);
	gfx_fillRoundRect(0, 0, 800, 480, 15, (uint16_t) GFX_COLOR_WHITE);
	gfx_drawRoundRect(0, 0, 800, 480, 15, (uint16_t) GFX_COLOR_RED);

	/* Yellow on blue characters and make it 2x bigger (16 x 24 font box) */
	gfx_setTextColor(GFX_COLOR_YELLOW, GFX_COLOR_BLUE);
	gfx_setTextSize(2);
	gfx_fillRoundRect(236, 100, 20*16+7, 24+8, 10, (uint16_t) GFX_COLOR_BLUE);
	gfx_drawRoundRect(235, 100, 20*16+7, 24+8, 10, (uint16_t) GFX_COLOR_YELLOW);

	gfx_setCursor(236+5, 100+24);
	gfx_puts((unsigned char *) "Simple Graphics Demo");

	gfx_fillCircle(230, 200, 50, GFX_COLOR_RED);
	gfx_fillTriangle(310, 250, 360, 150, 410, 250, GFX_COLOR_GREEN);
	gfx_fillRect(180, 280, 100, 100, GFX_COLOR_BLUE);
	/* check for proper edge filling on adjacent triangles */
	gfx_fillTriangle(310, 380, 310, 280, 410, 280, GFX_COLOR_RED);
	gfx_fillTriangle(310, 380, 410, 280, 410, 380, GFX_COLOR_GREEN);

	gfx_drawLine(180, 264, 410, 264, GFX_COLOR_BLACK);
	gfx_drawLine(180, 265, 410, 265, GFX_COLOR_BLACK);
	gfx_drawLine(180, 266, 410, 266, GFX_COLOR_BLACK);

	gfx_drawLine(294, 150, 294, 380, GFX_COLOR_BLACK);
	gfx_drawLine(295, 150, 295, 380, GFX_COLOR_BLACK);
	gfx_drawLine(296, 150, 296, 380, GFX_COLOR_BLACK);

	gfx_setTextColor(0, 0);
	gfx_setTextRotation(GFX_ROT_90);
	gfx_setCursor(550, 150);
	gfx_puts((unsigned char *) "Rotated Text");
	gfx_setFont(GFX_FONT_SMALL);
	gfx_setCursor(520, 150);
	gfx_puts((unsigned char *) "And two distinct fonts");
	gfx_setTextRotation(GFX_ROT_0);
	gfx_setTextSize(1);
	gfx_setCursor(180, 420);
	gfx_puts((unsigned char *) "At scale = 1, small font is hard to read.");
	
}

int
main(void)
{
	int t, c, x, y;
	unsigned int	i;
	uint32_t	*buf = (uint32_t *)(FB_ADDRESS);
	uint32_t	col;
	uint32_t	addr;
	uint32_t	pixel;

	uint32_t	*test_buf;
	uint32_t	test_val;


	fprintf(stderr, "LTDC/DSI Demo Program\n");
	gpio_init();
	/* 
	 * If we did this right, this fills the initial
	 * image frame buffer with a white grid on a black
	 * background.
	 */
	gfx_init(draw_pixel, 800, 480, GFX_FONT_LARGE);
	simple_graphics();
	
	lcd_init((uint8_t *)FB_ADDRESS);

	/* Note you can't adjust the LTDC if the DSI Wrapper is enabled! */
	DSI_WCR |= (DSI_WCR_DSIEN | DSI_WCR_LTDCEN);

	
	printf("Anything interesting?\n");
	col = DSI_WCFGR;
	printf("DSI Wrapper configuration : 0x%08X\n", (unsigned int) col);
	printf("DSI_WCFGR = 0x%08X\n", UINT DSI_WCFGR);
	printf("DSI_LCOLCR = 0x%08X\n", UINT DSI_LCOLCR);
	
	while (1) {
		char xc;

		while (1) {
			xc = console_getc(1);
			switch (xc) {
				case 'C':
					printf("Enter fill color: ");
					fflush(stdout);
					col = console_getnumber();
					printf("\n");
					for (i = 0; i < 800 * 480; i++) {
						*(buf + i) = col;
					}
					break;
				case 'g':
					printf("Grid\n");
					printf("Enter grid size : ");
					fflush(stdout);
					t = console_getnumber();
					printf("\n");
					for (y = 0; y < 480;  y++) {
						for (x = 0; x < 800; x++) {
							pixel = 0xff000000; /* black */
							if (((x % t) == 0) || ((y % t) == 0) ||
								(x == 799) || (y == 479)) {
								pixel = 0xffffffff; /* white */
							}
							*(buf + y * 800 + x) = pixel;
						}
					}
					break;
				case 'c':
					printf("Color Bars\n");
					for (y = 0; y < 480; y++) {
						for (x = 0; x < 800; x++) {
							pixel = 0;
							if (y < 400) {
								pixel = color_bars[(x / 66) % 12];
							} else if (y > 410) {
								pixel = (x / 3) & 0xff;
								pixel = (pixel << 8) | pixel;
								pixel = (pixel << 8) | pixel;
							}
							pixel |= 0xff000000;
							*(buf + y * 800 + x) = pixel;
						}
					}
					break;
				case 't':
					/* This is pretty contrived, just to show the primitives */
					printf("GFX Test Output\n");
					simple_graphics();
					break;
				case 'e':
					/* memory explorer */
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
								break;
							default:
								printf("n - next page\n");
								printf("p - previous page\n");
								printf("j - jump to specified address (hex entry is allowed)");
								printf("    legal range 0xc0000000 - 0xc0ffff00\n");
								break;
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
				case 'm':
					test_buf = (uint32_t *)(SDRAM_BASE_ADDRESS);
					printf("Testing Memory\n");
					for (test_val = 0; test_val < 1000000; test_val++) {
						*test_buf++ = test_val;
					}
					test_buf = (uint32_t *)(SDRAM_BASE_ADDRESS);
					for (test_val = 0; test_val < 1000000; test_val++) {
						if (*test_buf != test_val) {
							printf("Memory should have %u, but instead has %u\n", (unsigned int) test_val,
								(unsigned int) *test_buf);
						}
						test_buf++;
					}
					break;
				default:
				case '?':
					printf("\nFunctions/cmds :\n");
					printf("    g - create a grid of specified size\n");
					printf("    C - fill with specified color (0xffffff is white)\n");
					printf("    t - Show the simple graphics test\n");
					printf("    c - Show basic color bars\n");
					printf("    e - Start the memory explorer (esc to exit)\n");
					printf("    m - Run a simple memory test on SDRAM memory\n");
					break;
			}
			flip();
		}
	}
}
