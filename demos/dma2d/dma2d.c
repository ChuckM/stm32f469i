/*
 * DMA2D Demo
 *
 * Copyright (C) 2016 Chuck McManis, All rights reserved.
 *
 * This demo shows how the Bit Block Transfer (BitBlT)
 * peripheral known as DMA2D can be used to speed up
 * graphical sorts of things.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <gfx.h>

#include "../util/util.h"
#include "../util/helpers.h"

void draw_digit(GFX_CTX *g, int x, int y, int d, GFX_COLOR color, GFX_COLOR outline);
void draw_colon(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline);
void draw_dp(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline);
void dma2d_digit(int x, int y, int d, uint32_t color, uint32_t outline);
void generate_background(void);
void generate_digits(void);

/*
 *   This defines a parameterized 7 segment display graphic
 *   overall graphic is DISP_WIDTH pixels wide by DISP_HEIGHT
 *   pixels tall.
 *
 *   Segments are SEG_THICK pixels "thick".
 *
 *   Gaps between segments are SEG_GAP pixel(s)
 */

#define DISP_HEIGHT 100
#define DISP_WIDTH (DISP_HEIGHT / 2)
#define SEG_THICK 15
#define SEG_GAP 1

/* Vertical segment (6 co-ordinate pairs) */
static const int v_seg[] = {
/*A*/	0, 0,
/*B*/	SEG_THICK / 2, SEG_THICK / 2,
/*C*/	SEG_THICK / 2, DISP_HEIGHT / 2 - SEG_GAP,
/*D*/	0, DISP_HEIGHT / 2 - SEG_GAP,
/*E*/	-SEG_THICK / 2, ((DISP_HEIGHT / 2) - SEG_GAP) - (SEG_THICK / 2),
/*F*/	-SEG_THICK / 2, SEG_THICK / 2
};

/* Middle segment (6 co-ordinate pairs) */
static const int m_seg[] = {
/*A*/	0, 0,
/*B*/	SEG_THICK / 2, -SEG_THICK / 2,
/*C*/	SEG_THICK / 2 + (DISP_WIDTH - (2 * (SEG_THICK + SEG_GAP))), -SEG_THICK / 2,
/*D*/	(DISP_WIDTH - SEG_THICK - (2 * SEG_GAP)), 0,
/*E*/	SEG_THICK / 2 + (DISP_WIDTH - (2 * (SEG_THICK + SEG_GAP))), SEG_THICK / 2,
/*F*/	SEG_THICK / 2, SEG_THICK / 2
};

/* Horizontal segment (4 co-ordinate pairs) */
static const int h_seg[] = {
/*A*/	0, 0,
/*B*/	0, -SEG_THICK,
/*C*/	DISP_WIDTH - (2 * (SEG_THICK + SEG_GAP)), -SEG_THICK,
/*D*/	DISP_WIDTH - (2 * (SEG_THICK + SEG_GAP)), 0
};

/*
 * This array maps which segments are on for each digit
 * by having a '1' value if the segment is on, and a '0'
 * if the segment is off. The segments are identified
 * in the diagram below:
 *
 *        6
 *       ---
 *   5 / 4 / 3
 *     ---
 * 2 / 1 / 0
 *   ---
 *
 * Segments by bit position.
 */
const uint8_t seg_map[10] = {
	0b01101111,		/* 0 */
	0b00001001,		/* 1 */
	0b01011110,		/* 2 */
	0b01011011,		/* 3 */
	0b00111001,		/* 4 */
	0b01110011,		/* 5 */
	0b01110111,		/* 6 */
	0b01001001,		/* 7 */
	0b01111111,		/* 8 */
	0b01111001		/* 9 */
};

/*
 * This structure gives rendering parameters for each segment.
 * Which set of parameters to use (there are three unique sets)
 * and how to reflect them to meet their orientation requirements.
 * Each segment descriptor has a reference point (co-ordinate "A")
 */
