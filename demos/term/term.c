/*
 * term.c - emulate a simple terminal
 */
#include <stdio.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <gfx.h>
#include "../util/util.h"
#include "term.h"

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
 *
 *  3       2       1  
 *	1.......4.......6.......8.......0
 *  | bg co | fg co | attr  | char  |
 *	+.......+.......+.......+.......+
 *
 *
 * 
 */

uint32_t buffer[TERM_WIDTH * TERM_HEIGHT];

extern TERM_FONT regular_font;
extern TERM_FONT bold_font;

/*
 * Global state.
 */
int current_fg_color;
int current_bg_color;
int	current_attrs;

/* Current cursor position */
struct {
	int	row;
	int	col;
	int	state;	/* blinking or not */
	uint32_t	addr;
} text_cursor;

/* function prototypes */

void dma2d_char(uint32_t glyph, int w, int h, uint32_t addr, uint32_t fg, uint32_t bg);
uint32_t get_glyph(struct term_font *font, unsigned char c);
void clear_screen(uint32_t bgcolor);
int simple_scroll(int lines);
void fast_scroll(int lines);
uint32_t render_buffer(int option);
uint32_t splash_screen(int opt);

/*
 * Public facing API
 */
void term_putc(unsigned char c);
void term_clear(void);
void term_set_colors(int fg_color, int bg_color);
int term_get_fg(void);
int term_get_bg(void);
void cursor_move(enum cursor_direction m);
void cursor_set(int row, int col);
void term_set_attrs(uint8_t attr);

static const uint8_t __cursor_glyph[CHAR_HEIGHT][CHAR_WIDTH] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

void term_draw_cursor(int on_or_off);
void term_blink_cursor(void);

void
term_draw_cursor(int state) {
	uint32_t addr, buf_char;
	int fg, bg, t;
	TERM_FONT *f;
	uint8_t c, attr;

	if (text_cursor.state == state) {
		return;
	}
	text_cursor.state = state;
	addr = FRAMEBUFFER_ADDRESS +
				( text_cursor.row * LINE_SPACE ) * 800 * 4 +
				(text_cursor.col * CHAR_WIDTH) * 4;
	if (state) {
		dma2d_char((uint32_t) (&__cursor_glyph[0][0]), CHAR_WIDTH, CHAR_HEIGHT, 
					addr, __term_color_table[TERM_COLOR_BCYAN], 0x0);
	} else {
		buf_char = buffer[text_cursor.row * TERM_WIDTH + text_cursor.col]; 
		c = buf_char & 0xff;
		attr = (buf_char >> 8) & 0xff;
		fg = (buf_char >> 16) & 0xff;
		bg = (buf_char >> 24) & 0xff;
		if (attr & TERM_CHAR_INVERSE) {
				t = bg; bg = fg; fg = t;
		}
		f = (attr & TERM_CHAR_BOLD) ? &bold_font : &regular_font;
		dma2d_char(get_glyph(f, c), f->w, f->h,
						   addr, __term_color_table[fg], __term_color_table[bg]);
	}
	lcd_flip(0);
}

void
term_blink_cursor(void)
{
	text_cursor.state ^= 1;
	term_draw_cursor(text_cursor.state);
}
/*
 * clear screen
 */
void
clear_screen(uint32_t color)
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
}

void
term_clear(void) {
    clear_screen(__term_color_table[current_bg_color]);
}

void
term_set_colors(int fg, int bg)
{
	current_fg_color = fg % TERM_MAX_COLOR;
	current_bg_color = bg % TERM_MAX_COLOR;
}
void
term_set_attrs(uint8_t attr)
{
	current_attrs = attr;
}

/*
 * This is a "simple" scroller. It moves data in screen character
 * buffer and then re-renders the screen.
 */
