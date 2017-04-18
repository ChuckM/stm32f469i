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
#include "dma2d.h"

/*
 * MEMS Setup
 * -------------------------------------------------------
 */

#define DMA2D_SHADOW	(DMA2D_COLOR){.raw=0x80000000}
#define DMA2D_RED	(DMA2D_COLOR){ .raw=0xffff0000}
#define DMA2D_BLACK	(DMA2D_COLOR){.raw=0xff000000}
#define DMA2D_WHITE	(DMA2D_COLOR){.raw=0xffffffff}
#define DMA2D_GREY	(DMA2D_COLOR){.raw=0xff555555}

/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	6

GFX_COLOR ret_colors[] = {
	GFX_COLOR_BLACK, 	/* 0 = BLACK */
	GFX_COLOR_WHITE,	/* 1 = WHITE */
	GFX_COLOR_LTGREY,	/* 2 = GREY */
	GFX_COLOR_CYAN,		/* 3 = CYAN */
	GFX_COLOR_YELLOW,	/* 4 = YELLOW */
	GFX_COLOR_MAGENTA,	/* 5 = MAGENTA */
	GFX_COLOR_DKBLUE,	/* 6 = Dark blue (dark grid) */
	GFX_COLOR_BLUE 		/* 7 = Blue (light grid) */
};

#define RET_GRID	COLOR(0, 1, 0)
#define LIGHT_GRID	COLOR(0, 7, 0)
#define DARK_GRID	COLOR(0, 6, 0)

DMA2D_BITMAP *create_reticule(void);

DMA2D_BITMAP screen = {
	.buf = (void *)(FRAMEBUFFER_ADDRESS),
	.mode = DMA2D_ARGB8888,
	.w = 800,
	.h = 480,
	.stride = 800 * 4,
	.fg = DMA2D_WHITE,
	.bg = DMA2D_BLACK,
	.clut = NULL
};

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

#define RCOLOR(c)	(GFX_COLOR){.raw=(uint32_t) c}
#define TOP_MARGIN	12
#define LEFT_MARGIN	35	
#define RET_WIDTH	700
#define RET_HEIGHT	450
#define RET_BPP		4
static uint8_t	__reticule_fb[(RET_WIDTH * RET_HEIGHT) / 2];
struct {
	DMA2D_BITMAP	bm;
	int o_x, o_y;	/* offset x and y into the box */
	int b_w, b_h;	/* box width, box height */
} reticule;

/*
 * This is a 'trampoline' function between the GFX
 * libary's generalized notion of displays and the
 * DMA2D devices understanding of bitmaps.
 */
static void
reticule_pixel(void *buf, int x, int y, GFX_COLOR c)
{
	/* use the green component as the 'color' */
	dma2d_draw_4bpp(buf, x, y, (uint32_t)(c.raw) & 0xff);
}

/*
 * Build a reticule in a 4bpp buffer.
 */
