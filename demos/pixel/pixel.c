#include <stdint.h>
#include <stdio.h>
#include <gfx.h>
#include "../util/util.h"

#ifdef old_style
typedef union __dma2d_pixel {
    struct {
        uint8_t    b:5;
        uint8_t    g:6;
        uint8_t    r:5;
    } rgb565;
    struct {
        uint8_t b:8;
        uint8_t g:8;
        uint8_t r:8;
        uint8_t a:8;
    } argb8888;
    struct {
        uint8_t b:8;
        uint8_t g:8;
        uint8_t r:8;
    } rgb888;
    struct {
        uint8_t    b:5;
        uint8_t    g:5;
        uint8_t    r:5;
        uint8_t    a:1;
    } argb1555;
    struct {
        uint8_t    b:4;
        uint8_t    g:4;
        uint8_t    r:4;
        uint8_t    a:4;
    } argb4444;
    struct {
        uint8_t    l:8;
        uint8_t    a:8;
    } al88;
    struct {
        uint8_t    l:4;
        uint8_t    a:4;
    } al44;
    struct {
        uint8_t    a:8;
    } a8;
    struct {
        uint8_t    l:8;
    } l8;
    uint32_t raw;
} pixel_t;
#else 
/* 8 bit pixel definitions */
typedef union __dma2d_pixel_8 {
    struct {
        uint8_t    a:8;
    } a8;
    struct {
        uint8_t    l:8;
    } l8;
	struct {
		uint8_t		l:4;
		uint8_t		a:4;
	} al44;
/*
	struct {
		uint8_t		l:4;
	} l4[2];
	struct {
		uint8_t		a:4;
	} a4[2];
*/
	uint8_t	raw;
} pixel8_t;

/* 16 bit pixel definitions */
typedef union __dma2d_pixel_16 {
    struct {
        uint16_t    b:5;
        uint16_t    g:6;
        uint16_t    r:5;
    } rgb565;
    struct {
        uint16_t    b:5;
        uint16_t    g:5;
        uint16_t    r:5;
        uint16_t    a:1;
    } argb1555;
    struct {
        uint16_t    b:4;
        uint16_t    g:4;
        uint16_t    r:4;
        uint16_t    a:4;
    } argb4444;
    struct {
        uint16_t    l:8;
        uint16_t    a:8;
    } al88;
    uint16_t raw;
} pixel16_t;

/* 32 bit pixel definitions */
typedef union __dma2d_pixel_32 {
    struct {
        uint8_t b:8;
        uint8_t g:8;
        uint8_t r:8;
        uint8_t a:8;
    } argb8888;
    struct {
        uint8_t b:8;
        uint8_t g:8;
        uint8_t r:8;
    } rgb888;
    uint32_t raw;
} pixel32_t;

/* A unified view of 32 bits (4 bytes) as pixels */
typedef union __dma2d_pixel {
	pixel8_t	p8[4];
	pixel16_t	p16[2];
	pixel32_t	p32;
	// uint32_t	raw;
} pixel_t;
#endif

void print_pixel(pixel_t p);

void
print_pixel(pixel_t p) {
	printf("Pixel values (0x%08X) :\n", (unsigned int) p.p32.raw);
	printf("    ARGB8888 (A, R, G, B) = (0x%02X, 0x%02X, 0x%02X, 0x%02X)\n",
		p.p32.argb8888.a, p.p32.argb8888.r, p.p32.argb8888.g, p.p32.argb8888.b);
	printf("    RG8888 (R, G, B) = (0x%02X, 0x%02X, 0x%02X)\n",
		p.p32.rgb888.r, p.p32.rgb888.g, p.p32.rgb888.b);
	printf("    RGB565 (R, G, B) = (0x%02X, 0x%02X, 0x%02X)\n",
		p.p16[0].rgb565.r, p.p16[0].rgb565.g, p.p16[0].rgb565.b);
	printf("    ARGB1555 (A, R, G, B) = (0x%02X, 0x%02X, 0x%02X, 0x%02X)\n",
		p.p16[0].argb1555.a, p.p16[0].argb1555.r, p.p16[0].argb1555.g, p.p16[0].argb1555.b);
	printf("    ARGB4444 (A, R, G, B) = (0x%02X, 0x%02X, 0x%02X, 0x%02X)\n",
		p.p16[0].argb4444.a, p.p16[0].argb4444.r, p.p16[0].argb4444.g, p.p16[0].argb4444.b);
	printf("    AL88 (A, L) = (0x%02X, 0x%02X)\n",
		p.p16[0].al88.a, p.p16[0].al88.l);
	printf("    AL44 (A, L) = (0x%02X, 0x%02X)\n",
		p.p8[0].al44.a, p.p8[0].al44.l);
	printf("    A8 (A) = (0x%02X)\n",
		p.p8[0].a8.a);
	printf("    L8 (A) = (0x%02X)\n",
		p.p8[0].l8.l);
}

#define RGB565(r, g, b)	(((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f))
#define ARGB1555(a, r, g, b)	(((a & 0x1) << 15) | ((r & 0x1f) << 10) | \
										((g & 0x1f) << 5) | (b & 0x1f))
#define ARGB4444(a, r, g, b)	(((a & 0xf) << 12) | ((r & 0xf) << 8) | \
										((g & 0xf) << 4) | (b & 0xf))

/* assume the table is 16 long words */
static void
dump_pixel_table(uint8_t *buf)
{
	int	b, l;
	uint8_t	*t = (buf + ((16 * 4) - 1));
	/* longword 15 downto 0 */
	printf("      : @ + 3  | @ + 2  | @ + 1  | @ + 0  |\n");
	printf("------+--------+--------+--------+--------+\n");
	for (l = 15; l >= 0; l--) {
		printf(" 0x%02x : ", (unsigned int)((l << 2)+3));
		/* byte 3 downto 0 */
		for (b = 3; b >= 0; b--) {
			printf(" 0x%02x  | ", (unsigned int)(*t));
			t--;
		}
		printf("\n");
	}
}

/*
 * This is a simple test program to validate that the pixels are layed out
 * in memory according to the spec in Rev 3 of  RM0386, pg 258.
 */

int
main(void)
{
	/* a bit of memory to put pixels into */
	pixel_t pix[16];
	int	i;


	printf("Pixel experiments\n");
	printf("Pixel structure size is %d bytes\n", sizeof(pixel_t));
	printf("   Pixel8 structure size is %d bytes\n", sizeof(pixel8_t));
	printf("   Pixel16 structure size is %d bytes\n", sizeof(pixel16_t));
	printf("   Pixel32 structure size is %d bytes\n", sizeof(pixel32_t));

	for (i = 0; i < 16; i++) {
		pix[i].p32.argb8888.a = 'A';
		pix[i].p32.argb8888.r = 'R';
		pix[i].p32.argb8888.g = 'G';
		pix[i].p32.argb8888.b = 'B';
	}
	dump_pixel_table((uint8_t *)&pix[0]);
	printf("Hex dump of pix[16]:\n");
	hex_dump(0, (uint8_t *) pix, sizeof(pix));

	while (1) {
		console_puts("Enter value: ");
		pix[0].p32.raw = console_getnumber();
		printf("\n");
		print_pixel(pix[0]);
	}
}

