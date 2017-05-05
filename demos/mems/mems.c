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
#include "signal.h"
#include "dma2d.h"

/*
 * MEMS Setup
 * -------------------------------------------------------
 */

#define DMA2D_SHADOW	(DMA2D_COLOR){.raw=0x80000000}
#define DMA2D_RED	(DMA2D_COLOR){ .raw=0xffff0000}
#define DMA2D_BLACK	(DMA2D_COLOR){.raw=0xff000000}
#define DMA2D_WHITE	(DMA2D_COLOR){.raw=0xffffffff}
#define DMA2D_GREY	(DMA2D_COLOR){.raw=0xff444444}

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

#define SAMP_SIZE	1024
#define	SAMP_RATE	8192

typedef struct {
	float	*d;
	int	sz;
	float	max_val;
	float	min_val;
} DFT_RESULT;

float *sample_data, *result_data, *re_x, *im_x, *fft_data;

void dft(sample_buffer *s, float min_freq, float max_freq, int bins, float rx[], float ix[], float mag[]);

/* 
 * dft( ... )
 *
 * Compute the Discrete Fourier Transform using the
 * correlation method. This out of DSP for engineers and scientists 
 *
*/
#define DEBUG_DFT
void
dft(sample_buffer *s, float min_freq, float max_freq, int bins, float rx[], float ix[], float mag[])
{
	int	i, k;
	float peak_freq, max_value;

#ifdef DEBUG_DFT
	printf("DFT : %fhz through %fhz (%fhz)\n",
		min_freq, min_freq + (float) (bins - 1) * (max_freq - min_freq) / (float) bins, max_freq);
#endif
	max_value = 0;
	peak_freq = 0;
	/* run through each bin */
	for (k = 0; k < bins; k++) {
		float current_freq;
#ifdef DEBUG_DFT
		printf("\r  %d of %d ... ", k, bins);
		fflush(stdout);
#endif
		/* current frequency based on bin # and frequency span */
		current_freq = min_freq + k * (max_freq - min_freq) / (float) bins;

		rx[k] = 0;
		ix[k] = 0;
		mag[k] = 0;
		/* correlate this frequency with each sample */
		for (i = 0; i < s->n; i++) {
			float r;

			/* compute correlation */
			r = 2 * M_PI * current_freq * i / s->r;
			rx[k] = rx[k] + s->data[i] * cos(r);
			ix[k] = ix[k] + (- s->data[i] * sin(r));
		}
		/* magnitude of the correlation */
		mag[k] = sqrt(rx[k] * rx[k] + ix[k] * ix[k]);
#ifdef DEBUG_DFT
		if (mag[k] >= max_value) {
			peak_freq = current_freq; 
			max_value = mag[k];
		}
#endif
	}
#ifdef DEBUG_DFT
	printf("Done.\n");
	printf("Max value: %f, frequency %f\n", max_value, peak_freq);
#endif

}

/*
 * Lets see if we can look at MEMs microphones
 * Mems microphone comes in on
 * PD6 (SPI3 MOSI/5) (SAI1_SDA/6) (USART2 RX/7) (FMC_NWAIT/12) (DCMI_D10/13) (LCD_B2/14) 
 * MEMs clock is on 
 * PD13 (TM4_CH2/2) (QUADSPI_BK1_IO3/9) (FMC_A18/12)
 * So somehow we use timer 4 to clock USART2 in a synchronous mode to get microphone data.
 * 
 */
int
main(void) {
	int i, bins;
	float data_min, data_max;
	sample_buffer sb;
	GFX_CTX	*g;
	GFX_VIEW *vp;

	sample_data = (float *)(0xc0000000);
	result_data = sample_data + SAMP_SIZE;
	re_x = result_data + SAMP_SIZE;
	im_x = re_x + SAMP_SIZE;
	fft_data = im_x + SAMP_SIZE;

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "FFT Exploration Demo program\n");

	g = gfx_init(lcd_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *)FRAMEBUFFER_ADDRESS);
	dma2d_clear(&screen, DMA2D_GREY);
	(void) create_reticule();
	dma2d_render((DMA2D_BITMAP *)&reticule, &screen, 0, 0);

	sb.data = sample_data;
	sb.n = SAMP_SIZE;
	sb.r = SAMP_RATE;
	memset(sample_data, 0, sizeof(float) * SAMP_SIZE);


	
	/* fill sample data with a signal */
	add_cos(&sb, 400.0, 1.0);
	add_cos(&sb, 500.0, 1.0);
	add_cos(&sb, 600.0, 1.0);

	
	/*
	 * show our original signal
	 */
	data_min = data_max = 0;
	for (i = 0; i < sb.n; i++) {
		data_min = (sample_data[i] < data_min) ? sample_data[i] : data_min;
		data_max = (sample_data[i] > data_max) ? sample_data[i] : data_max;
	}
	vp = gfx_viewport(g, reticule.o_x, reticule.o_y, reticule.b_w, reticule.b_h,
			0, data_min, (float) sb.n, data_max);
	printf("VP: [min_x, min_y], [max_x, max_y] = [%f, %f], [%f, %f]\n", 
				0.0, data_min, (double) sb.n, data_max);
	for (i = 1; i < sb.n; i++) {
		vp_plot(vp, i - 1, sample_data[i -1], i, sample_data[i], GFX_COLOR_DKGREY);
	}

	/*
	 * Apply the DFT and compute real and imaginary
	 * components.
	 */

	bins = reticule.b_w; /* scaled to size of reticule */
	dft(&sb, 300.0, 700.0, bins, re_x, im_x, fft_data);	/* compute DFT per article */

#ifdef PLOT_RAW
	data_min = data_max = 0;
	for (i = 0; i < sb.n / 2; i++) {
		data_min = (re_x[i] < data_min) ? re_x[i] : data_min;
		data_max = (re_x[i] > data_max) ? re_x[i] : data_max;
		data_min = (im_x[i] < data_min) ? im_x[i] : data_min;
		data_max = (im_x[i] > data_max) ? im_x[i] : data_max;
	}
	vp = gfx_viewport(g, reticule.o_x, reticule.o_y, reticule.b_w, reticule.b_h,
			0, data_min, (float) sb.n / 2.0, data_max);
	printf("VP: [min_x, min_y], [max_x, max_y] = [%f, %f], [%f, %f]\n", 
		0.0, data_min, (double) bins, data_max);
	for (i = 1; i < bins; i++) {
		vp_plot(vp, i - 1, re_x[i - 1], i, re_x[i], GFX_COLOR_MAGENTA);
		vp_plot(vp, i - 1, im_x[i -1], i, im_x[i], GFX_COLOR_CYAN);
	}
#endif

	/*
	 * Build DFT_RESULT structure 
	 * Plot the magnitude data from the DFT
	 */
	data_min = data_max = 0;
	for (i = 0; i < bins; i++) {
		data_min = (fft_data[i] < data_min) ? fft_data[i] : data_min;
		data_max = (fft_data[i] > data_max) ? fft_data[i] : data_max;
	}

	vp = gfx_viewport(g, reticule.o_x, reticule.o_y, reticule.b_w, reticule.b_h,
			0, data_min, (float) bins, data_max);
	printf("VP: [min_x, min_y], [max_x, max_y] = [%f, %f], [%f, %f]\n", 
		0.0, data_min, (float)  bins, data_max);
	for (i = 1; i < bins; i++) {
		vp_plot(vp, i - 1, fft_data[i - 1], i, fft_data[i], GFX_COLOR_YELLOW);
	}

	lcd_flip(0);
	while (1) ;
}