DMA2D_BITMAP *
create_reticule(void)
{
	int i, k, t;
	int margin_top, margin_bottom, margin_left;
	int box_width, box_height;
	GFX_CTX *g;

	/* half the bytes (4 bit pixels) */
	reticule.bm.buf = __reticule_fb;
	reticule.bm.mode = DMA2D_L4;
	reticule.bm.stride = RET_WIDTH/2;
	reticule.bm.w = RET_WIDTH;
	reticule.bm.h = RET_HEIGHT;
	reticule.bm.maxc = 3;
	reticule.bm.clut = (uint32_t *)ret_colors;
	dma2d_clear((DMA2D_BITMAP *)&reticule, DMA2D_GREY);

	g = gfx_init(reticule_pixel, RET_WIDTH, RET_HEIGHT, GFX_FONT_LARGE, 
								(void *)&reticule);
	margin_top = gfx_get_text_height(g) + 1;
	margin_left = gfx_get_string_width(g, "-120") + margin_top;
	margin_bottom = 2 * margin_top;
	box_width = RET_WIDTH - (margin_top + margin_left);
	box_height = RET_HEIGHT - (margin_top + margin_bottom);
	gfx_move_to(g, margin_left, margin_top);
	gfx_draw_rounded_rectangle(g, box_width, box_height, 10, RCOLOR(1));
	/* Text will be 'White'(color index 1)  */
	gfx_set_text_color(g, RCOLOR(1), RCOLOR(1));
	i = gfx_get_string_width(g, "Power Spectrum FFT") / 2;
	gfx_set_text_cursor(g, margin_left + (box_width - i), gfx_get_text_baseline(g));
	gfx_puts(g, "Power Spectrum FFT");

	for (k = 30; k < box_height; k++) {
		for (i = 50; i < box_width; i ++) {
			if ((k % 50) == 0) {
				gfx_move_to(g, margin_left, margin_top + k);
				gfx_draw_line(g, 15, 0, RCOLOR(1));
				gfx_draw_line(g, box_width - 30, 0, RCOLOR(2));
				gfx_draw_line(g, 15, 0, RCOLOR(1));
			}
			if ((i % 50) == 0) {
				/* should use a "+" here? */
				gfx_move_to(g, margin_left + i, margin_top);
				gfx_draw_line(g, 0, 15, RCOLOR(1));
				gfx_draw_line(g, box_height - 30, 0, RCOLOR(2));
				gfx_draw_line(g, 0, 15, RCOLOR(1));
			} 
		}
	}
	for (i = 30; i < box_height; i += 50) {
		if ((i/50) < 7) {
			t = gfx_get_string_width(g, (char *) dbm[i/50]);
			gfx_set_text_cursor(g, margin_left - (t + 1),
					margin_top + i +
				gfx_get_text_baseline(g) / 2);
			gfx_puts(g, (char *) dbm[i/50]);
		}
	}
	for (i = 0; i < box_width; i += 50) {
		if ((i/50) < 14) {
			t = gfx_get_string_width(g, (char *) hz[i/50]);
			gfx_set_text_cursor(g, i + margin_left - t/2,
				       margin_top + box_height + gfx_get_text_baseline(g));
			gfx_puts(g, (char *)hz[i/50]);
		}
	}
	t = gfx_get_string_width(g, "FREQUENCY (kHz)");
	gfx_set_text_cursor(g, margin_left + box_width/2 - t/2,
		margin_top + box_height + gfx_get_text_height(g) + gfx_get_text_baseline(g));
	gfx_puts(g, "FREQUENCY (kHz)");
	gfx_set_text_rotation(g, -90);
	t = gfx_get_string_width(g, "POWER (dBm)");
	gfx_set_text_cursor(g, gfx_get_text_baseline(g), 
				margin_top + box_height / 2 + t / 2);
	gfx_puts(g, "POWER (dBm)");
	return (DMA2D_BITMAP *)&reticule;
}

/*
 * Lets see if we can look at MEMs microphones
 */
int
main(void) {
	int i;
	int	x0, y0, x1, y1;
	int	y2, y3;
	GFX_CTX	*g;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "MEMS Microphone Demo program\n");

	g = gfx_init(lcd_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *)FRAMEBUFFER_ADDRESS);
	dma2d_clear(&screen, DMA2D_GREY);
	(void) create_reticule();
	x0 = 51;
	y0 = TOP_MARGIN + 25 + RET_HEIGHT/2;
	y2 = TOP_MARGIN + 25 + RET_HEIGHT/2;
	for (i = 0; i < 699; i++) {
		y1 =  (int) (100 * sin((float) i * M_PI / 100.0)) + 25 + TOP_MARGIN + RET_HEIGHT/2;
		y3 =  (int) (100 * cos((float) i * M_PI / 100.0)) + 25 + TOP_MARGIN + RET_HEIGHT/2;
		x1 = i + 51;
		gfx_move_to(g, x0, y0);
		gfx_draw_line(g, (x1 - x0), (y1 - y0), GFX_COLOR_YELLOW);
		gfx_move_to(g, x0, y2);
		gfx_draw_line(g, x1 - x0, y3 - y2, GFX_COLOR_MAGENTA);
		x0 = x1;
		y0 = y1;
		y2 = y3;
	}
	dma2d_render((DMA2D_BITMAP *)&reticule, &screen, 0, 0);
	lcd_flip(0);
	while (1) {
	}
}