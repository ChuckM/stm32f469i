/*
 * makefont
 *
 * This tool uses the FreeType2 library to open a font, generate
 * an approprately sized set of glyphs, and then produce source code
 * that can be compiled and linked.
 */

#include <stdio.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdint.h>


#define WIDTH   800
#define HEIGHT  480


#define CELL_WIDTH	10
#define CELL_HEIGHT	19
#define CELL_BASELINE 5


/*
typedef struct __term_font {
	int	w;
	int	h;
	uint32_t	glyphs[256];
	uint8_t	*glyph_data;
} TERM_FONT;

TERM_FONT regular_font = {
	CHAR_WIDTH,
	CHAR_HEIGHT,
	{ 
		0x00
	},
	&__font_data[0][0][0]
};
*/
	
FILE	*font_src;
uint32_t	font_offset;

/*
 * Generate an open box shape as the "null" glyph (when a
 * character has no glyph, the terminal can print this box
 * instead. And generate the 'space' glyph since space shows
 * up as a zero width glyph in font files.
 */
void
null_glyph(void)
{
	int row, col;
	char gdata[CELL_WIDTH+1];
	uint8_t b;

	fprintf(font_src, "\t{ /* null glyph */\n");
	for ( row = 0; row < CELL_HEIGHT; row++ )
	{
		printf("|");
		fprintf(font_src, "\t\t{");
		for ( col = 0; col < CELL_WIDTH; col++)
	    {
			b = 0;
			if ((row == 1) || (row == ((CELL_HEIGHT - CELL_BASELINE) - 1))) {
				if ((col > 0) && (col < (CELL_WIDTH - 1))) {
					b = 0xff;
				}
			} else if ((row > 1) && (row < ((CELL_HEIGHT - CELL_BASELINE) - 1))) {
				if ((col == 1) || (col == (CELL_WIDTH - 2))) {
					b = 0xff;
				} 
			}

			if (b < 64) {
				putchar(' ');
				gdata[col] = ' ';
			} else if (b < 128) {
				putchar('.');
				gdata[col] = '.';
			} else if (b < 192) {
				putchar('*');
				gdata[col] = '*';
			} else {
				putchar('@');
				gdata[col] = '@';
			}
			if (col < (CELL_WIDTH-1)) {
				fprintf(font_src, "0x%02x, ", b);
			} else {
				fprintf(font_src, "0x%02x", b);
			}
		}
		gdata[CELL_WIDTH] = 0;
		fprintf(font_src, "},\t/* %10s */\n", gdata);
		printf("|\n");
	}
	fprintf(font_src, "\t},\n");
	fprintf(font_src, "\t{ /* Space glyph */\n");
	for ( row = 0; row < CELL_HEIGHT; row++ )
	{
		printf("|");
		fprintf(font_src, "\t\t{");
		for ( col = 0; col < CELL_WIDTH; col++)
	    {
			b = 0;
			if (b < 64) {
				putchar(' ');
				gdata[col] = ' ';
			} else if (b < 128) {
				putchar('.');
				gdata[col] = '.';
			} else if (b < 192) {
				putchar('*');
				gdata[col] = '*';
			} else {
				putchar('@');
				gdata[col] = '@';
			}
			if (col < (CELL_WIDTH-1)) {
				fprintf(font_src, "0x%02x, ", b);
			} else {
				fprintf(font_src, "0x%02x", b);
			}
		}
		gdata[CELL_WIDTH] = 0;
		fprintf(font_src, "},\t/* %10s */\n", gdata);
		printf("|\n");
	}
	fprintf(font_src, "\t},\n");
}

/*
 * Let's assume the font glyph is in a box that is 10 x 19 pixels
 */
