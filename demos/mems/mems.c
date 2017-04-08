/*
 * MEMs demo
 *
 * Copyright (C) 2016 Chuck McManis, All rights reserved.
 *
 * This demo shows off the MEMs microphone that is on
 * the STM32F469-DISCOVERY board.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <math.h>
#include <gfx.h>

#include "../util/util.h"

/*
 * MEMS Setup
 * -------------------------------------------------------
 */

#define SHADOW 0x80000000
#define DMA2D_RED	0xffff0000
#define DMA2D_BLACK	0xff000000

/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	6

void dma2d_bgfill(void);
void dma2d_fill(uint32_t color);

uint8_t reticule[700 * 450];

#define WHITE	COLOR(0xff, 0xff, 0xff)
#define GREY	COLOR(0x80, 0x80, 0x80)
#define BLACK	COLOR(0x0, 0x0, 0x0)
#define CYAN	COLOR(0x0, 0xff, 0xff)
#define YELLOW	COLOR(0xff, 0xff, 0x00)
#define MAGENTA	COLOR(0xff, 0x00, 0xff)
#define GRID_BG		COLOR(0xff, 0xff, 0xff)	/* white */
#define LIGHT_GRID	COLOR(0xc0, 0xc0, 0xff)	/* light blue */
#define DARK_GRID	COLOR(0xc0, 0xc0, 0xe0)	/* dark blue */

void create_reticule(int x, int y, GFX_COLOR c);
#define TOP_MARGIN	12
#define LEFT_MARGIN	35	
#define RET_WIDTH	700
#define RET_HEIGHT	400

static const char *dbm[] = {
	"   0",
	" -20",
	" -40",
	" -60",
	" -80",
	"-100",
	"-120"
};

static const char *hz[] = {
	" 0",
	" 2",
	" 4",
	" 6",
	" 8",
	"10",
	"12",
	"14",
	"16",
	"18",
	"20",
	"22",
	"24",
	"26"
};

void create_reticule(int x, int y, GFX_COLOR c) {
	int i, k;

/*
	clear_screen();
	reticule.width = 700;
	reticule.height = 450;
	reticule.fmt = L4;
	reticule.stride = 700;
	reticule.bpp = 8;
	fb = &reticule;
	gfx_init(write_pixel, reticule.width, reticule.height, GFX_FONT_LARGE);
*/

	gfx_drawRoundRect(x, y+TOP_MARGIN, RET_WIDTH, RET_HEIGHT, 10, WHITE);
	gfx_setTextSize(1);
	gfx_setCursor(x+(RET_WIDTH/2) - 50, y);
	gfx_setTextColor(WHITE, BLACK);
	gfx_puts((unsigned char *) "Power Spectrum FFT");
	y += TOP_MARGIN; /* margin */
	gfx_fillTriangle(x+0,y+0, x+20, y+0, x+0, y+20, c);
	for (k = 30; k < RET_HEIGHT; k++) {
		for (i = 50; i < RET_WIDTH; i ++) {
			if ((k % 50) == 0) {
				gfx_drawLine(x, y+k, x+RET_WIDTH, y+k, GREY);
				gfx_drawLine(x+RET_WIDTH-15, y+k, x+RET_WIDTH, y+k, WHITE);
				gfx_drawLine(x, y+k, x+15, y+k, WHITE);
			}
			if ((i % 50) == 0) {
				/* should use a "+" here? */
				gfx_drawLine(x+i, y, x+i, y+RET_HEIGHT-1, GREY);
				gfx_drawLine(x+i, y, x+i, y+15, WHITE);
				gfx_drawLine(x+i, y+RET_HEIGHT-15, x+i, y+RET_HEIGHT-1, WHITE);
			} 
		}
	}
	for (i = 30; i < RET_HEIGHT; i += 50) {
		gfx_setCursor(x - LEFT_MARGIN, i+y+30-6);
		if ((i/50) < 7) {
			gfx_puts((unsigned char *)dbm[i/50]);
		}
	}
	for (i = 0; i < RET_WIDTH; i += 50) {
		gfx_setCursor(x + i  - 8 , y + RET_HEIGHT + 14);
		if ((i/50) < 14) {
			gfx_puts((unsigned char *)hz[i/50]);
		}
	}

}

/*
 * The DMA2D device can copy memory to memory, so in this case
 * we have another frame buffer with just the background in it.
 * And that is copied from there to the main display. It happens
 * faster than the tight loop fill but slightly slower than the
 * register to memory fill.
 */
void
dma2d_bgfill(void)
{
#ifdef MEMORY_BENCHMARK
	uint32_t t1, t0;
#endif

	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		DMA2D_IFCR |= 0x3F;
		if (DMA2D_ISR & DMA2D_ISR_CEIF) {
			printf("Failed to clear configuration error\n");
			while (1);
		}
	}

#ifdef MEMORY_BENCHMARK
	t0 = mtime();
#endif
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2M);
	/* no change in alpha, same color mode, no CLUT */
	DMA2D_FGPFCCR = 0x0;
	DMA2D_FGMAR = (uint32_t) BACKGROUND_FB;
	DMA2D_FGOR = 0; /* full screen */
	DMA2D_OOR =	0;
	DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
	DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while ((DMA2D_CR & DMA2D_CR_START));
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error!\n");
		while (1);
	}
#ifdef MEMORY_BENCHMARK
	t1 = mtime();
	printf("Transfer rate (M2M) %6.2f MB/sec\n", 1464.84 / (float) (t1 - t0));
#endif
}