int
simple_scroll(int lines)
{
	int	len;
	uint32_t *dst, *src;

	len = (TERM_WIDTH * TERM_HEIGHT) - (lines * TERM_WIDTH);
	dst = &buffer[0];
	src = &buffer[lines * TERM_WIDTH];
	while (len--) {
		*dst = *src;
		dst++; src++;
	}
	for (len = 0; len < TERM_WIDTH; len++) {
		buffer[(TERM_HEIGHT - 1) * TERM_WIDTH + len] = 
			(current_bg_color << 24) | (current_fg_color << 16) |
			(current_attrs << 8) | ' ';
	}
	(void) render_buffer(1);
	return 24;
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
cursor_move(enum cursor_direction m)
{
	switch (m) {
	case CURSOR_UP:
		text_cursor.row = (text_cursor.row > 0) ? text_cursor.row - 1 : 0;
		break;
	case CURSOR_DOWN:
		text_cursor.row = (text_cursor.row < 24) ? text_cursor.row + 1 : simple_scroll(1);
		break;
	case CURSOR_LEFT:
		text_cursor.col = (text_cursor.col > 0) ? text_cursor.col - 1 : 0;
		break;
	case CURSOR_RIGHT:
		if (text_cursor.col < 79) {
			text_cursor.col++;
		} else {
			text_cursor.col = 0;
			cursor_move(CURSOR_DOWN);
		}
		break;
	case CURSOR_RETURN:
		text_cursor.col = 0;
		break;
	case CURSOR_HOME:
		text_cursor.col = text_cursor.row = 0;
		break;
	}
	text_cursor.addr = FRAMEBUFFER_ADDRESS +
				( text_cursor.row * LINE_SPACE ) * 800 * 4 +
				(text_cursor.col * CHAR_WIDTH) * 4;
}

void
cursor_set(int row, int col) {
	text_cursor.row = row % TERM_WIDTH;
	text_cursor.col = col % TERM_HEIGHT;
	text_cursor.addr = FRAMEBUFFER_ADDRESS +
				( text_cursor.row * LINE_SPACE ) * 800 * 4 +
				(text_cursor.col * CHAR_WIDTH) * 4;
}

/*
 * character glpyhs
 *
 * This code takes a character (0 - 255) and returns
 * a pointer to a width x height array of bytes that
 * represent that character in this font. This is "easy"
 * for the ASCII set, less so for the Latin chars, and
 * not easy at all for control characters etc which
 * have no "official" glyph.
 */
uint32_t
get_glyph(TERM_FONT *f, unsigned char c) {
	return (uint32_t) (f->glyph_data + f->glyphs[c]);
}

/*
 * High level API, put a character on the screen
 */
void
term_putc(unsigned char c) {
	uint32_t fg, bg;
	TERM_FONT	*f;

	/* process special characters */
	switch (c) {
	case '\r':
		cursor_move(CURSOR_RETURN);
		return;
	case '\n':
		cursor_move(CURSOR_DOWN);
		return;
	case 0x08: /* ^H (non-destructive, moves left */
		cursor_move(CURSOR_LEFT);
		return;
	case 0xff: /* DEL (destructive, moves left, writes 'space' */
		cursor_move(CURSOR_LEFT);
		return term_putc(' ');
	case 0x2:
		current_attrs ^= TERM_CHAR_BOLD;
		return;
	case 0xc: /* refresh screen */
		render_buffer(1);
		return;
	default:
		break;
	}

	if (current_attrs & TERM_CHAR_BOLD) {
		f = &bold_font;
	} else {
		f = &regular_font;
	}
	buffer[text_cursor.row * TERM_WIDTH + text_cursor.col] = 
		(current_bg_color << 24) | (current_fg_color << 16) | c;
	if (current_attrs & TERM_CHAR_INVERSE) {
		bg = __term_color_table[current_fg_color];
		fg = __term_color_table[current_bg_color];
	} else {
		fg = __term_color_table[current_fg_color];
		bg = __term_color_table[current_bg_color];
	}
	dma2d_char(get_glyph(f, c), f->w, f->h, text_cursor.addr, fg, bg);
	buffer[text_cursor.row * TERM_WIDTH + text_cursor.col] =
		(current_bg_color << 24) | (current_fg_color << 16) | (current_attrs << 8) | c;
	cursor_move(CURSOR_RIGHT);
}


/*
 * Lowest level rendering function. It takes a character, an address
 * and a foreground and background color. It looks up glpyhs in its
 * table (although from an API perspective it probably shouldn't)
 */
void
dma2d_char(uint32_t glyph, int w, int h, uint32_t addr, uint32_t fg, uint32_t bg)
{
	if (fg == bg) {
		return; /* invisible character */
	}
	while (DMA2D_CR & DMA2D_CR_START); /* Wait for idle */

	/* Set up for a memory to memory with blend transfer of one character
	 * cell (glyph)
	 */
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2MWB);
	DMA2D_NLR = DMA2D_SET(NLR, PL, w) | DMA2D_SET(NLR, NL, h);

	/* 
	 * Point FG and BG to the character glyph, with BG_COLR set to the background
	 * color and FG_COLR set to the foreground color. BG Alpha is fixed at 0xff
	 * and FG Alpha comes from the glyph data.
	 */
	DMA2D_BGPFCCR = DMA2D_SET(BGPFCCR, CM, DMA2D_FGPFCCR_CM_A8) | DMA2D_SET(BGPFCCR, AM, 1) |
					DMA2D_SET(BGPFCCR, ALPHA, 0xff);
	DMA2D_BGMAR = glyph;
	DMA2D_BGOR = 0;
	DMA2D_BGCOLR = bg;

	DMA2D_FGPFCCR = DMA2D_SET(FGPFCCR, CM, DMA2D_FGPFCCR_CM_A8) | DMA2D_SET(FGPFCCR, AM, 2) |
					DMA2D_SET(FGPFCCR, ALPHA, 0xff);
	DMA2D_FGMAR = glyph;
	DMA2D_FGOR = 0;
	DMA2D_FGCOLR = fg;

	/* Set the output to put it into the frame buffer */
	DMA2D_OPFCCR = DMA2D_OPFCCR_CM_ARGB8888;
	DMA2D_OMAR = addr;
	DMA2D_OOR = 800 - w;
	DMA2D_CR |= DMA2D_CR_START;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("DMA2D Configuration Error!\n");
		while(1);
	}
}

