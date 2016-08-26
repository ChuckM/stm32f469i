/*
 * term.c - emulate a simple terminal
 */
#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <gfx.h>
#include "../util/util.h"

#ifndef NULL
#define NULL	((void *) 0)
#endif

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 19
#define LINE_SPACE 19 

/* This gives 4 equal tempered grey levels 0, 55, aa, ff */
const uint32_t __term_color_table[16] = {
	0xff000000,	/* 0: black */
	0xffc00000,	/* 1: red */
	0xff00c000, /* 2: green */
	0xff0000c0, /* 3: blue */
	0xffc0c000, /* 4: yellow */
	0xff00c0c0, /* 5: cyan */
	0xffc000c0,	/* 6: magenta */
	0xff555555, /* 7: dark grey */
	0xffaaaaaa,	/* 8: light grey */
	0xffff0000, /* 9: bold red */
	0xff00ff00, /* A: bold green */
	0xff0000ff, /* B: bold blue */
	0xffffff00, /* C: bold yellow */
	0xff00ffff, /* D: bold cyan */
	0xffff00ff, /* E: bold magenta */
	0xffffffff	/* F: white */
};

#define TERM_COLOR_BLACK	0
#define TERM_COLOR_RED		1
#define TERM_COLOR_GREEN	2
#define TERM_COLOR_BLUE		3
#define TERM_COLOR_YELLOW	4
#define TERM_COLOR_CYAN		5
#define TERM_COLOR_MAGENTA	6
#define TERM_COLOR_DKGREY	7
#define TERM_COLOR_LTGREY	8
#define TERM_COLOR_BRED		9
#define TERM_COLOR_BGREEN	10
#define TERM_COLOR_BBLUE	11
#define TERM_COLOR_BYELLOW	12
#define TERM_COLOR_BCYAN	13
#define TERM_COLOR_BMAGENTA	14
#define TERM_COLOR_WHITE	15

#define TERM_MAX_COLOR		16

extern const uint8_t font_glyphs[128 - 32][CHAR_HEIGHT][CHAR_WIDTH];


/* a box size (w) width and (h) height */
typedef struct _pixblock {
	int	w;
	int h;
} pixel_block;

/* a target for a pixel block */
typedef struct _pixtarget {
	int w;
	int h;
	uint32_t	addr;
} pixel_target;

enum cursor_direction {
	CURSOR_UP,
	CURSOR_DOWN,
	CURSOR_RIGHT,
	CURSOR_LEFT,
	CURSOR_RETURN,
	CURSOR_HOME
};

/*
 * Global state on where the
 * cursor is currently.
 *
 * Global cursor state : plus API is easeier
 * 	minus sucks to multi-task/multi-thread.
 */
int current_row;
int current_col;
int current_fg_color;
int current_bg_color;
pixel_target text_cursor;

/* function prototypes */

void dma2d_char(char c, int fg, int bg);
void move_cursor(enum cursor_direction m);
int get_glyph(pixel_target *tgt, unsigned char c);
void clear_screen(int bgcolor);

/*
 * clear screen
 */
void
clear_screen(int color)
{
    DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
    DMA2D_OPFCCR = 0x0; /* ARGB8888 pixels */
    /* force it to have full alpha */
    DMA2D_OCOLR = __term_color_table[color % TERM_MAX_COLOR];
    DMA2D_OOR = 0;
    DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
    DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

    /* kick it off */
    DMA2D_CR |= DMA2D_CR_START;
    while (DMA2D_CR & DMA2D_CR_START);
}

/*
 * questions - global state
 *				cursor postion
 *				auto wrap on col 80
 *				auto scroll on line 24?
 *				address zero based ? 0 - 79, 0 - 23 or 1 based?
 */

/*
 * cursor movement
 * Update the text cursor position
 */
