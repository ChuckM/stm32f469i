Font Games
----------

So the tricky bit about this terminal thing is the font. On the
one hand I could just grab the EGA Font, or another 10 x 18
binary font, but it looks bad. That is because the display is
capable of 256 levels of color and so ideally you want an
anti-aliased font. It looks good even when it is tiny.

I started by using a paint program to render the font into a
bitmap (antialiased) and then another program (in perl) to
pull that apart into glyph "chunks". That really didn't work
out very well, and after wasting a week on it I decided to
just use FreeType2 to render the glyphs for me. The library
is a bit large to embed in the flash, so I'm generating a
set of glyphs.

The program `makefont.c` started life as `example1` from
the Freetype tutorial. After beating to my will, this
program generates a file font-src.c which and be massaged
into something useful in the main program (see `80x25-font.c`
in the parent directory). Nominally the font array is 256
glyphs large (8 bit character). Other glyph spots are filled
in by pulling from other fonts.

The fonts were fetched from the [Google Fonts][font] repository
on github.

[font]: https://github.com/google/fonts/
