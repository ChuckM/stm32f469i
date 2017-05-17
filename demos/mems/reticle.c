/*
 * reticle.c
 *
 * Copyright (C) 2016, 2017 Chuck McManis, All rights reserved.
 *
 * Generate a 'reticle' (the grid part of a measuring instrument)
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
#include "reticle.h"

DMA2D_COLOR ret_colors[] = {
	DMA2D_COLOR_CLEAR,	/* 0 - Transparent */
	DMA2D_COLOR_BLACK, 	/* 1 = BLACK */
	DMA2D_COLOR_WHITE,	/* 2 = WHITE */
	DMA2D_COLOR_DKGREY,	/* 3 = DARK GREY */
	DMA2D_COLOR_LTGREY,	/* 4 = LIGHT GREY */
	DMA2D_COLOR_CYAN,		/* 5 = CYAN */
	DMA2D_COLOR_YELLOW,	/* 6 = YELLOW */
	DMA2D_COLOR_MAGENTA,	/* 7 = MAGENTA */
	(DMA2D_COLOR) 		{.raw=0xffffff00},	/* 8 = yellow(dark grid) */
	DMA2D_COLOR_BLUE, 	/* 9 = Blue (light grid) */
	DMA2D_COLOR_RED, 		/* 10 = Red */
	DMA2D_COLOR_GREEN 	/* 11 = Green */
};

#define RET_GRID	COLOR(0, 1, 0)
#define LIGHT_GRID	COLOR(0, 7, 0)
#define DARK_GRID	COLOR(0, 6, 0)

#define RCOLOR(c)	(GFX_COLOR){.raw=(uint32_t) c}
#define RET_BPP		4

/*
 * This is a 'trampoline' function between the GFX
 * libary's generalized notion of displays and the
 * DMA2D devices understanding of bitmaps.
 */
static void
reticle_pixel(void *buf, int x, int y, GFX_COLOR c)
{
	/* use the green component as the 'color' */
	dma2d_draw_4bpp(buf, x, y, (uint32_t)(c.raw) & 0xff);
}

/*
 * Build a reticle in a 4bpp buffer.
 *
 * What makes for a good Reticule? Readability? Measurability?
 * Rapid assimilation of meaning? I don't know, but this
 * is where I'm looking at different options.
 *
 * For now, we'll "force" reticle to be 10 divisions wide
 * regardless of how wide in pixels they are (caveat < 20 pixels!)
 * and 8 divisions high. This is a 'standard' Oscilloscope
 * view.
 *
 * That said, this is the layout, in ASCII art
 *
 *                            TITLE    /   /
 *          +-------+-------+-------+-/   /-+-------+
 *   S999.9 |       |       |       |/   /  |       |
 * Y        +-------+-------+-------/   /---+-------+
 *   S999.9 |       |       |      /   /    |       |
 * -        +-------+-------+-----/   /-----+-------+
 *   S999.9 |       |       |    /   /      |       |
 * a        +-------+-------+---/   /-------+-------+
 * x S999.9 |       |       |  /   /|       |       |
 * i        +-------+-------+-/   /-+-------+-------+
 * s S999.9 |       |       |/   /  |       |       |
 *          +-------+-------/   /---+-------+-------+
 *   S999.9 |       |      /   /    |       |       |
 *          +-------+-----/   /-----+-------+-------+
 *                 99    /   /     99       99 
 *                   X - Axis Label
 *
 * Width and Height passed in are overall width and height
 * and we compute inside width and height of just the reticle
 * area which we can use in viewport calculations as well.
 */