void
move_cursor(enum cursor_direction m)
{
	switch (m) {
	case CURSOR_UP:
		current_row = (current_row > 0) ? current_row - 1 : 0;
		break;
	case CURSOR_DOWN:
		current_row = (current_row < 23) ? current_row + 1 : 23;
		break;
	case CURSOR_LEFT:
		current_col = (current_col > 0) ? current_col - 1 : 0;
		break;
	case CURSOR_RIGHT:
		current_col = (current_col < 79) ? current_col + 1 : 79;
		break;
	case CURSOR_RETURN:
		current_col = 0;
		break;
	case CURSOR_HOME:
		current_col = current_row = 0;
		break;
	}
	text_cursor.w = current_col * CHAR_WIDTH * 4;
	text_cursor.h = CHAR_HEIGHT;
	text_cursor.addr = FRAMEBUFFER_ADDRESS +
				( current_row * LINE_SPACE ) * 800 * 4 +
				(current_col * CHAR_WIDTH) * 4;
}

/* character glpyhs */
int
get_glyph(pixel_target *tgt, unsigned char c) {
	int	ndx = c - 0x20;
	if (tgt == NULL) {
		return 1;
	}
	if ((ndx < 0) || (ndx > 191)) {
		ndx = 0;
	}
	tgt->w = CHAR_WIDTH;
	tgt->h = CHAR_HEIGHT;
	tgt->addr = (uint32_t)(&font_glyphs[ndx][0][0]);
	return 0;
}


/*
 * This function has an issue, it is putting junk in the output frame buffer
 * after the character is about half written.
 * What is more, the junk is "weird" colors, it isn't the green of the FG
 * Color register or the black of the BG color register. 
 */
void
dma2d_char(char c, int fg, int bg)
{
	pixel_target glyph;

	get_glyph(&glyph, c);
	if (fg == bg) {
		return; /* invisible character */
	}
	while (DMA2D_CR & DMA2D_CR_START); /* Wait for idle */

	/* Set up for a memory to memory with blend transfer of one character
	 * cell (glyph)
	 */
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2MWB);
	DMA2D_NLR = DMA2D_SET(NLR, PL, glyph.w) | DMA2D_SET(NLR, NL, glyph.h);

	/* 
	 * Point FG and BG to the character glyph, with BG_COLR set to the background
	 * color and FG_COLR set to the foreground color. BG Alpha is fixed at 0xff
	 * and FG Alpha comes from the glyph data.
	 */
	DMA2D_BGPFCCR = DMA2D_SET(BGPFCCR, CM, DMA2D_FGPFCCR_CM_A8) | DMA2D_SET(BGPFCCR, AM, 1) |
					DMA2D_SET(BGPFCCR, ALPHA, 0xff);
	DMA2D_BGMAR = glyph.addr;
	DMA2D_BGOR = 0;
	DMA2D_BGCOLR = __term_color_table[bg % TERM_MAX_COLOR];

	DMA2D_FGPFCCR = DMA2D_SET(FGPFCCR, CM, DMA2D_FGPFCCR_CM_A8) | DMA2D_SET(FGPFCCR, AM, 2) |
					DMA2D_SET(FGPFCCR, ALPHA, 0xff);
	DMA2D_FGMAR = glyph.addr;
	DMA2D_FGOR = 0;
	DMA2D_FGCOLR = __term_color_table[fg % TERM_MAX_COLOR];

	/* Set the output to put it into the frame buffer */
	DMA2D_OPFCCR = DMA2D_OPFCCR_CM_ARGB8888;
	DMA2D_OMAR = text_cursor.addr;
	DMA2D_OOR = 800 - glyph.w;
	DMA2D_CR |= DMA2D_CR_START;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("DMA2D Configuration Error!\n");
		while(1);
	}
}

void screen_puts(char *s);

