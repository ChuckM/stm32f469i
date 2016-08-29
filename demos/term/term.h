/*
 * Defines and prototypes in the "term" space
 */

#ifndef TERM_H
#define TERM_H

#ifndef NULL
#define NULL	((void *) 0)
#endif

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 19
#define LINE_SPACE 19 

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

#define TERM_WIDTH	80
#define TERM_HEIGHT	25

#define TERM_CHAR_INVERSE	0x1
#define TERM_CHAR_BOLD		0x2

enum cursor_direction {
	CURSOR_UP,
	CURSOR_DOWN,
	CURSOR_RIGHT,
	CURSOR_LEFT,
	CURSOR_RETURN,
	CURSOR_HOME
};

/*
 * Simple font descriptor, size of the *monospaced* characters
 * a bitmap indicating which characters are present, and a pointer
 * to the glyph data.
 */
typedef struct term_font {
	int	w;
	int	h;
	uint32_t	glyphs[256];
	const uint8_t	*glyph_data;
} TERM_FONT;

#endif
