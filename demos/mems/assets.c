/*
 * assets.c - generate graphic assets
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/dma2d.h>
#include <gfx.h>

#define L1                   1UL
#define L4                   2UL
#define L8                   3UL

/* 
 * write_pixel()
 * Used to store pixel data into memory in a DMA2D compatible way.
 *
 * Note that it uses a GFX_COLOR struct which is a 32 bit structure
 * holding 8 bit values for alpha, red, green, blue. But only green
 * is used 8bpp, 4bpp, or 1bpp. (L8, L4, L1)
 */

static void 
write_pixel(int x, int y, GFX_COLOR color) {
	uint32_t offset;
	uint8_t *pixel;
	uint8_t mask;

	offset = (y * fb->stride) + ((x * fb->stride) / fb->width );
	if (offset >= sizeof(screen)) {
		printf("write_pixel: %d, %d is out of the buffer\n", x, y);
		return;
	}
	pixel = &(screen[offset]);
	switch (fb->fmt) {
		case L1:
			mask = 0x80 >> (x % 8);
			if ((color.g & 1) != 0) {
				*pixel |= mask;
			} else {
				*pixel &= ~mask;
			}
			break;
		case L4:
			mask = 0xf0 >> ((x & 1) * 4);
			*pixel &= ~mask;
			*pixel |= (color.g & 0xf) << ((x & 1) * 4);
			break;
		case L8:
			*pixel = (color.g & 0xff);
			break;
	}
}

typedef struct {
	uint32_t	addr;
	uint16_t	width;
	uint16_t	height;
	uint16_t	stride;
	uint8_t		bpp;
	uint8_t		fmt;
} dma2d_bitmap;


/*
 * render_asset()
 *
 * This uses the DMA2D peripheral to render an asset from the
 * pre-rendered asset buffer, and render it into the main display
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
render_asset(int x, int y, dma2d_bitmap *asset, dma2d_pixel *clut)
/*int d, uint32_t color, uint32_t outline) */
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
	DMA2D_BGPFCCR = DMA2D_SET(BGPFCCR, CM, DMA2D_BGPFCCR_CM_ARGB8888) |
					DMA2D_SET(BGPFCCR, AM, 0);

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
	DMA2D_FGPFCCR = DMA2D_SET(FGPFCCR, CS, 255) |
					DMA2D_SET(FGPFCCR, ALPHA, 255) |
					DMA2D_SET(FGPFCCR, AM, 0) |
					DMA2D_SET(FGPFCCR, CM, DMA2D_FGPFCCR_CM_L8);
	/*
	 * Transfer it!
	 */
	DMA2D_CR |= DMA2D_CR_START;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error\n");
		while (1);
	}
}

#define TIMER_MAX	16384

uint8_t screen[16384]; // 16K which gets re-used
struct eve_bitmap *fb; 
struct eve_bitmap reticule;
struct eve_bitmap measuring_cursor[3];
struct eve_bitmap horiz_cursor[3];

void create_screen(void);
void clear_screen(void);
void write_pixel(int x, int y, uint16_t color);


void clear_screen(dma2d_bitmap *fb) {
	int size = fb->width * fb->height;
	if (fb->fmt == L1) {
		size /= 8;
	} else if (fb->fmt == L4) {
		size /= 2;
	}
	memset((uint8_t *)(fb->addr), size, 0);
}

void create_screen(void) {
	int i, k;

	clear_screen();
	reticule.width = 304;
	reticule.height = 240;
	reticule.fmt = L1;
	reticule.stride = 38;
	reticule.bpp = 1;
	fb = &reticule;
	gfx_init(write_pixel, reticule.width, reticule.height, GFX_FONT_LARGE);
	gfx_drawRoundRect(0, 0, 300, 240, 10, 1);
	gfx_setTextSize(1);
	gfx_setCursor(20, 12);
	gfx_setTextColor(1, 0);
	gfx_puts((unsigned char *) "Test Bitmap");
	gfx_fillTriangle(0,0, 20, 0, 0, 20, 1);
	for (k = 10; k < 240; k+= 10) {
		for (i = 10; i < 300; i += 10) {
			if ((k % 30) == 0) {
				gfx_drawLine(i-1, k, i+1, k, 1);
			}
			if ((i % 30) == 0) {
				/* should use a "+" here? */
				gfx_drawLine(i, k-1, i, k+1, 1);
			} 
		}
	}
	gfx_setFont(GFX_FONT_SMALL);
	gfx_setTextRotation(GFX_ROT_90);
	gfx_fillRoundRect(248, 23, gfx_getTextHeight()*2+5, gfx_getTextWidth() * 2 * 12 + 5, 5, 0);
	gfx_drawRoundRect(248, 23, gfx_getTextHeight()*2+5, gfx_getTextWidth() * 2 * 12 + 5, 5, 1);

	gfx_setCursor(250, 25);
	gfx_setTextSize(2);
	gfx_puts((unsigned char *) "Oscilloscope");
}

