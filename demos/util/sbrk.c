/*
 * sbrk.c
 *
 * This implements a larger heap than you get by default
 * in newlib. It uses the DRAM as the heap. The 'frame buffer'
 * for the LCD display is the last 1.5MB and this is 1MB before
 * that.
 *
 * It is adapted from the one that is in newlib, it just uses
 * the DRAM memory instead of __heap_start in the linker script.
 * that stays the way it is because when we don't have DRAM
 * configured we don't want to try to use it for dynamic memory!
 */
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include "../util/util.h"

/* 1 MB of heap space */
#define MAX_HEAP_SIZE	0x100000ul

static uint8_t *_cur_brk = (uint8_t *) (FRAMEBUFFER_ADDRESS - MAX_HEAP_SIZE);
void *_sbrk_r(struct _reent *, ptrdiff_t );

void *_sbrk_r(struct _reent *reent, ptrdiff_t diff)
{
	uint8_t *_old_brk = _cur_brk;
    if (_cur_brk + diff > (uint8_t *)(FRAMEBUFFER_ADDRESS)) {
        reent->_errno = ENOMEM;
        return (void *)-1;
    }
    _cur_brk += diff;
    return _old_brk;
}