void term_puts(char *s);

void
term_puts(char *s)
{
	while (*s != 0) {
		term_putc(*s);
		if (*s == '\n') {
			term_putc('\r');
		}
		s++;
	}
}

/*
 * There are two ways to render the character buffer.
 * you can either just go through every spot in the
 * render buffer and render that glpyh (1,280 separate
 * DMA2D transactions. Or you can clear the entire
 * screen to "space", (1 DMA2D transaction) and then
 * render only those characters in the buffer that
 * are either not ' ', or are ' ' with different
 * attributes than the defaults.
 *
 * We'll benchmark both kinds
 *	If opt == 0 - every character cell is written
 *				  and there is no "pre-clear"
 *	If opt != 0 - Pre-clear to space + bg_color and
 *				  only render if buffer isn't a space
 *				  or the bg color is different.
 */
uint32_t
render_buffer(int opt)
{
	int row, col;
	uint32_t t0, t1;
	int fg, bg, t;
	TERM_FONT	*f;
	uint32_t	addr;
	uint32_t	buf_char;
	uint8_t		attr;
	unsigned char c;

	t0 = mtime();
	if (opt) {
		clear_screen(current_bg_color);
	}
	for (row = 0; row < TERM_HEIGHT; row ++) {
		for (col = 0; col < TERM_WIDTH; col++) {
			buf_char = buffer[ row * TERM_WIDTH + col];
			/* this is a safety check */
			if ((buf_char & 0xffff0000) == 0) {
				buf_char |= (current_bg_color << 24) | (current_fg_color << 16);
			}
			/* pull out current attributes and colors */
			c = buffer[row * TERM_WIDTH + col] & 0xff;
			attr = (buf_char >> 8) & 0xff;
			fg = (buf_char >> 16) & 0xff;
			bg = (buf_char >> 24) & 0xff;
			if ((opt == 0) || (c != ' ') || (bg != current_bg_color)) {
				addr = FRAMEBUFFER_ADDRESS + ( row * LINE_SPACE ) * 800 * 4 +
						( col * CHAR_WIDTH) * 4;
				if (attr & TERM_CHAR_INVERSE) {
					t = bg; bg = fg; fg = t;
				}
				f = (attr & TERM_CHAR_BOLD) ? &bold_font : &regular_font;
				dma2d_char(get_glyph(f, c), f->w, f->h,
						   addr, __term_color_table[fg], __term_color_table[bg]);
			}
		}
	}
	lcd_flip(0);
	t1 = mtime();
	return (t1 - t0);
}


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

	buffer[y*TERM_WIDTH + x] = pixel;
}