#define IS_HORIZ	1
#define IS_VERT		0
#define MAX_CURSORS 6

struct cursor_data_struct {
	struct eve_bitmap	*bitmap;
	uint32_t timer;
	int16_t		prev;
	int		hv;
	int		ndx;
	uint16_t	color;
} cursor_data[MAX_CURSORS];


void create_cursor(int n, struct cursor_data_struct *);
void create_horiz_cursor(int n, struct cursor_data_struct *);
/*
 * Cursors:
 *
 *	0 - T0
 *	1 - T1
 *	2 - Trigger (TRG)
 *	3 - V0
 * 	4 - V1
 * 	5 - GND
 *
 * Swaps 	(T0 -> V0,  0 <=> 3)
 *			(T1 -> V1,	1 <=> 4)
 *			(GND -> TRG, 	5 <=> 2)
 */

void create_cursor(int n, struct cursor_data_struct *cursor) {
	int i;

	clear_screen();
	fb = &measuring_cursor[n];
	fb->width= 16 ;
	fb->height = 280;
	fb->fmt = L1;
	fb->stride = 2;
	fb->bpp = 1;
	gfx_init(write_pixel, fb->width, fb->height, GFX_FONT_SMALL);
	gfx_fillTriangle(1, 0, 7, 0, 4, 6, 1);
	gfx_fillTriangle(1, 249, 7, 249, 4, 243, 1);
	for (i = 10; i < 240; i+= 10) {
		gfx_drawLine(4, i+5, 4, i+8, 1);
	}
	gfx_drawRoundRect(0, 249, 9, 20, 3, 1);
	gfx_setTextColor(1, 0);
	gfx_setCursor(2, 249+9);
	switch (n) {
		case 0:
		case 1:
			gfx_putc('t');
			gfx_setCursor(2, 250+16);
			gfx_putc('0'+n);
			cursor->color = VFX_WHITE;
			cursor->ndx = 150;
			break;
		default:
			gfx_putc('T');
			gfx_setCursor(2, 250+16);
			gfx_putc('R');
			cursor->color = VFX_GREY;
			cursor->ndx = 110;
	}
	eve_store_bitmap(&measuring_cursor[n], screen);
	cursor->bitmap = &measuring_cursor[n];
	cursor->hv = IS_VERT;
	cursor->timer = 0;
	cursor->prev = 0;
	printf("Stored vertical cursor %d: Bitmap @ 0x%x\n", n, (unsigned int) measuring_cursor[n].addr);
}

void create_horiz_cursor(int n, struct cursor_data_struct *cursor) {
	int i;

	clear_screen();
	fb = &horiz_cursor[n];
	fb->width= 336;
	fb->height = 12;
	fb->fmt = L1;
	fb->stride = 42;
	fb->bpp = 1;
	gfx_init(write_pixel, fb->width, fb->height, GFX_FONT_SMALL);
	gfx_fillTriangle(0, 3, 0, 9, 6, 6, 1);
	gfx_fillTriangle(311, 3, 305, 5, 311, 9, 1);
	if (n < 2) {
		gfx_drawRoundRect(311, 0, 15, 12, 3, 1); /* two char label */
	} else {
		gfx_drawRoundRect(311, 0, 21, 12, 3, 1); /* three char label */
	}
	for (i = 10; i < 305; i+= 10) {
		gfx_drawLine(i+5, 6, i+8, 6, 1);
	}
	gfx_setTextColor(1, 0);
	gfx_setCursor(313, 9);
	switch (n) {
		case 0:
			gfx_puts((unsigned char *)"v0");
			cursor->color = VFX_WHITE;
			cursor->ndx = 110;
			break;
		case 1:
			gfx_puts((unsigned char *)"v1");
			cursor->color = VFX_WHITE;
			cursor->ndx = 90;
			break;
		default:
			gfx_puts((unsigned char *)"GND");
			cursor->color = VFX_GREEN;
			cursor->ndx = 100;
	}
	eve_store_bitmap(&horiz_cursor[n], screen);
	cursor->bitmap = &horiz_cursor[n];
	cursor->hv = IS_HORIZ;
	cursor->timer = 0;
	printf("Stored horizontal cursor %d: Bitmap @ 0x%x\n", n, (unsigned int) horiz_cursor[n].addr);
}