void
draw_bitmap( FT_Bitmap*  bitmap,
             FT_Int      x,
             FT_Int      y,
			 unsigned char		this_glyph)
{
  FT_Int  p, q;
	int	start_x, start_y;
	int end_x, end_y;
	int	row, col;
	char gdata[CELL_WIDTH+1];
	unsigned char b;

	if (bitmap->width == 0)
		return;

	/*
	 * X and Y, are the offsets of the glyph into
	 * the glyph box, assuming that Y goes "up" from
	 * from the bottom of the box.
	 *
	 *   +---------------+
     *   |               |
     *   |    R......    |
     *   |    .......    |
     *   |    .......    |
     *   |    .......    |
     *   |b- O......W  -b| <- baseline
     *   |               |
     *   |               |
	 *   +---------------+
	 *
	 * Where O is the origin, W is the width from the
	 * bitmap, and R is the number of rows from the
	 * bitmap. Our co-ordinate system is inverted and
	 * they may use "negative" Y to indicate below
	 * baseline.
	 *
	 * So some assumptions :
	 *		box is CELL_WIDTH x CELL_HEIGHT
	 *		baseline is 5 from the bottom (row (CELL_HEIGHT - 1) -5)
     *
	 *		bottom row will be (CELL_HEIGHT - (baseline + y))
	 *		top row will be (bottom row - rows)
	 *
	 *		left column will be (x)
	 *		right column will be (left column + width)
	 *
	 *	y seems to be points above baseline.
	 *  so base line is (CELL_HEIGHT - CELL_BASELINE) - y should be start row
	 */

	start_x = x;
	end_x = x + bitmap->width;
	start_y = (CELL_HEIGHT - CELL_BASELINE) - y;
	end_y = start_y + bitmap->rows;

	if ((end_y - start_y) != bitmap->rows) {
		printf("*** Row mismatch (%d - %d) != %d\n", end_y, start_y, bitmap->rows);
	}
	if ((end_x - start_x) != bitmap->width) {
		printf("*** Width mismatch (%d - %d) != %d\n", end_x, start_x, bitmap->width);
	}
	printf("Passed in origin (x, y) => (%d, %d)\n", x, y);
	printf("Cell properties H = %d, W = %d, BL = %d\n", CELL_HEIGHT, CELL_WIDTH, CELL_BASELINE);
	printf("Glyph size (w, h) => (%d, %d)\n", bitmap->width, bitmap->rows);
	printf("Glyph box (sx, sy) => (ex, ey) : (%d, %d) => (%d, %d)\n",
		start_x, start_y, end_x, end_y);

	printf("+");
	for (col = 0; col < CELL_WIDTH; col++) {
		printf("-");
	}
	printf("+\n");
	fprintf(font_src, "\t{ /* char '%c' */\n", this_glyph);
	for ( row = 0; row < CELL_HEIGHT; row++ )
	{
		printf("|");
		fprintf(font_src, "\t\t{");
		if (row >= start_y) {
			q = row - start_y;
		}
		for ( col = 0; col < CELL_WIDTH; col++)
	    {
			if (col >= start_x) {
				p = col - start_x;
			}
			if ( ((row >= start_y) && (col >= start_x)) &&
				 ((row < end_y) && (col < end_x))) {
				b = bitmap->buffer[q * bitmap->width + p];
			} else {
				b = 0;
			}
			if (b < 64) {
				putchar(' ');
				gdata[col] = ' ';
			} else if (b < 128) {
				putchar('.');
				gdata[col] = '.';
			} else if (b < 192) {
				putchar('*');
				gdata[col] = '*';
			} else {
				putchar('@');
				gdata[col] = '@';
			}
			if (col < (CELL_WIDTH-1)) {
				fprintf(font_src, "0x%02x, ", b);
			} else {
				fprintf(font_src, "0x%02x", b);
			}
		}
		gdata[CELL_WIDTH] = 0;
		fprintf(font_src, "},\t/* %10s */\n", gdata);
		printf("|\n");
	}
	fprintf(font_src, "\t},\n");
	printf("+");
	for (col = 0; col < CELL_WIDTH; col++) {
		printf("-");
	}
	printf("+\n");
}

