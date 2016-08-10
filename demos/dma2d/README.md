DMA2D Demo
----------

This then is a simple demonstration of the use of the DMA2D
peripheral. While ST Micro calls it the "Chrom-Art(tm)" 
most computer people would call it a bit block transfer
(BitBlT) unit. It can be very handy and relive some
of the memory copying of displaying graphics.

The demo here is pretty simple, it puts up a copy of
the SysTick time (milliseconds since start) as a clock
display of simulated 7 segment displays. If you don't
type at the terminal it will cycle every 10 seconds
through four "modes"

### Mode 1

In this mode the code renders a new frame of the graphics
by first clearing the screen with `gfx_fillScreen` and
then drawing in the various digits and text. The big
time waster here is that screen fill. It calls `lcd_draw_pixel`
over 1.5M times.

### Mode 2

In mode 2 the screen fill has been replaced with a fairly tight
loop. This boosts the frame rate from 4 to 10 FPS approximately.
The speedup comes from accessing memory more efficiently. This
mode has a reddish background (to distinguish it from the mode
above).

### Mode 3

Mode 3 is the first mode to actually use the DMA2D peripheral.
It sets up a memory fill (register to memory mode) to fill the
background (in this case to a light green color). It is faster
still than the tight loop (although not that much faster).

### Mode 4

Mode 4 is where the DMA2D starts to shine. At the beginning of
the code a "graph paper" background is generated, and in mode
4 the DMA2D copies in the graph paper as the background. This
achieves a pleasant effect while not taking a hit in terms
of frame rate.

## Connections

Serial connection is 57,600 baud, 8N1. 

