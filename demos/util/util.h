
/*
 * This include file describes the functions exported by clock.c
 */
#pragma once
#include <stdint.h>
#include <stdlib.h>

/*
 * Definitions for clock functions
 */
/*
 *  This define will assemble the various dividers into a set of
 * bits you can pass to pll_clock_setup.
 */
#define PLL_CONFIG_BITS(pllr, pllp, plln, pllq, pllm, src)  (\
		((pllr & RCC_PLLCFGR_PLLR_MASK) << RCC_PLLCFGR_PLLR_SHIFT) |\
		((pllp & RCC_PLLCFGR_PLLP_MASK) << RCC_PLLCFGR_PLLP_SHIFT) |\
		((plln & RCC_PLLCFGR_PLLN_MASK) << RCC_PLLCFGR_PLLN_SHIFT) |\
		((pllq & RCC_PLLCFGR_PLLQ_MASK) << RCC_PLLCFGR_PLLQ_SHIFT) |\
		((pllm & RCC_PLLCFGR_PLLM_MASK) << RCC_PLLCFGR_PLLM_SHIFT) |\
		((src & RCC_PLLCFGR_PLLSRC_MASK) << RCC_PLLCFGR_PLLSRC_SHIFT) )

struct pll_parameters {
	int pllr;
	int pllm;
	int plln;
	int pllp;
	int pllp_real;
	int pllq;
	int src;
};
struct pll_parameters *dump_clock(void);

void hsi_clock_setup(uint32_t hsi_frequency);
void hse_clock_setup(uint32_t hse_frequency);
void pll_clock_setup(uint32_t pll_parameters, uint32_t input_frequency);
uint32_t clock_setup(uint32_t desired_frequency, uint32_t input_frequency);
uint32_t mtime(void);
void msleep(uint32_t millis);
unsigned char *time_string(uint32_t millis);

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
void all_leds_on(void);
void all_leds_off(void);
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
#ifndef DMA2D_
typedef union __gfx_color {
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
		uint32_t	a:8;
	} c;
	uint32_t	raw;
} DMA2D_;
#endif
#endif

/* #define FRAMEBUFFER_ADDRESS (0xc0000000U - (800U * 480U * 4U)) */
#define FRAMEBUFFER_ADDRESS (0xc1000000U - 0x200000U)
void lcd_init(void);
void lcd_clear(uint32_t color);
void lcd_flip(int te_locked);
void lcd_draw_pixel(void *buf, int x, int y, uint32_t color);

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

/* Pre filled bitmap structure describing the LCD screen */
extern DMA2D_BITMAP lcd_screen;

int dma2d_mode_to_bpp(int mode);
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

/* 50 % opaque black */
#define DMA2D_COLOR_SHADOW	(DMA2D_COLOR){.raw=0x80000000}

/* Transparent black */
#define DMA2D_COLOR_CLEAR		(DMA2D_COLOR){.raw=0x00000000}

#define DMA2D_COLOR_BLACK		(DMA2D_COLOR){.argb8888={0x00, 0x00, 0x00, 0xff}}
#define DMA2D_COLOR_DKGREY		(DMA2D_COLOR){.argb8888={0x55, 0x55, 0x55, 0xff}}
#define DMA2D_COLOR_GREY		(DMA2D_COLOR){.argb8888={0x80, 0x80, 0x80, 0xff}}
#define DMA2D_COLOR_LTGREY		(DMA2D_COLOR){.argb8888={0xaa, 0xaa, 0xaa, 0xff}}
#define DMA2D_COLOR_WHITE		(DMA2D_COLOR){.argb8888={0xff, 0xff, 0xff, 0xff}}
#define DMA2D_COLOR_DKBLUE		(DMA2D_COLOR){.argb8888={0xaa, 0x00, 0x00, 0xFF}}
#define DMA2D_COLOR_BLUE		(DMA2D_COLOR){.argb8888={0xff, 0x55, 0x55, 0xff}}
#define DMA2D_COLOR_LTBLUE		(DMA2D_COLOR){.argb8888={0xff, 0xaa, 0xaa, 0xff}}
#define DMA2D_COLOR_DKGREEN		(DMA2D_COLOR){.argb8888={0x00, 0xaa, 0x00, 0xff}}
#define DMA2D_COLOR_GREEN		(DMA2D_COLOR){.argb8888={0x55, 0xff, 0x55, 0xff}}
#define DMA2D_COLOR_LTGREEN		(DMA2D_COLOR){.argb8888={0xaa, 0xff, 0xaa, 0xff}}
#define DMA2D_COLOR_DKCYAN		(DMA2D_COLOR){.argb8888={0xaa, 0xaa, 0x00, 0xff}}
#define DMA2D_COLOR_CYAN		(DMA2D_COLOR){.argb8888={0xff, 0xff, 0x55, 0xff}}
#define DMA2D_COLOR_LTCYAN		(DMA2D_COLOR){.argb8888={0xff, 0xff, 0xaa, 0xff}}
#define DMA2D_COLOR_DKRED		(DMA2D_COLOR){.argb8888={0x00, 0x00, 0xaa, 0xff}}
#define DMA2D_COLOR_RED			(DMA2D_COLOR){.argb8888={0x55, 0x55, 0xff, 0xff}}
#define DMA2D_COLOR_LTRED		(DMA2D_COLOR){.argb8888={0xaa, 0xaa, 0xff, 0xff}}
#define DMA2D_COLOR_DKMAGENTA	(DMA2D_COLOR){.argb8888={0xaa, 0x00, 0xaa, 0xff}}
#define DMA2D_COLOR_MAGENTA		(DMA2D_COLOR){.argb8888={0xff, 0x55, 0xff, 0xff}}
#define DMA2D_COLOR_LTMAGENTA	(DMA2D_COLOR){.argb8888={0xff, 0xaa, 0xff, 0xff}}
#define DMA2D_COLOR_DKYELLOW	(DMA2D_COLOR){.argb8888={0x00, 0xaa, 0xaa, 0xff}}
#define DMA2D_COLOR_YELLOW		(DMA2D_COLOR){.argb8888={0x55, 0xff, 0xff, 0xff}}
#define DMA2D_COLOR_LTYELLOW	(DMA2D_COLOR){.argb8888={0xaa, 0xff, 0xff, 0xff}}

/* If you are going to move the heap, implement this function to set the start
 * and end points for the heap.
 */
void local_heap_setup(uint8_t **start, uint8_t **end);

/*
 *
 * The utility functions for i2c
 *
 */

#define I2C_100KHZ	0
#define I2C_400KHZ	1
#define I2C_1MHZ	2

/*
 * An i2c "device" on the i2c bus 'i2c' with address 'addr'
 */
typedef struct {
	int			i2c;	/* 1, 2, or 3. */
	uint8_t		addr;
} i2c_dev;

/*
 * Prototypes for functions.
 */

i2c_dev *i2c_init(int i2c, uint8_t addr, uint8_t baud);

int	i2c_write(i2c_dev *chan, uint8_t *buf, size_t buf_size, int send_stop);
int	i2c_read(i2c_dev *chan, uint8_t *buf, size_t buf_size, int send_stop);

/*
 * Touch panel controller parameters
 */

/* A single touch point (up to 2 per event) */
typedef struct {
	int	evt;
	int tid;
	int x, y;
	int weight, misc;
} touch_point;

/* A single touch event */
typedef struct {
	int				n;	/* number of valid touch points */
	touch_point 	tp[2];
} touch_event;

/* check for touch event, block if wait is true */
touch_event *get_touch(int wait);