void
screen_puts(char *s)
{
	while (*s != 0) {
		switch (*s) {
		case '\n':
			move_cursor(CURSOR_RETURN);
			move_cursor(CURSOR_DOWN);
			break;
		default:
			dma2d_char(*s, current_fg_color, current_bg_color);
			move_cursor(CURSOR_RIGHT);
			lcd_flip(0);	/* update per character or after the string? */
			break;
		}
		s++;
	}
}

/*
 * An experiment in rendering. Rendering from
 * a buffer with out of buffer attributes.
 * That works, but its easy to have the attributes
 * and characters get out of sync. And writing both
 * character and attributes is two stores instead of
 * one.
 *
 * A "character mode" frame buffer would be useful as
 * much of the work is algorithmic (insert/delete char/line)
 * The "new" buffer
 *  3       2       1  
 *	1.......4.......6.......8.......0
 *  | bg co | fg co | attr  | char  |
 *	+.......+.......+.......+.......+
 *
 *
 * 
 */
#define SCREEN_WIDTH	80
#define SCREEN_HEIGHT	25

uint32_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void render_buffer(int opt);
void
render_buffer(int opt)
{
	int row, col;
	uint32_t t0, t1;
	uint32_t fg, bg;
	unsigned char c;
	uint16_t	attr;

	t0 = mtime();
	clear_screen(current_bg_color);
	for (row = 0; row < SCREEN_HEIGHT; row ++) {
		for (col = 0; col < SCREEN_WIDTH; col++) {
			attr = (buffer[ row * SCREEN_WIDTH + col] >> 16) & 0xffff;
			if (attr == 0) {
				attr = (current_bg_color << 8) | (current_fg_color);
			}
			c = buffer[row * SCREEN_WIDTH + col] & 0xff;
			bg = (attr >> 8) & 0xff;
			fg = attr & 0xff;
			switch (opt) {
			case 0: /* render none */
					break;
			case 1:	/* render all */
				text_cursor.addr = FRAMEBUFFER_ADDRESS +
						( row * LINE_SPACE ) * 800 * 4 +
						( col * CHAR_WIDTH) * 4;
				dma2d_char(c, fg, bg);
				break;
			default: /* render non-space */
				if (c != ' ') {
					text_cursor.addr = FRAMEBUFFER_ADDRESS +
						( row * LINE_SPACE ) * 800 * 4 +
						( col * CHAR_WIDTH) * 4;
						dma2d_char(c, fg, bg);
				}
				break;
			}
		}
	}
	lcd_flip(0);
	t1 = mtime();
	printf("Time to render (opt = %d) screen %d mS\n", opt, (int)(t1 - t0));
}

void splash_screen(void);

static struct {
	int row;
	int col;
	char *msg;
	int fg;
	int bg;
} splash_messages[] = {
	{ 13, 14,
		"Copyright (c) 2016 Chuck McManis, All Rights Reserved",
		TERM_COLOR_BYELLOW, TERM_COLOR_BLACK },
	{ 16, 5,
		"Colors: ",
		TERM_COLOR_GREEN, TERM_COLOR_BLACK },
	{ 16, 13,
		"Black",
		TERM_COLOR_BLACK, TERM_COLOR_WHITE },
	{ 16, 21,
		"Red",
		TERM_COLOR_RED, TERM_COLOR_BLACK },
	{ 16, 29,
		"Green",
		TERM_COLOR_GREEN, TERM_COLOR_BLACK },
	{ 16, 37,
		"Blue",
		TERM_COLOR_BLUE, TERM_COLOR_BLACK },
	{ 16, 45,
		"Yellow",
		TERM_COLOR_YELLOW, TERM_COLOR_BLACK },
	{ 16, 53,
		"Cyan",
		TERM_COLOR_CYAN, TERM_COLOR_BLACK },
	{ 16, 61,
		"Magenta",
		TERM_COLOR_MAGENTA, TERM_COLOR_BLACK },
	{ 16, 69,
		"Dark Grey",
		TERM_COLOR_DKGREY, TERM_COLOR_BLACK },
	{ 17, 10,
		"Light Grey",
		TERM_COLOR_LTGREY, TERM_COLOR_BLACK },
	{ 17, 21,
		"Red",
		TERM_COLOR_BRED, TERM_COLOR_BLACK },
	{ 17, 29,
		"Green",
		TERM_COLOR_BGREEN, TERM_COLOR_BLACK },
	{ 17, 37,
		"Blue",
		TERM_COLOR_BBLUE, TERM_COLOR_BLACK },
	{ 17, 45,
		"Yellow",
		TERM_COLOR_BYELLOW, TERM_COLOR_BLACK },
	{ 17, 53,
		"Cyan",
		TERM_COLOR_BCYAN, TERM_COLOR_BLACK },
	{ 17, 61,
		"Magenta",
		TERM_COLOR_BMAGENTA, TERM_COLOR_BLACK },
	{ 17, 69,
		"White",
		TERM_COLOR_WHITE, TERM_COLOR_BLACK },
/*
	{ 19, 5,
		"Simple code frag $a = { 32.0 / (2 * 6) << 1 } | foo;",
		TERM_COLOR_BGREEN, TERM_COLOR_BLACK },
*/
	{ -1, -1,
		NULL,
		TERM_COLOR_WHITE, TERM_COLOR_BLACK }
};