void update_cursor(int cursor);

/* 
 * Put the indicated cursor on the screen at the appropriate place
 *
 * This function does two things, both reads the timer (quadrature encoder)
 * associated with the cursor, and positions it on the screen.
 */
void
update_cursor(int cursor_num)
{
	struct cursor_data_struct *cursor;
	uint32_t timer;
	int	x, y;
	int16_t delta;
	int c_limit;
	int16_t cur;

	/* invalid cursor */
	if ((cursor_num < 0) || (cursor_num >= MAX_CURSORS)) {
		return;
	}

	cursor = &cursor_data[cursor_num];
	c_limit = (cursor->hv == IS_HORIZ) ? 240 : 300; /* reticule width */
	timer = cursor->timer;
	
	/* if timer is non-null figure out if the user has turned the knob */
	if (timer) {
		cur = timer_get_counter(timer);
		delta = cur - cursor->prev;
		if (delta) {
			printf("Delta : %d\n", delta);
		}
		/* scaling function */
		if (delta != 0) {
			if (abs(delta) < 10) {
				delta = (delta < 0) ? -1 : 1;
			} else if (abs(delta) < 100) {
				delta = (delta < 0) ? -5 : 5;
			} else {
				delta = (delta < 0) ? -20 : 20;
			}
		}
		cursor->prev = cur;
		cursor->ndx += delta;
		cursor->ndx = (cursor->ndx < 0) ? 0 : cursor->ndx;
		cursor->ndx = (cursor->ndx >= c_limit) ? c_limit - 1 : cursor->ndx;
	}
	if (cursor->hv == IS_HORIZ) {
		x = 0;
		y = cursor->ndx;
	} else {
		x = cursor->ndx;
		y = 0;
	}

	eve_move_to(x, y);
	eve_set_color(cursor->color);
	eve_draw_bitmap(cursor->bitmap);
}

uint32_t cursor_timer(int n);
void activate_cursor(int n, uint32_t timer);

uint32_t cursor_timer(int n) {
	return cursor_data[n].timer;
}

void activate_cursor(int n, uint32_t timer) {
	struct cursor_data_struct *cursor = &cursor_data[n];
	int c_limit;

	/* Deal with changes in active/inactive */
	c_limit = (cursor->hv == IS_HORIZ) ? 240 : 300; /* reticule width */
	if (cursor->timer != timer) {
		if (timer) {
			timer_set_counter(timer, (cursor->ndx * 1024) / c_limit);
			cursor->timer = timer;
			cursor->prev = timer_get_counter(timer);
		} else {
			cursor->timer = 0; /* inactive */
		}
	}
}

void cursor_swap(PIN p, int cursor_1, int cursor_2);

/*
 * Debounce pin activity and swap cursors.
 */
void cursor_swap(PIN p, int cursor_1, int cursor_2) {
	msleep(2);
	if (pin_get(p, PXX)) {
		return;	/* just a glitch, ignore it */
	}

	while (pin_get(p, PXX) == 0) ;	/* wait while pressed */
	if (cursor_data[cursor_1].timer) {
		activate_cursor(cursor_2, cursor_data[cursor_1].timer);
		activate_cursor(cursor_1, 0);
	} else {
		activate_cursor(cursor_1, cursor_data[cursor_2].timer);
		activate_cursor(cursor_2, 0);
	}
	msleep(2); /* let it stay up for at least 2 ms */
}

void timer_setup(PIN p0, PIN p1, uint32_t);

