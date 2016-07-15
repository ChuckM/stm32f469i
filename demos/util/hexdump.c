/*
 * hexdump - create simple hexadecimal "dumps" of memory
 * regions or buffers.
 */
#include <stdio.h>
#include <stdint.h>
#include "../util/util.h"

/*
 * This code are some routines that implement a "classic"
 * hex dump style memory dump. So you can look at what is
 * in the RAM, alter it (in a couple of automated ways)
 */
static uint8_t *dump_line(uint32_t, uint8_t *, int);
static uint8_t *dump_page(uint32_t, uint8_t *, int);


/*
 * This routine does a simple "hex" dump to the console
 * it keeps non-ascii characters from printing.
 */
void
hex_dump(uint32_t addr, uint8_t *data, unsigned int len)
{
	dump_page(addr, data, len);
}

/*
 * dump a 'line' (an address, up to 16 bytes, and then the
 * ASCII representation of those bytes) to the console.
 * Takes an address (and possiblye a 'base' parameter
 * so that you can offset the address) and sends 16
 * bytes out. Returns the address +16 so you can
 * just call it repeatedly and it will send the
 * next 16 bytes out.
 */
static uint8_t *
dump_line(uint32_t addr, uint8_t *buf, int len)
{
	int i;
	uint8_t b;

	printf(console_color(WHITE));
	printf("%08X | ", (unsigned int) addr);
	printf(console_color(GREEN));
	for (i = 0; i < 16; i++) {
		if (i < len) {
			printf("%02X ", (uint8_t) *(buf + i));
		} else {
			printf("   ");
		}
		if (i == 7) {
			printf("  ");
		}
	}
	printf("%s| ", console_color(YELLOW));
	for (i = 0; i < 16; i++) {
		if (i < len) {
			b = *buf++;
			printf("%c", (((b == 126) || (b < 32) || (b == 255)) ? '.' : (char) b));
		} else {
			printf(" ");
		}
	}
	printf("%s\n", console_color(NONE));
	return buf;
}


/*
 * dump a 'page' like the function dump_line except this
 * does 16 lines for a total of 256 bytes. Back in the
 * day when you had a 24 x 80 terminal this fit nicely
 * on the screen with some other information.
 */
static uint8_t *
dump_page(uint32_t addr, uint8_t *buf, int len)
{
	int i;
	for (i = 0; (i < 16) && (len > 0); i++) {
		buf = dump_line(addr, buf, (len < 16) ? len : 16);
		addr += 16;
		len -= 16;
	}
	return buf;
}
