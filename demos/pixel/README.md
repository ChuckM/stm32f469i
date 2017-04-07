Pixel Experiements
------------------

This isn't really a demo, this code is some experimental code that
is looking at coming up with a fairly clean way to represent the
various types of pixels that the "Chrom-ART" (DMA2D) device can 
work with. The idea being that you should be able to create a
bitmap in that is rendered into with the graphics api and then
blitted around the screen with the DMA2D device.

Not all pixel types can be represented by the LTDC device some are
used as "alpha" only or palletted versions of the same thing.

For example, to make an 8 bit palleted display you would use the L8
type and the populate the DMA2D's color lookup table (CLUT) with
256 different color values. To 'display' that you would copy the
8 bit buffer into the 'display buffer' which would translate it on
the fly and store the colors as either RGB888 or RGB565 into the
display buffer. Which the LTDC could then display on the screen.