/*
 * Use the DMA2D peripheral in register to memory
 * mode to clear the frame buffer to a single
 * color.
 */
void
dma2d_fill(uint32_t color)
{
#ifdef MEMORY_BENCHMARK
	uint32_t t1, t0;

	t0 = mtime();
#endif
	DMA2D_IFCR |= 0x3F;
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
	DMA2D_OPFCCR = 0x0; /* ARGB8888 pixels */
	/* force it to have full alpha */
	DMA2D_OCOLR = 0xff000000 | color;
	DMA2D_OOR =	0;
	DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
	DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while (DMA2D_CR & DMA2D_CR_START);
#ifdef MEMORY_BENCHMARK
	t1 = mtime();
	printf("Transfer rate (R2M) %6.2f MB/sec\n", 1464.84 / (float) (t1 - t0));
#endif
}

/*
 * This set of utility functions are used once to
 * render our "background" into memory. Later we
 * will use the dma2d_bgfill to copy it into the
 * main display to "reset" the display to its non-drawn on
 * state.
 *
 * The background is a set up to look like quadrille
 * graph paper with darker lines every 5 lines.
 */

void bg_draw_pixel(int, int, GFX_COLOR);

/* Our own pixel writer for the background stuff */
void
bg_draw_pixel(int x, int y, GFX_COLOR color)
{
	uint32_t *fb = (uint32_t *) BACKGROUND_FB;
	*(fb + (y * 800) + x) = color.raw;
}

void generate_background(void);

/*
 * This generates a "pleasant" background which looks a bit
 * like graph paper.
 */
void
generate_background(void)
{
	int i, x, y;
	uint32_t *t;

	gfx_init(bg_draw_pixel, 800, 480, GFX_FONT_LARGE);
	for (i = 0, t = (uint32_t *)(BACKGROUND_FB); i < 800 * 480; i++, t++) {
		*t = 0xffffffff; /* clear to white */
	}
	/* draw a grid */
	t = (uint32_t *) BACKGROUND_FB;
	for (y = 1; y < 480; y++) {
		for (x = 1; x < 800; x++) {
			/* major grid line */
			if ((x % 50) == 0) {
				gfx_drawPixel(x-1, y, DARK_GRID);
				gfx_drawPixel(x, y, DARK_GRID);
				gfx_drawPixel(x+1, y, DARK_GRID);
			} else if ((y % 50) == 0) {
				gfx_drawPixel(x, y-1, DARK_GRID);
				gfx_drawPixel(x, y, DARK_GRID);
				gfx_drawPixel(x, y+1, DARK_GRID);
			} else if (((x % 25) == 0) || ((y % 25) == 0)) {
				gfx_drawPixel(x, y, LIGHT_GRID);
			}
		}
	}
	gfx_drawRoundRect(0, 0, 800, 480, 15, DARK_GRID);
	gfx_drawRoundRect(1, 1, 798, 478, 15, DARK_GRID);
	gfx_drawRoundRect(2, 2, 796, 476, 15, DARK_GRID);
}

/*
 * This then uses the DMA2D peripheral to copy a digit from the
 * pre-rendered digit buffer, and render it into the main display
 * area. It does what many people call a "cookie cutter" blit, which
 * is that the pixels that have color (are non-zero) are rendered
 * but when the digit buffer has a value of '0' the background is
 * rendered.
 *
 * The digit colors are stored in the Foreground color lookup
 * table, they are as passed in, except color 0 (background)
 * is always 0.
 *
 * DMA2D is set up to read the original image (which has alpha of
 * 0xff (opaque) and then it multiples that alpha and the color
 * alpha, and 0xFF renders the digit color opaquely, 0x00 renders
 * the existing color. When drawing drop shadows we use an alpha
 * of 0x80 which makes the drop shadows 50% transparent.
 */
#ifndef DMA2D_FG_CLUT
#define DMA2D_FG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0400UL)
#endif
#ifndef DMA2D_BG_CLUT
#define DMA2D_BG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0800UL)
#endif

#define N_FRAMES	10
uint32_t frame_times[N_FRAMES] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int te_lock = 1;

/*
 * This is the code for the simple DMA2D Demo
 *
 * Basically it lets you put display digits on the
 * screen manually or with the DMA2D peripheral. It
 * has various 'bling' levels, and it tracks performance
 * by measuring how long it takes to go from one frame
 * to the next.
 */
int
main(void) {
	int i;
	int	x0, y0, x1, y1;
	int	y2, y3;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "MEMS Microphone Demo program\n");

	gfx_init(lcd_draw_pixel, 800, 480, GFX_FONT_LARGE);
	dma2d_fill(0x0);
	create_reticule(50, 25, LIGHT_GRID);
	x0 = 51;
	y0 = TOP_MARGIN + 25 + RET_HEIGHT/2;
	y2 = TOP_MARGIN + 25 + RET_HEIGHT/2;
	for (i = 0; i < 699; i++) {
		y1 =  (int) (100 * sin((float) i * M_PI / 100.0)) + 25 + TOP_MARGIN + RET_HEIGHT/2;
		y3 =  (int) (100 * cos((float) i * M_PI / 100.0)) + 25 + TOP_MARGIN + RET_HEIGHT/2;
		x1 = i + 51;
		gfx_drawLine(x0, y0, x1, y1, YELLOW);
		gfx_drawLine(x0, y2, x1, y3, MAGENTA);
		x0 = x1;
		y0 = y1;
		y2 = y3;
	}
	lcd_flip(0);
	while (1) {
	}
}
