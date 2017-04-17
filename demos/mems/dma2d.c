/*
 * dma2d.c -- Utility functions for DMA2D peripheral
 *
 * Copyright (c) 2016-2017 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * <license>
 *
 * These are some helper functions I've written for the
 * DMA2D device, see the dma2d.h file for the API.
 */

#include <stdint.h>
#include "dma2d.h"

/*
 * dma2d_draw_4bpp -- Write nyblets into the framebuffer
 */
void
dma2d_draw_4bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	uint8_t *p = ((uint8_t *) fb->bm) + y * fb->stride + (x >> 1);
	if ((x & 1) == 0) {
		*p = *p & 0xf0 | (pixel & 0xf);
	} else {
		*p = *p & 0xf | ((pixel & 0xf) << 4);
	}
}

/*
 * dma2d_draw_8bpp -- Write bytes into the framebuffer
 */
void
dma2d_draw_16bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->bm) + y * fb->stride + x) = pixel & 0xff;
}

/*
 * dma2d_draw_16bpp -- Write shorts into the framebuffer
 */
void
dma2d_draw_16bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->bm) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
}

/*
 * dma2d_draw_24bpp -- Write tri-bytes into the framebuffer
 */
void
dma2d_draw_24bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->bm) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 2) = (pixel >> 16) & 0xff;
}

/*
 * dma2d_draw_32bpp -- Write longs into the framebuffer
 */
void
dma2d_draw_24bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->bm) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 2) = (pixel >> 16) & 0xff;
	*((uint8_t *)(fb->bm) + y * fb->stride + x + 3) = (pixel >> 24) & 0xff;
}

/*
 * Clear the bitmap to a single color value.
 */
void
dma2d_clear(DMA2D_BITMAP *bm, DMA2D_COLOR color);
{
    DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
    DMA2D_OPFCCR = 0x0; /* ARGB8888 pixels */
    /* force it to have full alpha */
    DMA2D_OCOLR = color.raw;
    DMA2D_OOR = 0;
    DMA2D_NLR = DMA2D_SET(NLR, PL, bm->w) | bm->h; 
    DMA2D_OMAR = (uint32_t) bm->fb;

    /* kick it off */
    DMA2D_CR |= DMA2D_CR_START;
}

void
dma2d_render(DMA2D_BITMAP *bm, int x, int y)
{
}
