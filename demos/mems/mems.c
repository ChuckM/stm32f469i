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
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <math.h>
#include <gfx.h>

#include "../util/util.h"
#include "dma2d.h"

typedef struct {
	GFX_CTX		*g;
	float		sx, sy;		/* X scale and Y scale */
	float		ox, oy;		/* offset X and offset Y */
	int			x, y, w, h;	/* box on the screen to use */
} GFX_VIEW;

void plot(GFX_VIEW *v, float x0, float y0, float x1, float y1, GFX_COLOR c);
GFX_VIEW *gfx_viewport(GFX_CTX *g, int x, int y, int w, int h, 
	float min_x, float min_y, float max_x, float max_y);

#define USE_VIEWPORT

GFX_VIEW __local_view;

#define MAP_X(vp, inX)	(int)((((inX) + (vp)->ox) / (vp)->sx) + (vp->x))
/* flip Y from 'natural' co-ordinates (+Y goes 'up') to 'display' (+y goes 'down') */
#define MAP_Y(vp, inY)	(int)((vp->h) - (((inY) + (vp)->oy) / (vp)->sy) + (vp->y))

/*
 * Create a transform that will scale between floating
 * point (x,y) co-ordinates into a region on the display
 * screen.
 */
GFX_VIEW *
gfx_viewport(GFX_CTX *g, int x, int y, int w, int h, 
	float min_x, float min_y, float max_x, float max_y)
{
	GFX_VIEW *res = &__local_view;
	memset(res, 0, sizeof(GFX_VIEW));
	res->g = g;
	res->x = x;
	res->y = y;
	res->w = w;
	res->h = h;
	
	res->sx = (max_x - min_x) / (float) w;
	res->sy = (max_y - min_y) / (float) h;
	res->ox = -min_x;
	res->oy = -min_y;
	printf("Computed view port:\n");
	printf("    (x, y, w, h) = (%d, %d, %d, %d)\n",
			x, y, w, h);
	printf("   (%f) X axis %f => %f, becomes %d => %d\n", res->sx,
		min_x, max_x, MAP_X(res, min_x), MAP_X(res, max_x));
	printf("   (%f) Y axis %f => %f, becomes %d => %d\n", res->sy,
		min_y, max_y, MAP_Y(res, min_y), MAP_Y(res, max_y));
	return &__local_view;
}

/*
 * Note we flip Y axes here to get more "intuitive" plotting.
 */
void
plot(GFX_VIEW *v, float x0, float y0, float x1, float y1, GFX_COLOR c)
{
	gfx_move_to(v->g, MAP_X(v, x0), MAP_Y(v, y0));
	gfx_draw_line_to(v->g, MAP_X(v, x1), MAP_Y(v, y1), c);
}

/*
 * MEMS Setup
 * -------------------------------------------------------
 */

#define DMA2D_SHADOW	(DMA2D_COLOR){.raw=0x80000000}
#define DMA2D_RED	(DMA2D_COLOR){ .raw=0xffff0000}
#define DMA2D_BLACK	(DMA2D_COLOR){.raw=0xff000000}
#define DMA2D_WHITE	(DMA2D_COLOR){.raw=0xffffffff}
#define DMA2D_GREY	(DMA2D_COLOR){.raw=0xff444444}

/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	6

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

#define RETIBUFFER (FRAMEBUFFER_ADDRESS - 0x200000U)
#define RCOLOR(c)	(GFX_COLOR){.raw=(uint32_t) c}
#define RET_WIDTH	800
#define RET_HEIGHT	480
#define RET_BPP		4
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
	reticule.bm.buf = (uint8_t *)(RETIBUFFER);
	memset((uint8_t *)(RETIBUFFER), 0, (RET_WIDTH * RET_HEIGHT)/2);
	reticule.bm.mode = DMA2D_L4;
	reticule.bm.stride = RET_WIDTH/2;
	reticule.bm.w = RET_WIDTH;
	reticule.bm.h = RET_HEIGHT;
	reticule.bm.maxc = 12;
	reticule.bm.clut = (uint32_t *)ret_colors;
	dma2d_clear((DMA2D_BITMAP *)&reticule, DMA2D_GREY);

	g = gfx_init(reticule_pixel, RET_WIDTH, RET_HEIGHT, GFX_FONT_LARGE, 
								(void *)&reticule);
	margin_top = gfx_get_text_height(g) + 1;
	margin_left = gfx_get_string_width(g, "-120") + margin_top + 1;
	margin_bottom = 2 * margin_top;
	box_width = RET_WIDTH - (margin_top + margin_left);
	box_height = RET_HEIGHT - (margin_top + margin_bottom);
	reticule.b_w = box_width;
	reticule.b_h = box_height;
	reticule.o_x = margin_left;
	reticule.o_y = margin_top;

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
	return (DMA2D_BITMAP *)&reticule;
}

extern void gen_data(int);
extern float r_x[];
extern float s_data[];
extern float i_x[];
extern float res_dft[];
extern int ri_len;

/*
 * Lets see if we can look at MEMs microphones
 */
int
main(void) {
	int i;
	float data_min, data_max;
	float peak;
	GFX_CTX	*g;
	GFX_VIEW *vp;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "MEMS Microphone Demo program\n");

	g = gfx_init(lcd_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *)FRAMEBUFFER_ADDRESS);
	dma2d_clear(&screen, DMA2D_GREY);
	(void) create_reticule();
	dma2d_render((DMA2D_BITMAP *)&reticule, &screen, 0, 0);
	gen_data(2);
#if 0
	/* generate the complex conjugate */
	for (i = 0; i < ri_len; i++) {
		i_x[i] = -i_x[i];
	}
#endif
	data_min = data_max = 0;
	for (i = 0; i < ri_len; i++) {
		peak = res_dft[i] = sqrt(r_x[i] * r_x[i] + i_x[i] * i_x[i]);
		data_min = (peak < data_min) ? peak : data_min;
		data_max = (peak > data_max) ? peak : data_max;
	}
	vp = gfx_viewport(g, reticule.o_x, reticule.o_y, reticule.b_w, reticule.b_h,
			0, data_min, (float) ri_len, data_max);
	for (i = 1; i < ri_len; i++) {
		plot(vp, i - 1, res_dft[i -1], i, res_dft[i], GFX_COLOR_YELLOW);
	}

	/* plot the original waveform */
	data_min = data_max = 0;
	for (i = 0; i < ri_len * 2; i++) {
		data_min = (s_data[i] < data_min) ? s_data[i] : data_min;
		data_max = (s_data[i] > data_max) ? s_data[i] : data_max;
	}
	vp = gfx_viewport(g, reticule.o_x, reticule.o_y, reticule.b_w, reticule.b_h,
			0, data_min, (float) ri_len * 2.0, data_max);
	for (i = 1; i < ri_len * 2; i++) {
		plot(vp, i - 1, s_data[i -1], i, s_data[i], GFX_COLOR_DKCYAN);
	}
	lcd_flip(0);
	while (1) {
	}
}
