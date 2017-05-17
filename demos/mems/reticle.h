#pragma once

typedef struct {
	DMA2D_BITMAP	bm;
	int o_x, o_y;
	int b_w, b_h;
} reticle_t;

reticle_t * create_reticle(char *label, int w, int h, GFX_FONT font,
				char *xlabel, float min_x, float max_x, 
				char *ylabel, float min_y, float max_y);