reticle_t *
create_reticle(char *label, int w, int h, GFX_FONT font,
				char *xlabel, float min_x, float max_x, 
				char *ylabel, float min_y, float max_y)
{
	int i, t;
	int margin_top, margin_bottom, margin_left;
	int box_width, box_height, division;
	reticle_t *res;
	char value[8];
	GFX_CTX my_ctx; /* don't bother alloc and freeing one */
	GFX_CTX *g;

	/* half the bytes (4 bit pixels) */
	res = malloc(sizeof(reticle_t));
	/* we won't segv but caller might */
	if (res == NULL) {
		return res;
	}

	memset(res, 0, sizeof(reticle_t));
	res->bm.buf = malloc((w * h * RET_BPP) / 8);
	memset(res->bm.buf, 0, (w * h * RET_BPP) / 8);
	res->bm.mode = DMA2D_L4;
	res->bm.stride = (w * RET_BPP) / 8;
	res->bm.w = w;
	res->bm.h = h;
	res->bm.maxc = 12;
	res->bm.clut = (uint32_t *)ret_colors;
	dma2d_clear(&(res->bm), DMA2D_COLOR_GREY);

	division = w / 10;
	g = gfx_init(&my_ctx, reticle_pixel, w, h, font, (void *)res);

	/* Labels on the top and bottom, numbers on the left and bottom */
	margin_top = gfx_get_text_height(g) + 1;
	/* four digits plus sign */
	margin_left = gfx_get_string_width(g, "-999.9") + margin_top + 1;
	margin_bottom = 2 * margin_top;

	/* Left over box width */
	box_width = w - (margin_top + margin_left);
	/* compute an even pixel count per division */
	division = box_width / 10;
	printf("Initial calc: box width, height %d, %d,  division size %d\n", division * 10, 
			division  * 8, division);

	/* 10 divsions wide by 8 divisions high */
	box_width = division * 10;
	box_height = division * 8;

	if (box_height > (h - (margin_top + margin_bottom))) {
		division = (h - (margin_top + margin_bottom)) / 10;
		box_height = 8 * division;
		box_width = 10 * division;
		printf("re-calc: box width, height %d, %d, division size %d\n", division * 10,
			division * 8, division);
	}
	res->b_w = box_width;
	res->b_h = box_height;
	res->o_x = margin_left;
	res->o_y = margin_top;

#if 0
	/* compute how big a bitmap we need */
	w = box_width + margin_top + margin_left;
	h = box_height + margin_top + margin_bottom;
	/* reset the context with the new size */
	g = gfx_init(&my_ctx, reticle_pixel, w, h, font, (void *)res);
#endif

	gfx_move_to(g, margin_left, margin_top);
	gfx_fill_rounded_rectangle(g, box_width, box_height, 10, RCOLOR(1));
	gfx_draw_rounded_rectangle(g, box_width, box_height, 10, RCOLOR(2));
	/* Text will be Color index 8)  */
	gfx_set_text_color(g, RCOLOR(8), RCOLOR(8));
	i = gfx_get_string_width(g, label) / 2;
	gfx_set_text_cursor(g, margin_left + (box_width/2 - i), gfx_get_text_baseline(g));
	gfx_puts(g, label);

	/* draw division lines */
	for (i = division; i < box_width; i += division) {
		gfx_move_to(g, margin_left + i, margin_top);
		gfx_draw_line(g, 0, 15, RCOLOR(2));
		gfx_draw_line(g, 0, box_height - 30, RCOLOR(3));
		gfx_draw_line(g, 0, 15, RCOLOR(2));
	}
	for (i = division; i < box_height; i += division) {
		gfx_move_to(g, margin_left, margin_top + i);
		gfx_draw_line(g, 15, 0, RCOLOR(2));
		gfx_draw_line(g, box_width - 30, 0, RCOLOR(3));
		gfx_draw_line(g, 15, 0, RCOLOR(2));
	}

	for (i = 1; i < 8; i++) {
		float tmp;
		gfx_set_text_cursor(g, margin_top + 1, 
							   margin_top + (8 - i) * division);
		tmp = ((max_y - min_y) / 8.0) * i + min_y;
		snprintf(value, sizeof(value),  "%4.1f", tmp);
		gfx_puts(g, value);
	}

	for (i = 1; i < 10; i++) {
		float tmp;
		int center_text;

		tmp = ((max_x - min_x) / 10.0) * i + min_x;
		snprintf(value, sizeof(value),  "%4.1f", tmp);
		center_text = gfx_get_string_width(g, value) / 2;
		gfx_set_text_cursor(g, margin_left + (i * division) - center_text,
							   2 * margin_top + box_height);
		gfx_puts(g, value);
	}
	t = gfx_get_string_width(g, xlabel);
	gfx_set_text_cursor(g, margin_left + box_width/2 - t/2,
		margin_top + box_height + gfx_get_text_height(g) + gfx_get_text_baseline(g));
	gfx_puts(g, xlabel);
	gfx_set_text_rotation(g, 90);
	t = gfx_get_string_width(g, ylabel);
	gfx_set_text_cursor(g, gfx_get_text_baseline(g),
				margin_top + box_height / 2 + t / 2);
	gfx_puts(g, ylabel);
	return res;
}
