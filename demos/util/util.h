
/*
 * This include file describes the functions exported by clock.c
 */
#ifndef __UTIL_H
#define __UTIL_H
#include <gfx.h>
/*
 * Definitions for clock functions
 */
void msleep(uint32_t);
uint32_t mtime(void);
void clock_setup(void);


/*
 * Our simple console definitions
 */

typedef enum term_colors_enum {
	NONE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE
} TERM_COLOR;

char * console_color(TERM_COLOR c);
void console_color_enable(void);
void console_color_disable(void);
void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);
void console_setup(int baud);
void console_baud(int baud);
uint32_t console_getnumber(void);

/* this is for fun, if you type ^C to this example it will reset */
#define RESET_ON_CTRLC

typedef enum led_color_e {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
	ORANGE_LED
} LED_COLOR;


/* dump memory contents as hex bytes */
void hex_dump(uint32_t addr, uint8_t *data, unsigned int len);

void on_led(LED_COLOR c);
void off_led(LED_COLOR c);
void toggle_led(LED_COLOR c);
void led_init(void);

/* Defines and prototypes for the sdram code */

#define SDRAM_BASE_ADDRESS ((uint8_t *)(0xC0000000))
void sdram_init(void);

/* QSPI FLASH utility functions */
void qspi_init(void);
/* Read data from FLASH (0 - 16MB worth) */
int qspi_read_flash(uint32_t addr, uint8_t *buf, int len);
/* Write date to FLASH (0 - 16MB worth) */
int qspi_write_flash(uint32_t addr, uint8_t *buf, int len);
/* Erase a 4K block of FLASH */
void qspi_erase_block(uint32_t addr);
/* Map FLASH into the address space */
void qspi_map_flash(void);
/* Unmap FLASH from the address space */
void qspi_unmap_flash(void);

#define FLASH_BASE_ADDRESS ((uint8_t *)(0x90000000))

/*
 * LCD function (if you've included lcd.o)
 */

#if 0
#ifndef GFX_COLOR
typedef union __gfx_color {
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
		uint32_t	a:8;
	} c;
	uint32_t	raw;
} GFX_COLOR;
#endif
#endif

/* #define FRAMEBUFFER_ADDRESS (0xc0000000U - (800U * 480U * 4U)) */
#define FRAMEBUFFER_ADDRESS (0xc1000000U - 0x200000U)
void lcd_init(void);
void lcd_clear(uint32_t color);
void lcd_flip(int te_locked);
void lcd_draw_pixel(void *buf, int x, int y, GFX_COLOR color);

/*
 * DMA 2D Utility functions and data structures
 */

/*
 * Color to bits map for the DMA2D_OCOLR register
 * Important note: If you re-use a variable defined as a
 * DMA2D_COLOR type, where one use is a different 'form'
 * than the other use, previous bits will not be cleared.
 * It is unclear that the DMA2D peripheral cares about
 * stray bits in the color register that aren't applicable
 * to the current color mode but be aware.
 */
typedef union __dma2d_color {
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
		uint32_t	a:8;
	} argb8888;
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
	} rgb888;
	struct {
		uint32_t	b:5;
		uint32_t	g:6;
		uint32_t	r:5;
	} rgb565;
	struct {
		uint32_t	b:5;
		uint32_t	g:5;
		uint32_t	r:5;
		uint32_t	a:1;
	} argb1555;
	struct {
		uint32_t	b:4;
		uint32_t	g:4;
		uint32_t	r:4;
		uint32_t	a:4;
	} argb4444;
	struct {
		uint32_t	l:8;
	} l8;
	struct {
		uint32_t	l:4;
	} l4;
	/* not sure these are useful ... */
	struct {
		uint32_t	a:8;
	} a8;
	struct {
		uint32_t	a:4;
	} a4;
	uint32_t	raw;
} DMA2D_COLOR;

#define DMA2D_ARGB8888	0
#define DMA2D_RGB888	1
#define DMA2D_RGB565	2
#define DMA2D_ARGB1555	3
#define DMA2D_ARGB4444	4
#define DMA2D_L8	5
#define DMA2D_AL44	6
#define DMA2D_AL88	7
#define DMA2D_L4	8
#define DMA2D_A8	9
#define DMA2D_A4	10

#ifndef DMA2D_FG_CLUT
#define DMA2D_FG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0400UL)
#endif
#ifndef DMA2D_BG_CLUT
#define DMA2D_BG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0800UL)
#endif

/*
 * This is a 'blittable' bitmap. The important bits to
 * get right are the stride and the bits per pixel. Also
 * the type. If it is A8 or A4 you need 'color' set.
 * for L8 or L4 you need the pointer to the color map
 * set. For transparency effects make sure that the color
 * that represents transparency (typically 0) has ALPHA
 * set to 0.
 */
typedef struct __dma2d_bitmap  {
	void		*buf;	/* this is where the pixel data is */
	int		mode;	/* This is the bitmap 'mode' */
	int		w, h;	/* width and height */
	int		stride;	/* distance between 'lines' in the bitmap */
	DMA2D_COLOR	fg, bg;	/* Default colors on A4/A8 bitmaps */
/* should this be a DMA2D_COLOR pointer ? */
	int		maxc;	/* Maximum size of the CLUT table */
	uint32_t	*clut;	/* the color lookup table */
} DMA2D_BITMAP;

void dma2d_clear(DMA2D_BITMAP *bm, DMA2D_COLOR color);
void dma2d_render(DMA2D_BITMAP *src, DMA2D_BITMAP *dst, int x, int y);
/*
 * render 4 bit, 8 bit, 16 bit, 24 bit, or 32 bit pixels into a
 * buffer. The lower bits are used in the passed in value.
 */
void dma2d_draw_4bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel);
void dma2d_draw_8bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel);
void dma2d_draw_16bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel);
void dma2d_draw_24bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel);
void dma2d_draw_32bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel);

#endif /* generic header protector */
