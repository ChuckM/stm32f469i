/*
 * These then are the "utility" routines for the LCD display
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
#include "../util/util.h"
#include "../util/helpers.h"

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
static const uint8_t __lcd_init_data[] = {
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
	2, 0x35, 0x00,	/* Tearing Effect On (Attempt 2) */

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
static void send_command(uint8_t n_arg, const uint8_t *args);
static void init_display(void);

/*
 * This sends a command through the DSI to the display, either it has 2 
 * parameters which can be sent with a single write, or it has more than 2
 * in which case we push them into the FIFO and then send a "long" write 
 * with the number of args.
 *
 * If length is 1 it is a special case we just delay for *(args) milliseconds.
 */
static void
send_command(uint8_t n_arg, const uint8_t *args)
{
	int i;
	uint32_t tmp;

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
		}
		/* we assume that n_arg is always <= 255 */
		tmp = DSI_SET(GHCR, DT, 0x39) | DSI_SET(GHCR, WCLSB, n_arg);
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
static void
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

static void ltdc_layer_setup(void);

/*
 * Clear the frame buffer to a particular value
 */
void
lcd_clear(uint32_t color)
{
	uint32_t *fb;
	int	i;

	fb = (uint32_t *) FRAMEBUFFER_ADDRESS;
	for (i = 0; i < (800 * 480); i++) {
		/* force alpha to 0xff (opaque) */
		*(fb + i) = color | 0xff000000;
	}
}

/*
 * This should configure the LTDC controller to to display an 800 x 480 x 4 (ARGB8888)
 * region of memory, which if it goes as planned, will show up as the display contents.
 */
static void 
ltdc_layer_setup(void)
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
	LTDC_L1CFBAR = (uint32_t) FRAMEBUFFER_ADDRESS;
	LTDC_L1CFBLR = LTDC_SET(LxCFBLR, CFBP, (800 * 4)) | LTDC_SET(LxCFBLR, CFBLL, (800 * 4) + 3);
	LTDC_L1CFBLNR = 480;
	LTDC_L1CR = LTDC_LxCR_LAYER_ENABLE; /* no color key, no lookup table */

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
lcd_init(void)
{
	uint32_t tmp;
	uint16_t	m, n, q, r, p;

	gpio_init();
	gpio_clear(GPIOH, GPIO7);
	msleep(20);
	gpio_set(GPIOH, GPIO7);
	msleep(10);
	/* 
	 * Set up the PLLSAI clock. This clock sets the VCO to 384Mhz
	 * (HSE / PLLM * NDIV = 384Mhz
	 * It sets its PLLSAI48CLK to 48Mhz VCO/PLLP = 384 / 8 = 48
	 * It sets the SAI clock to 11.29Mhz 
	 * It sets the LCD Clock to 48 Mhz as well (div 2 in PLLSAIR, then another 4 in DKCFGR)
 	 */
	m = (RCC_PLLCFGR >> RCC_PLLCFGR_PLLM_SHIFT) & RCC_PLLCFGR_PLLM_MASK;
	tmp = RCC_PLLSAICFGR;
	p = 8;
	q = 34;
	n = 384 * m / 8;
	r = 5;
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
				  (RCC_DCKCFGR_PLLSAIDIVR_DIVR_2 << RCC_DCKCFGR_PLLSAIDIVR_SHIFT);
	RCC_DCKCFGR |= RCC_DCKCFGR_48MSEL; /* 48Mhz clock comes from SAI */
	rcc_osc_on(RCC_PLLSAI);
	rcc_wait_for_osc_ready(RCC_PLLSAI);

	/* Turn on the peripheral clocks for LTDC and DSI */
	rcc_periph_clock_enable(RCC_DSI);
	rcc_periph_clock_enable(RCC_LTDC);

	/* turn on the voltage regulator to the PHY */
	DSI_WRPCR |= DSI_WRPCR_REGEN;
	while ((DSI_WISR & DSI_WISR_RRS) == 0) ;

	/* set up the DSI PLL */
	
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

	/* reset errors */
	DSI_IER0 = 0;
	DSI_IER1 = 0;
	
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

	/* set all the commands to be low power type */
	DSI_CMCR |= DSI_CMCR_DLWTX | DSI_CMCR_DSR0TX | DSI_CMCR_DSW1TX | DSI_CMCR_DSW0TX |
				DSI_CMCR_GLWTX | DSI_CMCR_GSR2TX | DSI_CMCR_GSR1TX | DSI_CMCR_GSR0TX |
				DSI_CMCR_GSW2TX | DSI_CMCR_GSW1TX | DSI_CMCR_GSW0TX ;

	/* 
	 * Now the DSI Host is pretty much set up, so we need to initialize the LTDC.
	 * Note that if the DSI Wrapper is enabled (Bit DSIEN in DSI_WCR) you can't
	 * access the LTDC. So we have to work on it with the wrapper disabled
	 */

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
	LTDC_BCCR = 0x000000;

	/* Turn it on */
	LTDC_GCR |= LTDC_GCR_LTDCEN;
	
	/*
	 * Now program the Layer 1 registers to show our frame buffer (which is
	 * the first 1.5MB of SDRAM) as an 800 x 480 x 4 byte ARGB888 buffer.
	 */
	ltdc_layer_setup();
	lcd_clear(0); /* clear any junk out of it */

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
	init_display();

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

/*
 * Wait for DSI to be not busy then send it the frame
 */
void
lcd_flip(int te_lock)
{
	/* Wait for DSI to be not busy */
	while (DSI_WISR & DSI_WISR_BUSY) ;
	
	if (te_lock) {
		while (gpio_get(GPIOJ, GPIO2)  == 0) ;
	}
	DSI_WCR |= DSI_WCR_LTDCEN;
}

/*
 * lcd_draw_pixel -
 * 	Converts a RGB565 color to an ARGB888 color and puts
 *	it into the frame buffer. It is passed as a callback
 *  to the simple graphics library.
 */
void
lcd_draw_pixel(void * buf, int x, int y, uint32_t color)
{
	uint32_t dx, dy;
	uint32_t *ptr = (uint32_t *)buf;
	uint32_t pixel;


	dx = x; dy = y;
	pixel = 0xff000000 | color;
	*(ptr + dy * 800U + dx) = pixel;
}