/*
 * Splash screen generator
 */

static void
generate_splash(void) {
	gfx_init(draw_splash_pixel, TERM_WIDTH, TERM_HEIGHT, GFX_FONT_LARGE);

	gfx_fillScreen((uint16_t) 0x0200 | ' ');
	gfx_drawRoundRect(0, 0, TERM_WIDTH, TERM_HEIGHT, 5, 0x0500 | '*');
	gfx_setCursor(3, 12);
	/* this text doesn't write the 'background' color */
	gfx_setTextColor((uint16_t) 0x0200| '*', (uint16_t) 0x0200| ' ');
	gfx_puts((unsigned char *) "TERM V0.2");
}

/*
 * This code initializes various bits and throws up the
 * the splash screen on the LCD.
 */
uint32_t
splash_screen(int opt)
{
	unsigned int i;
	char	*msg;
	int		row, col;

	generate_splash();
	current_fg_color = TERM_COLOR_BGREEN;
	current_bg_color = TERM_COLOR_BLACK;
	i = 0;
	while (splash_messages[i].row > 0) {
		msg = splash_messages[i].msg;
		row = splash_messages[i].row;
		col = splash_messages[i].col;
		while (*msg) {
			buffer[row * TERM_WIDTH + col] = (splash_messages[i].bg << 24) |
												(splash_messages[i].fg << 16) | *msg;
			msg++;
			col++;
		}
		i++;
	}
	cursor_move(CURSOR_HOME);
	return render_buffer(opt);
}

int
main(void) {
	int	c;
	uint32_t	bnch;
	char	buf[81];
#ifdef DUMP_GLYPH
	int		row, col;
	uint8_t	*gl;
#endif

	rcc_periph_clock_enable(RCC_DMA2D);
	printf("Terminal simulation.\n");
	bnch = splash_screen(0);
	printf("Splash Screen renders in %d mS on option 0\n", (int) bnch);
	bnch = splash_screen(1);
	printf("Splash Screen renders in %d mS on option 1\n", (int) bnch);
	printf("Please type characters :\n");
	while (1) {

		if ((c = console_getc(0)) != 0) {
			console_putc(c);
			term_putc(c);
			if (c == '\r') {
				console_putc('\n');
				term_putc('\n');
			}
			if (c == 0x19) {
				for (c = 0; c < 256; c++) {
					term_putc(c);
					if (((c-1) % 32) == 0) {
						term_putc('\r');
						term_putc('\n');
					}
				}
			}
			if (c == 0x1a) {
				for (c = 0; c < 30; c++) {
					snprintf(buf, 81, "This is a test line # %d\n", c);
					term_puts(buf);
				}
			}
		}
	}
}
