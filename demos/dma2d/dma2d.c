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

void draw_digit(int x, int y, int d, uint16_t color);
void draw_colon(int x, int y, uint16_t color);
void draw_dp(int x, int y, uint16_t color);
void generate_background(void);

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
/*E*/	- SEG_THICK / 2, (( DISP_HEIGHT / 2) - SEG_GAP) - (SEG_THICK / 2),
/*F*/	- SEG_THICK / 2, SEG_THICK / 2
};

/* Middle segment (6 co-ordinate pairs) */
static const int m_seg[] = {
/*A*/	0, 0,
/*B*/	SEG_THICK / 2, - SEG_THICK / 2,
/*C*/	SEG_THICK / 2 + (DISP_WIDTH - (2 * (SEG_THICK + SEG_GAP))), - SEG_THICK / 2,
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
const uint8_t seg_map [10] = {
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
//	printf("   ... dy = %d, skew = %d\n", dy, i);
	return i;
}

/* gap question, is disp_height everything, including gaps? (same with display width?) */
/* How about display pad, for the "socket" around the display? */
/* How about the decimal point with respect to the digits? */

/*
 * Draw a graphic that looks like a 7 segment display
 * digit. (yes its retro, ok?)
 */
void
draw_digit(int x, int y, int d, uint16_t color) {
	int i;
	uint8_t seg_mask;
	int	sx, sy, ndx, xf, yf;

	seg_mask = seg_map[d % 10];

#ifdef OUTLINE_DIGIT_BOX
	gfx_drawLine(
		skew_factor(0, DISP_HEIGHT) + x - 1,
 	    y - 1,
		skew_factor(0, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y - 1, GFX_COLOR_BLUE);
	gfx_drawLine(
		skew_factor(0, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y - 1,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y + DISP_HEIGHT + 2, GFX_COLOR_BLUE);
	gfx_drawLine(
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x + DISP_WIDTH+2,
		y + DISP_HEIGHT + 2,
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x - 1,
		y + DISP_HEIGHT + 2, GFX_COLOR_BLUE);
	gfx_drawLine(
		skew_factor(DISP_HEIGHT, DISP_HEIGHT) + x - 1,
		y + DISP_HEIGHT + 2,
		skew_factor(0, DISP_HEIGHT) + x - 1,
 	    y - 1, GFX_COLOR_BLUE);
#endif

	for (i = 0; i < 7; i++) {
		if ((seg_mask & (1 << i)) != 0) {
			// draw segment
			sx = x + segment_data[i].xo;
			sy = y + segment_data[i].yo;
			xf = segment_data[i].xf;
			yf = segment_data[i].yf;
			/* tesselate it */
			for (ndx = 2; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_fillTriangle(
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
				gfx_drawLine(
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx + *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
					    	sx	+ *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,
							GFX_COLOR_BLACK);
			}
			gfx_drawLine(
				skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + 0) * xf,
				sy + *(segment_data[i].segs + 1) * yf,
				skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + ndx) * xf,
				sy + *(segment_data[i].segs + ndx + 1) * yf,
							GFX_COLOR_BLACK);
		} else {
			// draw 'off' segment?
#ifdef DRAW_DARK_SEGMENT
			// draw segment
			sx = x + segment_data[i].xo;
			sy = y + segment_data[i].yo;
			xf = segment_data[i].xf;
			yf = segment_data[i].yf;
			/* tesselate it */
			for (ndx = 2; ndx < (segment_data[i].n_coords - 1) * 2; ndx += 2) {
				gfx_fillTriangle(
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
				gfx_drawLine(
					skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
						sx + *(segment_data[i].segs + ndx) * xf,
					sy + *(segment_data[i].segs + ndx + 1) * yf,
					skew_factor((sy + *(segment_data[i].segs + ndx + 3) * yf - y), DISP_HEIGHT) +
					    	sx	+ *(segment_data[i].segs + ndx + 2) * xf,
					sy + *(segment_data[i].segs + ndx + 3) * yf,
							GFX_COLOR_BLACK);
			}
			gfx_drawLine(
				skew_factor((sy + *(segment_data[i].segs + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + 0) * xf,
				sy + *(segment_data[i].segs + 1) * yf,
				skew_factor((sy + *(segment_data[i].segs + ndx + 1) * yf - y), DISP_HEIGHT) +
							sx + *(segment_data[i].segs + ndx) * xf,
				sy + *(segment_data[i].segs + ndx + 1) * yf,
							GFX_COLOR_BLACK);
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
draw_colon(int x, int y, uint16_t color)
{
	gfx_fillCircle(
		skew_factor(DISP_HEIGHT / 2 - COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 - COLON_SPREAD) - SEG_THICK/2, SEG_THICK/2, color);
	gfx_drawCircle(
		skew_factor(DISP_HEIGHT / 2 - COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 - COLON_SPREAD) - SEG_THICK/2, SEG_THICK/2, GFX_COLOR_BLACK);
	gfx_fillCircle(
		skew_factor(DISP_HEIGHT / 2 + COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 + COLON_SPREAD) + SEG_THICK/2, SEG_THICK/2, color);
	gfx_drawCircle(
		skew_factor(DISP_HEIGHT / 2 + COLON_SPREAD, DISP_HEIGHT) + x + SEG_THICK/2,
		y + (DISP_HEIGHT / 2 + COLON_SPREAD) + SEG_THICK/2, SEG_THICK/2, GFX_COLOR_BLACK);
}

/*
 * For displays with a decimal point, draw the decimal point
 */
void
draw_dp(int x, int y, uint16_t color)
{
	gfx_fillCircle(
		skew_factor(DISP_HEIGHT - (SEG_THICK/2), DISP_HEIGHT) + x + SEG_THICK/2,
		y + DISP_HEIGHT - SEG_THICK/2, SEG_THICK/2, color);
	gfx_drawCircle(
		skew_factor(DISP_HEIGHT - (SEG_THICK/2), DISP_HEIGHT) + x + SEG_THICK/2,
		y + DISP_HEIGHT - SEG_THICK/2, SEG_THICK/2, GFX_COLOR_BLACK);
}

void display_clock(int x, int y, uint32_t tm);

void
display_clock(int x, int y, uint32_t tm)
{
	int hh, mm, ss, ms;

	hh = (int) tm / 3600000;
	mm = (int) (tm % 3600000) / 60000;
	ss = (int) (tm % 60000) / 1000;
	ms = (int) tm % 1000;
	draw_digit(x, y, hh / 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(x, y, hh % 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_colon(x, y, GFX_COLOR_RED);
	x += SEG_THICK + SEG_THICK/2;;
	draw_digit(x, y, mm / 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(x, y, mm % 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_colon(x, y, GFX_COLOR_RED);
	x += SEG_THICK + SEG_THICK/2;;
	draw_digit(x, y, ss / 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(x, y, ss % 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_dp(x, y, GFX_COLOR_RED);
	x += SEG_THICK + SEG_THICK/2;
	draw_digit(x, y, ms / 100, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(x, y, ms / 10, GFX_COLOR_RED);
	x += DISP_WIDTH + SEG_THICK/2;
	draw_digit(x, y, ms, GFX_COLOR_RED);
}

/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	4

void dma2d_bgfill(void);
void dma2d_fill(uint32_t color);

/*
 * Copy in a the background first
 */
void
dma2d_bgfill(void)
{
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		DMA2D_IFCR |= 0x3F;
		if (DMA2D_ISR & DMA2D_ISR_CEIF) {
			printf("Failed to clear configuration error\n");
			while(1);
		}
	}

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
	while ((DMA2D_CR & DMA2D_CR_START)) ;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error!\n");
		while (1);
	}
	
}

/*
 * Use the DMA2D peripheral in register to memory
 * mode to clear the frame buffer to a single
 * color.
 */
void
dma2d_fill(uint32_t color)
{
	
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		DMA2D_IFCR |= 0x3F;
		if (DMA2D_ISR & DMA2D_ISR_CEIF) {
			printf("Failed to clear configuration error\n");
			while(1);
		}
	}
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
	DMA2D_OPFCCR = 0x0; /* ARGB8888 pixels */
	/* force it to have full alpha */
	DMA2D_OCOLR = 0xff000000 | color;
	DMA2D_OOR =	0;
	DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
	DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while ((DMA2D_CR & DMA2D_CR_START)) ;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error!\n");
		while (1);
	}
}

#define GRID_BG		0xffffff	/* white */
#define LIGHT_GRID	0xffc0c0ff	/* light blue */
#define DARK_GRID	0xffc0c0e0	/* dark blue */

void bg_draw_pixel(int, int, uint16_t);

/* Our own pixel writer for the background stuff */
void
bg_draw_pixel(int x, int y, uint16_t color)
{
	uint32_t c;
	uint32_t *fb = (uint32_t *) BACKGROUND_FB;
	c = GRID_BG;
	switch (color) {
		default:
			c = GRID_BG;
			break;
		case 1:
			c = LIGHT_GRID;
			break;
		case 2:
			c = DARK_GRID;
			break;
	}
	*(fb + (y * 800) + x) = c;
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
				gfx_drawPixel(x-1, y, 2);
				gfx_drawPixel(x, y, 2);
				gfx_drawPixel(x+1, y, 2);
			} else if ((y % 50) == 0) {
				gfx_drawPixel(x, y-1, 2);
				gfx_drawPixel(x, y, 2);
				gfx_drawPixel(x, y+1, 2);
			} else if (((x % 25) == 0) || ((y % 25) == 0)) {
				gfx_drawPixel(x, y, 1);
			}
		}
	}
	gfx_drawRoundRect(0, 0, 800, 480, 15, 2);
	gfx_drawRoundRect(1, 1, 798, 478, 15, 2);
	gfx_drawRoundRect(2, 2, 796, 476, 15, 2);
}

const char *demo_options[] = {
	 "Manual Clearing",
	 "Tight loop Clearing",
	 "DMA2D One color clear",
	 "DMA2D Background copy clear"
};

#define N_FRAMES	10
uint32_t frame_times[N_FRAMES] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
	int	i;
	uint32_t t1, t0;
	char buf[35];
	char *scr_opt;
	int f_ndx = 0;
	float avg_frame;
	int	can_switch;
	int	opt;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "DMA2D Demo program (triangle test 2)\n");
	printf("Generate background\n");
	generate_background();
	gfx_init(lcd_draw_pixel, 800, 480, GFX_FONT_LARGE);

	opt = 0; /* screen clearing mode */
	can_switch = 0; /* auto switching every 10 seconds */
	t0 = mtime();
	while (1) {
		switch (opt) {
			default:
			case 0:
				/* very slow way to clear the screen */
				scr_opt = "manual clear";
				gfx_fillScreen(0xffff);
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
		}

		/* This little state machine implements an automatic switch
		 * every 10 seconds unless you've disabled it with the 'd'
		 * command.
		 */
		if (((t0 / 1000) % 10 == 0) && (can_switch > 0)) {
			opt = (opt + 1) % MAX_OPTS;
			can_switch = 0;
		} else  if (((t0 / 1000) % 10 != 0) && (can_switch == 0)){
			can_switch = 1;
		}
		display_clock(25, 20, mtime());
		for (i = 0; i < 10; i++) {
			draw_digit(25 + i * (DISP_WIDTH + 8), 350, i, (uint16_t)(24 << 5));
		}
		gfx_setTextColor(0, 0);
		gfx_setTextSize(4);
		gfx_setCursor(25, 20 + DISP_HEIGHT + gfx_getTextHeight() + 2);
		gfx_puts((unsigned char *)"Hello world for DMA2D!");
		lcd_flip(te_lock);
		t1 = mtime();
		frame_times[f_ndx] = t1 - t0;
		f_ndx = (f_ndx + 1) % N_FRAMES;
		for (i = 0, avg_frame = 0; i < N_FRAMES; i++) {
			avg_frame += frame_times[i];
		}
		avg_frame = avg_frame / (float) N_FRAMES;
		snprintf(buf, 35, "FPS: %6.2f", 1000.0 / avg_frame);
		gfx_setCursor(25, 20 + DISP_HEIGHT + 2 * (gfx_getTextHeight() + 2));
		gfx_puts((unsigned char *)buf);
		gfx_puts((unsigned char *)" ");
		gfx_setCursor(25, 20 + DISP_HEIGHT + 3 * (gfx_getTextHeight() + 2));
		gfx_puts((unsigned char *)scr_opt);
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
				break;
			case 'e':
				can_switch = 0;
				break;
			case 't':
				te_lock = (te_lock == 0);
				break;
			default:
				printf("Options:\n");
				printf("\ts - switch demo mode\n");
				printf("\td - disable auto-switching of demo mode\n");
				printf("\te - enable auto-switching of demo mode\n");
			case 0:
				break;
		}
		t0 = t1;
	}
}
