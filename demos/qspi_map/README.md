# QSPI Mapped FLASH

This demo complements the indirect mode demo by mapping
the flash. Rather than copy all of the QUADSPI set up
code it uses the code that is in the utility code. As
with other demos you interact with it by opening a terminal
up on the serial port that shows up when you plug in the
the discovery board. The communication parameters are 57,600
baud 8 bits, no parity, one stop bit (8N1). 

Like the other flash demo is uses the simple graphics to 
create a buffer that is recognizable in the memory dump.
It then programs that buffer into the FLASH using the
indirect API and calls `qspi_map_flash` which maps the
FLASH chip to addresses `0x900000 - 0x90ffffff`.  You
can scroll forward and back through the memory dump of
the flash by using the plus and minus keys (or their shifted
equivalents). 

A couple of other things to note. It does a quick speed test
where it reads all of the flash and sums it. On my board this
usually takes about 400MS (or .4 seconds). That equates to a
speed of 40MB/sec. The other is that the 'u' command will
unmap the flash, read it using the indirect API, and then
re-map it. You would do something like that if you needed to
erase parts or wanted to program parts. The 'f' command is
the "fail" version where it unmaps the flash and doesn't
re-map it. When the code tries to read memory where it used
to be map it generates a hard fault exception. The code
catches that exception and prints a helpful message, you can
press any key to restart the system.