static const struct {
	int	n_coords;	/* number of co-ordinates */
	int	xf;			/* X mirror factor */
	int	yf;			/* Y mirror factor */
	int	xo;			/* X offset */
	int	yo;			/* Y offset */
	const int	*segs;	/* pointer to segment co-ordinates */
} segment_data[7] = {
/*0*/	{ 6, -1, 1, DISP_WIDTH - SEG_THICK / 2, DISP_HEIGHT / 2 + SEG_GAP, &v_seg[0]},
/*1*/	{ 4, 1, 1, SEG_THICK + SEG_GAP, DISP_HEIGHT, &h_seg[0] },
/*2*/	{ 6, 1, 1, SEG_THICK / 2, DISP_HEIGHT / 2 + SEG_GAP, &v_seg[0]},
/*3*/	{ 6, -1, -1, DISP_WIDTH - SEG_THICK / 2, DISP_HEIGHT / 2 - SEG_GAP, &v_seg[0]},
/*4*/	{ 6, 1, 1, SEG_THICK / 2 + SEG_GAP, DISP_HEIGHT / 2, &m_seg[0]},
/*5*/	{ 6, 1, -1, SEG_THICK/2, DISP_HEIGHT/2 - SEG_GAP, &v_seg[0] },
/*6*/	{ 4, 1, 1, SEG_THICK + SEG_GAP, SEG_THICK, &h_seg[0] },
};

#define SKEW_MAX (DISP_WIDTH / 3)
/*
 * Compute a "skew factor" so the display leans right
 * Takes x (computed x), dy (distance from top), and
 * display height (max distance from top). Slant goes
 * from 0 at dy == h to SKEW_MAX when dy == 0
 */
static int
skew_factor(int dy, int h)
{
	int i;
	/* dy is distance from the 'top' of the display box */
	i =  ((SKEW_MAX * (h - dy)) / h);
	return i;
}

/* gap question, is disp_height everything, including gaps? (same with display width?) */
/* How about display pad, for the "socket" around the display? */
/* How about the decimal point with respect to the digits? */
#define OUTLINE_DIGIT_BOX

/*
 * Draw a graphic that looks like a 7 segment display
 * digit. (yes its retro, ok?)
 */
void
draw_digit(GFX_CTX *g, int x, int y, int d, GFX_COLOR color, GFX_COLOR outline) {
	int i;
	uint8_t seg_mask;
	int	sx, sy, ndx, xf, yf;

	seg_mask = seg_map[d % 10];

#ifdef OUTLINE_DIGIT_BOX
	gfx_draw_line_at(g,
		skew_factor(0, DISP_HEIGHT) + x - 1,
		y - 1,
		skew_factor(0, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y - 1, GFX_COLOR_BLUE);
	gfx_draw_line_at(g,
		skew_factor(0, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y - 1,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y + DISP_HEIGHT + 2, GFX_COLOR_BLUE);
	gfx_draw_line_at(g,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y + DISP_HEIGHT + 2,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x - 1,
		y + DISP_HEIGHT + 2, GFX_COLOR_BLUE);
	gfx_draw_line_at(g,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x - 1,
		y + DISP_HEIGHT + 2,
		skew_factor(0, DISP_HEIGHT) + x - 1,
		y - 1, GFX_COLOR_BLUE);
#endif

	for (i = 0; i < 7; i++) {
		if ((seg_mask & (1 << i)) != 0) {
			/* draw segment */
			sx = x + segment_data[i].xo;
			sy = y + segment_data[i].yo;
			xf = segment_data[i].xf;
			yf = segment_data[i].yf;
			/* tesselate it */
			for (ndx = 2; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_fill_triangle_at(g,
					skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs) * xf,
					sy + *(segment_data[i].segs + 1) * yf,			/* #1 */
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx	+ *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,	/* #2 */
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
					     sx + *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,	/* #3 */
							 color);
			}
			/* outline it */
			for (ndx = 0; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_draw_line_at(g,
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx + *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
								sx	+ *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,
								outline);
			}
			gfx_draw_line_at(g,
				skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + 0) * xf,
				sy + *(segment_data[i].segs + 1) * yf,
				skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + ndx) * xf,
				sy + *(segment_data[i].segs + ndx + 1) * yf,
							outline);
		} else {
			/* draw 'off' segment? */
#ifdef DRAW_DARK_SEGMENT
			/* draw segment */
			sx = x + segment_data[i].xo;
			sy = y + segment_data[i].yo;
			xf = segment_data[i].xf;
			yf = segment_data[i].yf;
			/* tesselate it */
			for (ndx = 2; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_fill_triangle_at(g,
					skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs) * xf,
					sy + *(segment_data[i].segs + 1) * yf,			/* #1 */
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx	+ *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,	/* #2 */
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
					     sx + *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,	/* #3 */
							(uint16_t) (6 << 11 | 6 << 5 | 6)
							 );
			}
			/* outline it */
			for (ndx = 0; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_draw_line_at(g,
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx + *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
								sx	+ *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,
							outline);
			}
			gfx_draw_line_at(g,
				skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + 0) * xf,
				sy + *(segment_data[i].segs + 1) * yf,
				skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + ndx) * xf,
				sy + *(segment_data[i].segs + ndx + 1) * yf,
							outline);
