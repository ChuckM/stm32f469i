#pragma once

typedef struct {
	DMA2D_BITMAP	bm;
	int o_x, o_y;
	int b_w, b_h;
} reticule_t;

reticule_t *create_reticule(void);
