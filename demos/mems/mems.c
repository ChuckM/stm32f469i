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
#include <libopencm3/stm32/gpio.h>
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

/* This points at the bit banded address of PD6 */
int *pd6 = (int *)(0x42000000U + ((((GPIOD - GPIOA)) + 0x10) * 32) + (6 * 4));
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
	int i, frame_cnt, bins;
	uint32_t t0, t1;
	sample_buffer *signal, *mag0;
	reticle_t *reticle;
	GFX_CTX	*g;
	GFX_VIEW *vp;
	GFX_VIEW local_vp;

	signal = alloc_buf(1024);
	mag0 = alloc_buf(1024);

	/* Enable the clock to the DMA2D device */
	rcc_periph_clock_enable(RCC_DMA2D);
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	gpio_mode_setup(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO6);

	fprintf(stderr, "FFT Exploration Demo program V2.2\n");

	g = gfx_init(NULL, lcd_draw_pixel, 800, 480, GFX_FONT_LARGE,
										(void *)FRAMEBUFFER_ADDRESS);

	printf("Frame buffer address is : 0x%x\n", FRAMEBUFFER_ADDRESS);

	reticle = create_reticle("RF Power", 640, 480, GFX_FONT_LARGE,
		"Frequency", 0, 512, 
		"Power", -120, 0);
	vp = gfx_viewport(&local_vp, g, reticle->o_x + 2, reticle->o_y + 2,
									reticle->b_w - 4, reticle->b_h - 4,
			0, 0, reticle->b_w - 4, 1.0);

	dma2d_clear(&lcd_screen, DMA2D_COLOR_BLACK); /* option 1 this does not work? */
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
	
	vp_rescale(vp, 0, signal->sample_min, ONE_WAVE, signal->sample_max);
	for (i = 1; i < ONE_WAVE; i++) {
		vp_plot(vp, i - 1, *(signal->data + (i -1)),
					    i, *(signal->data + i), GFX_COLOR_DKGREEN);
	}
	lcd_flip(0);

	frame_cnt = 0;
	vp_rescale(vp, 0, -1.0, 1024, 1.0);
	while (1) {
		reset_minmax(signal);
		dma2d_clear(&lcd_screen, DMA2D_COLOR_BLACK); /* option 1 this does not work? */
		dma2d_render(&(reticle->bm), &lcd_screen, 0, 0); 
		printf("%d\n", frame_cnt++);
		t0 = mtime();
		for (i = 0; i < 1024; i++) {
			signal->data[i] = 0;
#define SAMPLE_MAX_BITS	256
			for (int k = 0; k < SAMPLE_MAX_BITS; k++) {
				gpio_clear(GPIOD, GPIO13);
				gpio_set(GPIOD, GPIO13);
				signal->data[i] += *pd6;
			}
			signal->data[i] = signal->data[i] / 256;
			set_minmax(signal, i);
		}
		t1 = mtime();
		printf("Sample time: %d mS\n", t1 - t0);
		for (i = 1; i < 1024; i++) {
			vp_plot(vp, i-1, signal->data[i-1], i, signal->data[i], GFX_COLOR_YELLOW);
		}
		gfx_set_text_size(g, 2);
		gfx_set_text_color(g, GFX_COLOR_YELLOW, GFX_COLOR_YELLOW);
		gfx_set_text_cursor(g, reticle->o_x + 5,
						   reticle->o_y+15 + gfx_get_text_height(g));
		gfx_puts(g, "FFT");
		printf("Signal min/max %f/%f\n", signal->sample_min, signal->sample_max);
		lcd_flip(0);
		msleep(1000);
	}

				

	/* fill signal buffer with test data */
	// add_triangle(signal, 110.0, 1.0);
	// add_cos(signal, 400.0, 1.5);
	clear_samples(signal);
	signal->r = 512;
	add_cos(signal, 150.0, 1.0);
	add_cos(signal, 300.0, 1.0);
	add_cos(signal, 146.0, 1.0);
	add_cos(signal, 148.1, 1.0);
	add_cos(signal, 144.2, 1.0);
	add_cos(signal, 150.3, 1.0);
	add_cos(signal, 152.4, 1.0);
	add_cos(signal, 154.5, 1.0);
	add_cos(signal, 156.6, 1.0);
	add_cos(signal, 300.0, 1.0);



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

	vp_rescale(vp, 0, mag0->sample_min, (float) bins/2, mag0->sample_max);
	printvp(vp);
	for (i = 1; i < bins/2; i++) {
		vp_plot(vp, i - 1, mag0->data[i - 1],
				    i, mag0->data[i], GFX_COLOR_YELLOW);
	}
	lcd_flip(0);
	while (1) ;
}