#endif
		}
	}
}

/* Colon spread as a percentage of display height */
#define COLON_SPREAD (DISP_HEIGHT * 15 / 100)

/*
 * For 'clock' type displays they have a colon between
 * the Hours, minutes, and seconds. This is our LED
 * colon (:) character.
 *
 * Started putting the 'dots' at 25% and 75% of display
 * height, and switched to equidistant from the display
 * vertical mid-point.
 */
void
draw_colon(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline)
{
	gfx_fill_circle_at(g,
		skew_factor(DISP_HEIGHT / 2 - COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 - COLON_SPREAD) - SEG_THICK/2, SEG_THICK/2, color);
	gfx_draw_circle_at(g,
		skew_factor(DISP_HEIGHT / 2 - COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 - COLON_SPREAD) - SEG_THICK/2, SEG_THICK/2, outline);
	gfx_fill_circle_at(g,
		skew_factor(DISP_HEIGHT / 2 + COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 + COLON_SPREAD) + SEG_THICK/2, SEG_THICK/2, color);
	gfx_draw_circle_at(g,
		skew_factor(DISP_HEIGHT / 2 + COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 + COLON_SPREAD) + SEG_THICK/2, SEG_THICK/2, outline);
}

/*
 * For displays with a decimal point, draw the decimal point
 */
void
draw_dp(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline)
{
	gfx_fill_circle_at(g,
		skew_factor(DISP_HEIGHT - (SEG_THICK/2), DISP_HEIGHT) + x + SEG_THICK/2,
		y + DISP_HEIGHT - SEG_THICK/2, SEG_THICK/2, color);
	gfx_draw_circle_at(g,
		skew_factor(DISP_HEIGHT - (SEG_THICK/2), DISP_HEIGHT) + x + SEG_THICK/2,
		y + DISP_HEIGHT - SEG_THICK/2, SEG_THICK/2, outline);
}

void display_clock(GFX_CTX *g, int x, int y, uint32_t tm);
void dma2d_clock(int x, int y, uint32_t tm, int ds);

