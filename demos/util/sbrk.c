/*
 * sbrk.c
 *
 * This implements a larger heap than you get by default
 * in newlib. It uses the DRAM as the heap. The 'frame buffer'
 * for the LCD display is the last 1.5MB and this is 1MB before
 * that.
 */
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include "../util/util.h"

/* 1 MB of heap space */
#define MAX_HEAP_SIZE	0x100000ul

static uint8_t *_cur_brk = (uint8_t *) (FRAMEBUFFER_ADDRESS - MAX_HEAP_SIZE);
void *_sbrk_r(struct _reent *, ptrdiff_t );

void *_sbrk_r(__attribute__((unused)) struct _reent *reent, ptrdiff_t diff)
{
	uint8_t *_old_brk = _cur_brk;
    if (_cur_brk + diff > (uint8_t *)(FRAMEBUFFER_ADDRESS)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    _cur_brk += diff;
    return _old_brk;
}

