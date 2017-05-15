/*
 * reticule.c
 *
 * Copyright (C) 2016, 2017 Chuck McManis, All rights reserved.
 *
 * Generate a 'reticule' (the grid part of a measuring instrument)
 * for use in displaying quantitaive values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <math.h>
#include <gfx.h>

#include "../util/util.h"
#include "reticule.h"

GFX_COLOR ret_colors[] = {
	(GFX_COLOR) {.raw=0x0},	/* 0 = transparent black */
	GFX_COLOR_BLACK, 	/* 1 = BLACK (opaque) */
	GFX_COLOR_WHITE,	/* 2 = WHITE */
	GFX_COLOR_DKGREY,	/* 3 = DARK GREY */
	GFX_COLOR_LTGREY,	/* 4 = LIGHT GREY */
	GFX_COLOR_CYAN,		/* 5 = CYAN */
	GFX_COLOR_YELLOW,	/* 6 = YELLOW */
	GFX_COLOR_MAGENTA,	/* 7 = MAGENTA */
	(GFX_COLOR) {.raw=0xffffff00},	/* 8 = yellow(dark grid) */
	GFX_COLOR_BLUE, 	/* 9 = Blue (light grid) */
	GFX_COLOR_RED, 		/* 10 = Red */
	GFX_COLOR_GREEN 	/* 11 = Green */
};

#define RET_GRID	COLOR(0, 1, 0)
#define LIGHT_GRID	COLOR(0, 7, 0)
#define DARK_GRID	COLOR(0, 6, 0)


/* XXX these should probably be generated */
static const char *dbm[] = {
	"   0",
	" -20",
	" -40",
	" -60",
	" -80",
	"-100",
	"-120",
	"-140"
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
	"26",
	"28",
};

#define RCOLOR(c)	(GFX_COLOR){.raw=(uint32_t) c}
#define RET_WIDTH	800
#define RET_HEIGHT	480
#define RET_BPP		4

reticule_t ret;

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
reticule_t *
create_reticule(void)
{
	int i, k, t;
	int margin_top, margin_bottom, margin_left;
	int box_width, box_height;
	GFX_CTX *g;

	/* half the bytes (4 bit pixels) */
	ret.bm.buf = malloc((RET_WIDTH * RET_HEIGHT * RET_BPP) / 8);
	memset(ret.bm.buf, 0, (RET_WIDTH * RET_HEIGHT * RET_BPP) / 8);
	ret.bm.mode = DMA2D_L4;
	ret.bm.stride = (RET_WIDTH * RET_BPP) / 8;
	ret.bm.w = RET_WIDTH;
	ret.bm.h = RET_HEIGHT;
	ret.bm.maxc = 12;
	ret.bm.clut = (uint32_t *)ret_colors;
	dma2d_clear(&ret.bm, DMA2D_GREY);

	g = gfx_init(reticule_pixel, RET_WIDTH, RET_HEIGHT, GFX_FONT_LARGE,
								(void *)&ret);
	margin_top = gfx_get_text_height(g) + 1;
	margin_left = gfx_get_string_width(g, "-120") + margin_top + 1;
	margin_bottom = 2 * margin_top;
	box_width = RET_WIDTH - (margin_top + margin_left);
	box_height = RET_HEIGHT - (margin_top + margin_bottom);
	ret.b_w = box_width;
	ret.b_h = box_height;
	ret.o_x = margin_left;
	ret.o_y = margin_top;

	gfx_move_to(g, margin_left, margin_top);
	gfx_fill_rounded_rectangle(g, box_width, box_height, 10, RCOLOR(1));
	gfx_draw_rounded_rectangle(g, box_width, box_height, 10, RCOLOR(2));
	/* Text will be 'White'(color index 1)  */
	gfx_set_text_color(g, RCOLOR(8), RCOLOR(8));
	i = gfx_get_string_width(g, "POWER SPECTRUM FFT") / 2;
	gfx_set_text_cursor(g, margin_left + (box_width/2 - i), gfx_get_text_baseline(g));
	gfx_puts(g, "POWER SPECTRUM FFT");

	for (k = 30; k < box_height; k++) {
		for (i = 50; i < box_width; i ++) {
			if (((k % 50) == 0) && (i == 50)) {
				if ((k / 50) < 9) {
					t = gfx_get_string_width(g, (char *) dbm[(k/50)-1]);
					gfx_set_text_cursor(g, margin_left - (t + 2),
							margin_top + k + gfx_get_text_baseline(g)/2);
					gfx_puts(g, (char *) dbm[(k/50)-1]);
				}
				gfx_move_to(g, margin_left, margin_top + k);
				gfx_draw_line(g, 15, 0, RCOLOR(2));
				gfx_draw_line(g, box_width - 30, 0, RCOLOR(3));
				gfx_draw_line(g, 15, 0, RCOLOR(2));
			}
			if ((i % 50) == 0) {
				/* should use a "+" here? */
				gfx_move_to(g, margin_left + i, margin_top);
				gfx_draw_line(g, 0, 15, RCOLOR(2));
				gfx_draw_line(g, 0, box_height - 30, RCOLOR(3));
				gfx_draw_line(g, 0, 15, RCOLOR(2));
				if ((i/50) < 15) {
					t = gfx_get_string_width(g, (char *) hz[i/50]);
					gfx_set_text_cursor(g, i + margin_left - t/2,
							1 + margin_top + box_height + gfx_get_text_baseline(g));
					gfx_puts(g, (char *)hz[i/50]);
				}
			}
		}
	}
	t = gfx_get_string_width(g, "FREQUENCY (kHz)");
	gfx_set_text_cursor(g, margin_left + box_width/2 - t/2,
		margin_top + box_height + gfx_get_text_height(g) + gfx_get_text_baseline(g));
	gfx_puts(g, "FREQUENCY (kHz)");
	gfx_set_text_rotation(g, 90);
	t = gfx_get_string_width(g, "POWER (dBm)");
	gfx_set_text_cursor(g, gfx_get_text_baseline(g),
				margin_top + box_height / 2 + t / 2);
	gfx_puts(g, "POWER (dBM)");
	return &ret;
}