void
display_clock(GFX_CTX *g, int x, int y, uint32_t tm)
{
	int hh, mm, ss, ms;

	hh = (int) tm / 3600000;
	mm = (int) (tm % 3600000) / 60000;
	ss = (int) (tm % 60000) / 1000;
	ms = (int) tm % 1000;
	draw_digit(g, x, y, hh / 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(g, x, y, hh % 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_colon(g, x, y, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += SEG_THICK + SEG_THICK/2;;
	draw_digit(g, x, y, mm / 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(g, x, y, mm % 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_colon(g, x, y, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += SEG_THICK + SEG_THICK/2;;
	draw_digit(g, x, y, ss / 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(g, x, y, ss % 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_dp(g, x, y, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += SEG_THICK + SEG_THICK/2;
	draw_digit(g, x, y, ms / 100, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(g, x, y, ms / 10, GFX_COLOR_RED, GFX_COLOR_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(g, x, y, ms, GFX_COLOR_RED, GFX_COLOR_BLACK);
}

#define SHADOW 0x80000000
#define DMA2D_RED	0xffff0000
#define DMA2D_BLACK	0xff000000

void
dma2d_clock(int x, int y, uint32_t tm, int ds)
{
	int hh, mm, ss, ms;

	hh = (int) tm / 3600000;
	mm = (int) (tm % 3600000) / 60000;
	ss = (int) (tm % 60000) / 1000;
	ms = (int) tm % 1000;
	if (ds) {
		dma2d_digit(x+10, y+10, hh / 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, hh / 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, hh % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, hh % 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, 10, DMA2D_RED, DMA2D_BLACK);
	x += SEG_THICK + SEG_THICK/2;;
	if (ds) {
		dma2d_digit(x+10, y+10, mm / 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, mm / 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, mm % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, mm % 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, 10, DMA2D_RED, DMA2D_BLACK);
	x += SEG_THICK + SEG_THICK/2;;
	if (ds) {
		dma2d_digit(x+10, y+10, ss / 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, ss / 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, ss % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, ss % 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, 11, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, 11, DMA2D_RED, DMA2D_BLACK);
	x += SEG_THICK + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, (ms / 100) % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, (ms / 100) % 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, (ms / 10) % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, (ms / 10) % 10, DMA2D_RED, DMA2D_BLACK);
	x += DISP_WIDTH + SEG_THICK/2;
	if (ds) {
		dma2d_digit(x+10, y+10, ms % 10, SHADOW, SHADOW);
	}
	dma2d_digit(x, y, ms % 10, DMA2D_RED, DMA2D_BLACK);
}

/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	6

void dma2d_bgfill(void);
void dma2d_fill(uint32_t color);

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
#define GRID_BG		COLOR(0xff, 0xff, 0xff)	/* white */
#define LIGHT_GRID	COLOR(0xc0, 0xc0, 0xff)	/* light blue */
#define DARK_GRID	COLOR(0xc0, 0xc0, 0xe0)	/* dark blue */

void bg_draw_pixel(void *, int, int, GFX_COLOR);

/* add pointer to frame buffer, and change to GFX_COLOR with
 * .raw being the holder for the color.
 */
/* Our own pixel writer for the background stuff */
void
bg_draw_pixel(void *fb, int x, int y, GFX_COLOR color)
{
	*((uint32_t *) fb + (y * 800) + x) = color.raw;
}

/*
 * This generates a "pleasant" background which looks a bit
 * like graph paper.
 */
void
generate_background(void)
{
	int i, x, y;
	uint32_t *t;
	GFX_CTX local_ctx;
	GFX_CTX	*g;

	g = gfx_init(&local_ctx, bg_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *) BACKGROUND_FB);
	for (i = 0, t = (uint32_t *)(BACKGROUND_FB); i < 800 * 480; i++, t++) {
		*t = 0xffffffff; /* clear to white */
	}
	/* draw a grid */
	t = (uint32_t *) BACKGROUND_FB;
	for (y = 1; y < 480; y++) {
		for (x = 1; x < 800; x++) {
			/* major grid line */
			if ((x % 50) == 0) {
				gfx_draw_point_at(g, x-1, y, DARK_GRID);
				gfx_draw_point_at(g, x, y, DARK_GRID);
				gfx_draw_point_at(g, x+1, y, DARK_GRID);
			} else if ((y % 50) == 0) {
				gfx_draw_point_at(g, x, y-1, DARK_GRID);
				gfx_draw_point_at(g, x, y, DARK_GRID);
				gfx_draw_point_at(g, x, y+1, DARK_GRID);
			} else if (((x % 25) == 0) || ((y % 25) == 0)) {
				gfx_draw_point_at(g, x, y, LIGHT_GRID);
			}
		}
	}
	gfx_draw_rounded_rectangle_at(g, 0, 0, 800, 480, 15, DARK_GRID);
	gfx_draw_rounded_rectangle_at(g, 1, 1, 798, 478, 15, DARK_GRID);
	gfx_draw_rounded_rectangle_at(g, 2, 2, 796, 476, 15, DARK_GRID);
}

/*
 * This should be a data buffer which can hold 10 digits,
 * a colon, and a decimal point, note that pixels in this
 * buffer are one byte each.
 */
void digit_draw_pixel(void *, int x, int y, GFX_COLOR color);
void print_digit(int n);

#define DIGIT_FB_WIDTH 	((DISP_WIDTH + SKEW_MAX) * 10 + (SEG_THICK + SKEW_MAX) * 2)
#define DIGIT_FB_HEIGHT (DISP_HEIGHT)

uint8_t digit_fb[DIGIT_FB_WIDTH * DIGIT_FB_HEIGHT];

/* This is the simple graphics helper function to draw into it */
void
digit_draw_pixel(void *fb, int x, int y, GFX_COLOR color)
{
	uint8_t	c = (uint8_t) color.raw;
	*((uint8_t *)fb + y * DIGIT_FB_WIDTH + x) = c;
}

/*
 * This then will render a digit to the console, its good for
 * debugging.
 */
void
print_digit(int n) {
	uint8_t *row;
	unsigned int tx, ty;

	printf("Digit FB size (W, H) = (%d, %d)\n", DIGIT_FB_WIDTH, DIGIT_FB_HEIGHT);
	row = (uint8_t *)&digit_fb[(n % 10) * (DISP_WIDTH+SKEW_MAX)];
	for (ty = 0; ty < DISP_HEIGHT; ty++) {
		for (tx = 0; tx < (DISP_WIDTH + SKEW_MAX); tx++) {
			switch (*(row + tx)) {
			case 0:
				console_putc(' ');
				break;
			case 1:
				console_putc('*');
				break;
			case 2:
				console_putc('.');
				break;
			default:
				console_putc('X');
				break;
			}
		}
		printf("\n");
		row += DIGIT_FB_WIDTH;
	}
	/* END DEBUG CODE */
}

#define DIGIT_OUTLINE_COLOR	(GFX_COLOR){.raw=2}
#define DIGIT_BODY_COLOR	(GFX_COLOR){.raw=1}

/*
 * And this is where we will generate our digits
 * when we copy them with the DMA2D device we can
 * use the color lookup table to set the values.
 * in this case we'll use 0, 1, and 2 for background
 * foreground, and outline color.
 *
 * While the digits are kerned in the display they are
 * drawn here with a box that is DISP_WIDTH + SKEW_MAX
 * pixels wide, but DISP_HEIGHT pixels high.
 */
void
generate_digits(void)
{
	GFX_CTX	local_gfx;
	GFX_CTX	*g;
	uint32_t i;
	g = gfx_init(&local_gfx, digit_draw_pixel, 
		DIGIT_FB_WIDTH, DIGIT_FB_HEIGHT, GFX_FONT_LARGE, (void *)digit_fb);

	/* Cleared to zero (background) */
	for (i = 0; i < sizeof(digit_fb); i++) {
		digit_fb[i] = 0;
	}
	for (i = 0; i < 10; i++) {
		draw_digit(g, i * (DISP_WIDTH + SKEW_MAX), 0, i, DIGIT_BODY_COLOR, DIGIT_OUTLINE_COLOR);
	}
	draw_colon(g, 10 * (DISP_WIDTH + SKEW_MAX), 0, DIGIT_BODY_COLOR, DIGIT_OUTLINE_COLOR);
	draw_dp(g, 10 * (DISP_WIDTH + SKEW_MAX) + SEG_THICK + SKEW_MAX, 0,
					DIGIT_BODY_COLOR, DIGIT_OUTLINE_COLOR);
	/* now the digit_fb memory has the 10 digits 0-9, : and . in it */
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

void
dma2d_digit(int x, int y, int d, uint32_t color, uint32_t outline)
{
	uint32_t t;
	int	w;

	while (DMA2D_CR & DMA2D_CR_START);
	/* This is going to be a memory to memory with PFC transfer */
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2MWB);

	*(DMA2D_FG_CLUT) = 0x0; /* transparent black */
	*(DMA2D_FG_CLUT+1) = color; /* foreground */
	*(DMA2D_FG_CLUT+2) = outline; /* outline color */

	/* compute target address */
	t = (uint32_t) FRAMEBUFFER_ADDRESS + (800 * 4 * y) + x * 4;
	/* Output goes to the main frame buffer */
	DMA2D_OMAR = t;
	/* Its also the pixels we want to read incase the digit is
	 * transparent at that point
	 */
	DMA2D_BGMAR = t;
	DMA2D_BGPFCCR = DMA2D_SET(xPFCCR, CM, DMA2D_xPFCCR_CM_ARGB8888) |
					DMA2D_SET(xPFCCR, AM, 0);

	/* output frame buffer is ARGB8888 */
	DMA2D_OPFCCR = 0; /* DMA2D_OPFCCR_CM_ARGB8888; */

	/*
	 * This sets the size of the "box" we're going to copy. For the
	 * digits it is DISP_WIDTH + SKEW_MAX pixels wide for the ':' and
	 * '.' character is will be SEG_THICK + SKEW_MAX wide, it is always
	 * DISP_HEIGHT tall.
	 */
	if (d < 10) {
		w = DISP_WIDTH + SKEW_MAX;
		/* This points to the top left corner of the digit */
		t = (uint32_t) &(digit_fb[(d % 10) * (DISP_WIDTH + SKEW_MAX)]);
	} else if (d == 10) {
		/* colon comes just past the 10 digits */
		w = SEG_THICK + SKEW_MAX;
		t = (uint32_t) &(digit_fb[10 * (DISP_WIDTH + SKEW_MAX)]);
	} else {
		/* the decimal point is the character after colon */
		w = SEG_THICK + SKEW_MAX;
		t = (uint32_t) &(digit_fb[10 * (DISP_WIDTH + SKEW_MAX) + (w)]);
	}
	/* So this then describes the box size */
	DMA2D_NLR = DMA2D_SET(NLR, PL, w) | DISP_HEIGHT;
	/*
	 * This is how many additional pixels we need to move to get to
	 * the next line of output.
	 */
	DMA2D_OOR = 800 - w;
	/*
	 * This is how many additional pixels we need to move to get to
	 * the next line of background (which happens to be the output
	 * so it is the same).
	 */
	DMA2D_BGOR = 800 - w;
	/*
	 * And finally this is the additional pixels we need to move
	 * to get to the next line of the pre-rendered digit buffer.
	 */
	DMA2D_FGOR = DIGIT_FB_WIDTH - w;

	/*
	 * And this points to the top left corner of the prerendered
	 * digit buffer, where the digit (or character) top left
	 * corner is.
	 */
	DMA2D_FGMAR = t;

	/* Set up the foreground data descriptor
	 *    - We are only using 3 of the colors in the lookup table (CLUT)
	 *	  - We don't set alpha since it is in the lookup table
	 *	  - Alpha mode is 'don't modify' (00)
	 *	  - CLUT color mode is ARGB8888 (0)
	 *	  - Color Mode is L8 (0101) or one 8 byte per pixel
	 *
	 */
	DMA2D_FGPFCCR = DMA2D_SET(xPFCCR, CS, 255) |
					DMA2D_SET(xPFCCR, ALPHA, 255) |
					DMA2D_SET(xPFCCR, AM, 0) |
					DMA2D_SET(xPFCCR, CM, DMA2D_xPFCCR_CM_L8);
	/*
	 * Transfer it!
	 */
	DMA2D_CR |= DMA2D_CR_START;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error\n");
		while (1);
	}
}

const char *demo_options[] = {
	 "Manual Clearing",
	 "Tight loop Clearing",
	 "DMA2D One color clear",
	 "DMA2D Background copy clear",
	 "DMA2D Background pattern with digits",
	 "DMA2D Background pattern with drop shadowed digits",
	 "Extra string",
	 "Extra string"
};

#define N_FRAMES	10
uint32_t frame_times[N_FRAMES] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int te_lock = 1;

static void
draw_pixel(void *buf, int x, int y, GFX_COLOR c)
{
	lcd_draw_pixel(buf, x, y, c.raw);
}

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
	int	i;
	uint32_t t1, t0;
	char buf[35];
	char *scr_opt;
	int f_ndx = 0;
	float avg_frame;
	int	can_switch;
	int	opt, ds;
	GFX_CTX local_context;
	GFX_CTX *g;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "DMA2D Demo program : Digits Gone Wild\n");
	printf("Generate background\n");
	generate_background();
	printf("Generate digits\n");
	generate_digits();

	g = gfx_init(&local_context, draw_pixel, 800, 480, GFX_FONT_LARGE, (void *)FRAMEBUFFER_ADDRESS);
	opt = 0; /* screen clearing mode */
	can_switch = 0; /* auto switching every 10 seconds */
	t0 = mtime();
	ds = 0;
/******* TESTING CODE ********/
	gfx_fill_screen(g, GFX_COLOR_WHITE);
	draw_digit(g, 100, 100, 8, GFX_COLOR_RED, GFX_COLOR_BLACK);

	/* In both cases we write the notes using the graphics library */
	gfx_fill_triangle_at(g, 300, 300, 300, 200, 400, 200, GFX_COLOR_GREEN);
	gfx_draw_triangle_at(g, 300, 300, 300, 200, 400, 200, GFX_COLOR_RED);
	gfx_set_text_color(g, GFX_COLOR_BLACK, GFX_COLOR_BLACK);
	gfx_set_text_size(g, 3);
	gfx_set_text_cursor(g, 25, 30 + DISP_HEIGHT + gfx_get_text_height(g) + 2);
	gfx_puts(g, (char *)"Hello world for DMA2D!");
	lcd_flip(te_lock);
	while (1) ;
/****************** ####### ***********/
	
	while (1) {
		switch (opt) {
		default:
		case 0:
			/* very slow way to clear the screen */
			scr_opt = "manual clear";
			gfx_fill_screen(g, GFX_COLOR_WHITE);
			break;
		case 1:
			/* faster, using a tight loop */
			scr_opt = "dedicated loop";
			lcd_clear(0xff7f7f);
			break;
		case 2:
			/* fastest? Using DMA2D to fill screen */
			scr_opt = "DMA 2D Fill";
			dma2d_fill(0xff7fff7f);
			break;
		case 3:
			/* still fast, using DMA2D to pre-populate BG */
			scr_opt = "DMA 2D Background";
			dma2d_bgfill();
			break;
		case 4:
			/* Now render all of the digits with DMA2D */
			scr_opt = "DMA 2D Digits";
			ds = 0;
			dma2d_bgfill();
			break;
		case 5:
			/* still fast, using DMA2D to render drop shadows */
			scr_opt = "DMA 2D Shadowed Digits";
			ds = 1;
			dma2d_bgfill();
			break;
		}

		/* This little state machine implements an automatic switch
		 * every 10 seconds unless you've disabled it with the 'd'
		 * command.
		 */
		if (((t0 / 1000) % 10 == 0) && (can_switch > 0)) {
			opt = (opt + 1) % MAX_OPTS;
			can_switch = 0;
		} else  if (((t0 / 1000) % 10 != 0) && (can_switch == 0)) {
			can_switch = 1;
		}

		/*
		 * The first four options (0, 1, 2, 3) all render the digits
		 * in software every time, options 4 and 5 use the DMA2
		 * device to render the digits
		 */
		if (opt < 4) {
			display_clock(g, 25, 20, mtime());
		} else {
			dma2d_clock(25, 20, mtime(), ds);
		}
		for (i = 0; i < 10; i++) {
			if (opt < 4) {
				draw_digit(g, 25 + i * (DISP_WIDTH + 8), 350, i, GFX_COLOR_GREEN, GFX_COLOR_BLACK);
			} else {
				if (ds) {
					dma2d_digit(35 + i * (DISP_WIDTH + 8), 360, i, SHADOW, SHADOW);
				}
				dma2d_digit(25 + i * (DISP_WIDTH + 8), 350, i, 0xff40c040, 0xff000000);
			}
		}

		/* In both cases we write the notes using the graphics library */
		gfx_set_text_color(g, GFX_COLOR_BLACK, GFX_COLOR_BLACK);
		gfx_set_text_size(g, 3);
		gfx_set_text_cursor(g, 25, 30 + DISP_HEIGHT + gfx_get_text_height(g) + 2);
		gfx_puts(g, (char *)"Hello world for DMA2D!");
		lcd_flip(te_lock);
		t1 = mtime();

		/* this computes a running average of the last 10 frames */
		frame_times[f_ndx] = t1 - t0;
		f_ndx = (f_ndx + 1) % N_FRAMES;
		for (i = 0, avg_frame = 0; i < N_FRAMES; i++) {
			avg_frame += frame_times[i];
		}
		avg_frame = avg_frame / (float) N_FRAMES;
		snprintf(buf, 35, "FPS: %6.2f", 1000.0 / avg_frame);
		gfx_set_text_cursor(g, 25, 30 + DISP_HEIGHT + 2 * (gfx_get_text_height(g) + 2));
		gfx_puts(g, (char *)buf);
		gfx_puts(g, (char *)" ");
		gfx_set_text_cursor(g, 25, 30 + DISP_HEIGHT + 3 * (gfx_get_text_height(g) + 2));
		gfx_puts(g, (char *)scr_opt);
		/*
		 * The demo runs continuously but it watches for characters
		 * typed at the console. There are a few options you can select.
		 */
		i = console_getc(0);
		switch (i) {
		case 's':
			opt = (opt + 1) % MAX_OPTS;
			printf("Switched to : %s\n", demo_options[opt]);
			break;
		case 'd':
			can_switch = -1;
			printf("Auto switching disabled\n");
			break;
		case 'e':
			can_switch = 0;
			printf("Auto switching enabled\n");
			break;
		case 't':
			te_lock = (te_lock == 0);
			printf("We are %s for the TE bit to be set\n", (te_lock) ? "WAITING" : "NOT WAITING");
			break;
		default:
			printf("Options:\n");
			printf("\ts - switch demo mode\n");
			printf("\td - disable auto-switching of demo mode\n");
			printf("\te - enable auto-switching of demo mode\n");
			printf("\tt - enable/disable Tearing effect lock wait\n");
		case 0:
			break;
		}
		t0 = t1;
	}
}