/*
 * This is the 'pixel' drawing function we'll use
 * to initialize our buffer of characters.
 */
static void
draw_splash_pixel(int x, int y, uint16_t color) {
	uint32_t pixel;

	pixel = (color & 0xf000) << 12 |
			(color & 0x0f00) << 8 |
			(color & 0xff);

	buffer[y*SCREEN_WIDTH + x] = pixel;
}

/*
 * Splash screen generator
 */

static void
generate_splash(void) {
	gfx_init(draw_splash_pixel, SCREEN_WIDTH, SCREEN_HEIGHT, GFX_FONT_LARGE);

	gfx_fillScreen((uint16_t) 0x0200 | ' ');
	gfx_drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5, 0x0500 | '*');
	gfx_setCursor(3, 12);
	/* this text doesn't write the 'background' color */
	gfx_setTextColor((uint16_t) 0x0200| '*', (uint16_t) 0x0200| ' ');
	gfx_puts((unsigned char *) "TERM V0.2");
}

/*
 * This code initializes various bits and throws up the
 * the splash screen on the LCD.
 */
void
splash_screen(void)
{
	unsigned int i;
	char	*msg;
	int		row, col;

	generate_splash();
	current_fg_color = TERM_COLOR_GREEN;
	current_bg_color = TERM_COLOR_BLACK;
	i = 0;
	while (splash_messages[i].row > 0) {
		msg = splash_messages[i].msg;
		row = splash_messages[i].row;
		col = splash_messages[i].col;
		while (*msg) {
			buffer[row * SCREEN_WIDTH + col] = (splash_messages[i].bg << 24) |
												(splash_messages[i].fg << 16) | *msg;
			msg++;
			col++;
		}
		i++;
	}
	render_buffer(3);
	move_cursor(CURSOR_HOME);
	
}
int
main(void) {
	char	c;
	pixel_target pt;
#ifdef DUMP_GLYPH
	int		row, col;
	uint8_t	*gl;
#endif

	rcc_periph_clock_enable(RCC_DMA2D);
	printf("Terminal simulation.\n");
	splash_screen();
	printf("Please type characters :\n");
	while (1) {
		c = console_getc(1);
		if (c == '\r') {
			move_cursor(CURSOR_RETURN);
			move_cursor(CURSOR_DOWN);
		} else if (c == 0xc) {
			clear_screen(0xff000000);
			lcd_flip(0);
			move_cursor(CURSOR_HOME);
		} else {
			get_glyph(&pt, c);
			dma2d_char(c, current_fg_color, current_bg_color);
			move_cursor(CURSOR_RIGHT);
			lcd_flip(0);
		}
	}
}
