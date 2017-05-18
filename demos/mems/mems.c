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
#include "reticle.h"
/*
 * MEMS Setup
 * -------------------------------------------------------
 */

#define SAMP_SIZE	1024
#define	SAMP_RATE	8192

void printvp(GFX_VIEW *vp);

void
printvp(GFX_VIEW *vp)
{
	printf("VP: [min_x, min_y], [max_x, max_y] = [%f, %f], [%f, %f]\n",
				vp->min_x, vp->min_y, vp->max_x, vp->max_y);
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
	uint32_t t0, t1;
	double max_epsilon, avg_epsilon;
	sample_buffer *signal, *fft, *mag0;
	reticle_t *reticle;
	GFX_CTX	*g;
	GFX_VIEW *vp;
	GFX_VIEW local_vp;

	signal = alloc_buf(1024);
	mag0 = alloc_buf(1024);
	fft = alloc_buf(1024);

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	fprintf(stderr, "FFT Exploration Demo program V2.2\n");

	g = gfx_init(NULL, lcd_draw_pixel, 800, 480, GFX_FONT_LARGE,
										(void *)FRAMEBUFFER_ADDRESS);
	dma2d_clear(&lcd_screen, DMA2D_COLOR_BLACK);
	reticle = create_reticle("RF Power", 640, 480, GFX_FONT_LARGE,
		"Frequency", 0, 1.0, 
		"Power", -120, 0);
	dma2d_render(&(reticle->bm), &lcd_screen, 0, 0);

	/* fill signal buffer with test data */
	// add_triangle(signal, 110.0, 1.0);
	// add_cos(signal, 400.0, 1.5);
/*
	high res wave
	box-w (number of increments)
	150hz is our slowest wave
	1/150 is one cycle, times 2*pi, divided by number of bits
 */
	signal->r = (150 * reticle->b_w) / (2 * M_PI) ;
	add_cos(signal, 150.0, 1.0);
	add_cos(signal, 300.0, 1.0);
	// add_cos(signal, 600.0, 1.5);

	/*
	 * show our original signal
	 */
#define ONE_WAVE reticle->b_w
	
	vp = gfx_viewport(&local_vp, g, reticle->o_x, reticle->o_y, reticle->b_w, reticle->b_h,
			0, signal->sample_min, ONE_WAVE, signal->sample_max);
	for (i = 1; i < ONE_WAVE; i++) {
		vp_plot(vp, i - 1, *(signal->data + (i -1)),
					    i, *(signal->data + i), GFX_COLOR_DKGREEN);
	}
	lcd_flip(0);

	/* fill signal buffer with test data */
	// add_triangle(signal, 110.0, 1.0);
	// add_cos(signal, 400.0, 1.5);
	clear_samples(signal);
	signal->r = 512;
	add_cos(signal, 150.0, 1.0);
	add_cos(signal, 300.0, 1.0);
	// add_cos(signal, 600.0, 1.5);



	/*
	 * Apply the DFT and compute real and imaginary
	 * components.
	 */

	bins = 1024;
	t0 = mtime();
	calc_fft(signal, bins, mag0);
	t1 = mtime();
	printf("FFT Compute time %ld milliseconds\n", t1 - t0);

	/* Need a better way to 'label' these viewports */
	gfx_set_text_size(g, 2);
	gfx_set_text_color(g, GFX_COLOR_YELLOW, GFX_COLOR_YELLOW);
	gfx_set_text_cursor(g, reticle->o_x + 5,
						   reticle->o_y+15 + gfx_get_text_height(g));
	gfx_puts(g, "FFT");

	vp = gfx_viewport(&local_vp, g, reticle->o_x, reticle->o_y,
						 reticle->b_w, reticle->b_h/2,
						 0, mag0->sample_min, (float) bins, mag0->sample_max);
	printvp(vp);
	for (i = 1; i < bins; i++) {
		vp_plot(vp, i - 1, mag0->data[i - 1],
				    i, mag0->data[i], GFX_COLOR_YELLOW);
	}
	lcd_flip(0);

	signal->r = 512;
	add_cos(signal, 146.0, 1.0);
	add_cos(signal, 148.0, 1.0);
	add_cos(signal, 144.0, 1.0);
	add_cos(signal, 150.0, 1.0);
	add_cos(signal, 152.0, 1.0);
	add_cos(signal, 154.0, 1.0);
	add_cos(signal, 156.0, 1.0);
	add_cos(signal, 300.0, 1.0);

	printf("Compute FFT\n");
	t0 = mtime();
	calc_fft(signal, bins, fft);
	t1 = mtime();
	printf("Computed FFT in %ld milliseconds\n", t1 - t0);
	gfx_set_text_color(g, GFX_COLOR_CYAN, GFX_COLOR_CYAN);
	gfx_set_text_cursor(g, reticle->o_x + 5,
						   reticle->o_y + 15 + reticle->b_h / 2 + gfx_get_text_height(g));
	gfx_puts(g, "FFT (real)");
	vp = gfx_viewport(&local_vp, g, reticle->o_x, reticle->o_y + reticle->b_h/2, 
						 reticle->b_w, reticle->b_h/2,
						 0, fft->sample_min, (float) bins / 2, fft->sample_max);
	printvp(vp);
	for (i = 1; i < bins / 2; i++) {
		vp_plot(vp, i - 1, fft->data[i - 1], i, fft->data[i], GFX_COLOR_CYAN);
	}
	lcd_flip(0);
	max_epsilon = avg_epsilon = 0;
	for (int j = 0; j < bins; j++) {
		double td;
		td = abs(mag0->data[j] - fft->data[j]);
		max_epsilon = max(td, max_epsilon);
		avg_epsilon += td;
	}
	avg_epsilon /= (double) bins;
	printf("Differences, max epsilon = %f, average %f\n",
		max_epsilon, avg_epsilon);
	while (1) ;
}
