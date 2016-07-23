#include <stdio.h>
#include <stdlib.h>
#include "../util/util.h"
#include <gfx.h>


void draw_pixel(int x, int y, uint16_t color);

#define GWIDTH	16	
#define GHEIGHT	256
uint8_t buffer[GWIDTH*GHEIGHT]; 

static const char HEX_CHARS[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F'
};

void
draw_pixel(int x, int y, uint16_t color)
{
	buffer[y*16 + x] = color & 0xff;
}

int
main(void)
{
	uint8_t page[256];
	uint32_t addr;
	uint32_t t0,t1;
	uint32_t sum;

	printf("QUAD SPI Mapping Demo\n");

	/* This code fills the test sector data with
     * some information. In this case PG 00 which
	 * is easy to see it is correct (or not) in
	 * the memory dump. 
	 */
	printf("Filling the first sector with recognizable data.\n");
	gfx_init(draw_pixel, GWIDTH, GHEIGHT, GFX_FONT_LARGE);
	gfx_fillScreen((uint16_t) ' ');
	gfx_drawRoundRect(0, 0, GWIDTH, GHEIGHT, 3, (uint16_t) '@');
	gfx_setTextColor((uint16_t) '*', (uint16_t) ' ');
	gfx_setTextRotation(GFX_ROT_90);
	gfx_setCursor(3, 3);
	gfx_puts((unsigned char *)"   QSPI FLASH Mapping Test   ");

	qspi_erase_block(0);
	qspi_write_flash(0, &buffer[0], GWIDTH*GHEIGHT);
	qspi_read_flash(0, page, 256);
	printf("Memory dump at 0x90000000 should look like:\n");
	hex_dump(0x90000000, page, 256);
	printf("Mapping Flash into the address space ...\n");
	qspi_map_flash();
	addr = 0x90000000;
	printf("Simple speed test\n");
	t0 = mtime();
	sum = 0;
	for (addr = 0x90000000; addr < 0x91000000; addr += 4) {
		sum += *(uint32_t *)(addr);
	}
	t1 = mtime();
	printf("Start time was %u\n", (unsigned int)t0);
	printf("End time was %u\n", (unsigned int)t1);
	printf("Summed 16MB into %u in %u MS\n", (unsigned int)sum, (unsigned int) (t1-t0));
	printf("   Read speed %f MB/second\n", 16000.0 / (float)(t1 - t0));
	addr =0x90000000;
	
	while (1) {
		char c;

/*		qspi_read_flash(addr, &page[0], 256); */
		hex_dump(addr, (uint8_t *)(addr), 256);
		toggle_led(RED_LED);
		c = console_getc(1);
		if ((c == '-') || (c == '_')) {
			addr = addr - 0x100;
			if (addr < 0x90000000) {
				addr = 0x90000000;
			}
		} else if ((c == '+') || (c == '=')) {
			addr = addr + 0x100;
			if (addr > 0x90ffff00) {
				addr = 0x90ffff00;
			}
		}
	}
}