void
timer_setup(PIN p0, PIN p1, uint32_t timer) {
	pin_function(GPIO_AF2, p0, p1, PXX);
	pin_attributes(PIN_PULLUP | PIN_SLOW, p0, p1, PXX);
	timer_set_period(timer, 0xffff);
	timer_slave_set_mode(timer, 0x3); // encoder
	timer_ic_set_input(timer, TIM_IC1, TIM_IC_IN_TI1);
	timer_ic_set_input(timer, TIM_IC2, TIM_IC_IN_TI2);
	timer_enable_counter(timer);
}

/*
 * Frob knobs, from left to right ...
 *
 * Frob on Timer 3, PC6, PC7	PB -> PC5
 *         Timer 4, PB6, PB7	PB -> PC1
 *		   Timer 5, PA0, PA1	PB -> PC0
 *
 */
int
main()
{
	int	i;
	char buf[25];

	clock_setup(96000000, 8000000);
	console(PA2, PA3, 115200);

	printf("\nVM800B Test code.\n");

	/* this is the debug flag/bit */
	rcc_periph_clock_enable(RCC_TIM3);
	rcc_periph_clock_enable(RCC_TIM4);
	rcc_periph_clock_enable(RCC_TIM5);
	
	timer_setup(PC6, PC7, TIM3);
	timer_setup(PB6, PB7, TIM4);
	timer_setup(PA0, PA1, TIM5); 

	pin_output(PC3, PA5, PXX);
	pin_input(PC0, PC1, PC5, PXX);
	pin_attributes(PIN_PULLUP, PC0, PC1, PC5, 
					PC6, PC7, PB6, PB7, PA0, PA1, PXX);

	/* You can print integers and floats */
    printf("Hello, Setting up EVE hardware.\n");
	eve_hw_init();

	printf("Now setting up EVE software.\n");
	eve_sw_init();
	
	create_screen();
	eve_store_bitmap(&reticule, screen);
	printf("Stored: Bitmap @ 0x%x\n", (unsigned int) reticule.addr);

	for (i = 0; i < 3; i++) {
		create_cursor(i, &cursor_data[i]);
	}
	for (i = 0; i < 3; i++) {
		create_horiz_cursor(i, &cursor_data[i+3]);
	}
	timer_set_counter(TIM3, 512);
	timer_set_counter(TIM4, 512);
	timer_set_counter(TIM5, 512);
	activate_cursor(0, TIM3);
	activate_cursor(1, TIM4);
	activate_cursor(5, TIM5);
	while (1) {
		eve_prep();
		eve_width(1.0);
		eve_set_color(VFX_YELLOW);
		eve_move_to(5, 5);
		eve_set_color(VFX_MAGENTA);
		eve_draw_bitmap(&reticule);

		/* cursor 0 drawing */
		for (i = 0; i < MAX_CURSORS; i++) {
			update_cursor(i);
		}

		if (cursor_data[0].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
			
		sprintf(buf, " T0 Cursor = %4d", cursor_data[0].ndx);
		eve_puts(355, 30, buf);
		if (cursor_data[1].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
		sprintf(buf, " T1 Cursor = %4d", cursor_data[1].ndx);
		eve_puts(355, 42, buf);
		if (cursor_data[3].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
		sprintf(buf, " V0 Cursor = %4d", cursor_data[3].ndx);
		eve_puts(355, 54, buf);
		if (cursor_data[4].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
		sprintf(buf, " V1 Cursor = %4d", cursor_data[4].ndx);
		eve_puts(355, 66, buf);
		if (cursor_data[2].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
		sprintf(buf, "TRG Cursor = %4d", cursor_data[2].ndx);
		eve_puts(350, 78, buf);
		if (cursor_data[5].timer) {
			eve_set_color(VFX_WHITE);
		} else {
			eve_set_color(VFX_GREEN);
		}
		sprintf(buf, "GND Cursor = %4d", cursor_data[5].ndx);
		eve_puts(350, 90, buf);

		/* T0/V0 */
		if (pin_get(PC5, PXX) == 0) {
			cursor_swap(PC5, 0, 3);
		}
		/* T1/V1 */
		if (pin_get(PC1, PXX) == 0) {
			cursor_swap(PC1, 1, 4);
		}
		/* GND / TRG */
		if (pin_get(PC0, PXX) == 0) {
			cursor_swap(PC0, 2, 5);
		}
		eve_start();
		msleep(2);
	}
}