int
main( int     argc,
      char**  argv )
{
  FT_Library    library;
  FT_Face       face;

  FT_GlyphSlot  slot;
  FT_Matrix     matrix;                 /* transformation matrix */
  FT_Vector     pen;                    /* untransformed origin  */
  FT_Error      error;

	int	i, j;
  char*         filename;

	int	offsets_table[256];		/* table of offsets */
  int           target_height;
  int           n;
	int	max_width, max_height;

	max_width = 0;
	max_height = 0;

  if ( argc != 2 )
  {
    fprintf ( stderr, "usage: %s font\n", argv[0] );
    exit( 1 );
  }

  filename      = argv[1];                           /* first argument     */
  target_height = 19;

  error = FT_Init_FreeType( &library );              /* initialize library */
	if (error) {
		printf("Error = %d from FT_Init_FreeType\n", error);
		exit(1);
	}

  error = FT_New_Face( library, filename, 0, &face );/* create face object */
	if (error) {
		printf("Error = %d from FT_New_Face\n", error);
		exit(1);
	}

	error = FT_Set_Pixel_Sizes( face, 0, 18);
	if (error) {
		printf("Error = %d from FT_Set_Pixel_Sizes\n", error);
		exit(1);
	}
  /* error handling omitted */

  slot = face->glyph;

  /* the pen position in 26.6 cartesian space coordinates; */
  /* start at (300,200) relative to the upper left corner  */
  pen.x = 20 * 64;
  pen.y = 40 * 64;

/*
typedef struct __term_font {
	int	w;
	int	h;
	uint32_t	glyphs[256];
	uint8_t	*glyph_data;
} TERM_FONT;

TERM_FONT regular_font = {
	CHAR_WIDTH,
	CHAR_HEIGHT,
	{ 
		0x00
	},
	&__font_data[0][0][0]
};
*/
	font_src = fopen("font_src.c", "w");
	fprintf(font_src, "#include <stdint.h>\n");
	fprintf(font_src, "#include \"term.h\"\n");
	fprintf(font_src, "/* Automatically generated font data using FreeType2 */\n");
	fprintf(font_src, "/* generated from %s */\n", filename);

	fprintf(font_src, "static const uint8_t __glyph_data[][CHAR_HEIGHT][CHAR_WIDTH] = {\n");
	font_offset = 0;
	null_glyph();	 /* non-glyph and 'space' glyph */
	offsets_table[0] = 0;
	font_offset += 2 * (CELL_WIDTH * CELL_HEIGHT);
	
  for ( n = 1; n < 256; n++ )
  {
    /* generate glyphs for char value 0 through 255 */
    error = FT_Load_Char( face, n, FT_LOAD_RENDER );
    if ( error ) {
		offsets_table[n] = 0;
      continue;                 /* ignore errors */
	}

	/* track the size of the various glyphs */
	if (slot->bitmap.width > max_width) {
		max_width = slot->bitmap.width;
	}
	if (slot->bitmap.rows > max_height) {
		max_height = slot->bitmap.rows;
	}
	printf("Character 0x%x [%d, %d]\n", n, slot->bitmap.width, slot->bitmap.rows);
    /* now, draw to our target surface (convert position) */
	
	/* special case ' ' character since it shows up as a null glyph */
	if ((slot->bitmap.rows == 0) || (slot->bitmap.width == 0)) {
		if ((char) n == ' ') {
			printf("Space Character, width is %d, height is %d\n", 
						slot->bitmap.width, slot->bitmap.rows);
			offsets_table[n] = CELL_WIDTH * CELL_HEIGHT;
		} else {
			offsets_table[n] = 0;
		}
	} else {
    	draw_bitmap( &slot->bitmap,
                 slot->bitmap_left,
                 slot->bitmap_top, (unsigned char) n );
		offsets_table[n] = font_offset;
		font_offset += CELL_WIDTH * CELL_HEIGHT;
	}
  }

	fprintf(font_src, "};\n");
	fprintf(font_src, "\n");
	fprintf(font_src, "TERM_FONT named_font = {\n");
	fprintf(font_src, "\t%d,\t/* width */\n", CELL_WIDTH);
	fprintf(font_src, "\t%d,\t/* height */\n", CELL_HEIGHT);
	fprintf(font_src, "\t{\t/* offsets */\n");
	for (i = 0; i < 256; i += 8) {
		fprintf(font_src, "\t\t");
		for (j = 0; j < 8; j++) {
			fprintf(font_src, "%6d,", offsets_table[i+j]);
		}
		fprintf(font_src, "\n");
	}
	fprintf(font_src, "\t},\n");
	fprintf(font_src, "\t&__glyph_data[0][0][0]\n");
	fprintf(font_src, "};\n");
	fclose(font_src);

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

	printf("Max bitmap size [%d, %d]\n", max_width, max_height);
  return 0;
}

/* EOF */
