struct term_font {
	int	w;
	int	h;
	uint32_t	glyphs[256];
	uint8_t	*glyph_data;
} current_font = {
	CHAR_WIDTH,
	CHAR_HEIGHT,
	{ 
		0x00 /* this needs to be populated */
	},
	&__font_data[0][0][0]
};
	
