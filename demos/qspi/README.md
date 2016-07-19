Quad SPI Example
----------------

Ok, this was a crazy demo to get working. There are a lot
of moving parts. In particular, there is the Quad SPI peripheral
which has a bunch of modes, and there is the Micron FLASH chip
which is attached to it, which has its own stuff going on.

One of the things that keeps hanging me up (or causing unexpected
behavior) is that the Quad SPI interface can send out three
things, an "instruction" (8 bits), an "address" (1, 2, 3, or 4
bytes), some "alternate bytes" (generally 1, 2, 3 or 4 alternate bytes),
and a bunch of "data bytes". All of those things can be sent
(or received) with 1, 2, or 4 lines. 

Using 1 line for instruction and one for data  makes this a
nominally quite useful "normal" SPI peripheral. D0 is the equivalent
of MOSI and D1 the equivalent of MISO. Further, the nCS line can be
programmed to operate quite reasonably in terms of going active prior
to the start of the serial clock, and then going inactive after the
transaction is complete. That is something you cannot program on the
normal SPI ports of the ST line. It then adds the ability to use two
lines (D0/D1) or four lines (D0/D1/D2/D3) as bidirectional serial lines
like I2C ports do. To complete the picture, the peripheral's state
machine logic can break the transaction into multiple phases, each of
which may  be optional and may occur using different sets of lines.

That is a lot of flexibility which this demo doesn't even begin to scratch
but it does get complex around some of these features. For example, when
the program is writing to the FLASH chip (which it can do in 256 byte
chunks), it first sends a "write enable" command, that command has no
data or address, and the command is sent on D0 only. In the code I verify
that the write enable latch is set (it is automatically reset after an
actual write, and will not set at all if there was a problem setting it).
Further the flash chip doesn't indicate an error if it fails to set, it
just ignores the write. Anyway, once it is set do the program sub-sector
command. A sub-sector is a 4096 byte space in the flash. There are 4096
of them and each has 4096 bytes (16MB total). The command works by taking
any 24 bit address you send it and using the upper 12 bits as the sub-sector
number and ignores the lower 12 bits. So erasing address 0 and address 0x7ff
both end up erasing sub-sector 0 which covers address 0x000 to address 0xfff.

As I was learning as I went along, this demo prints out a lot of various
diagnostic bits. In particular the state of the `QUADSPI_SR` register from
time to time, and the internal `STATUS` register from the flash chip. It
is this latter register which indicates a write is in progress even after
the command is returned as "complete" as far as the SPI peripheral is
concerned. 

The general flow is as follows:

  * Read the ID register -- this is an easy first test of the 
    basic SPI function.
  * Create a buffer of data (256 bytes) -- the trick is to make it
    data I would recognize when I saw it again.
  * Reading a chunk of bytes from the flash from a known address -- as
    that is the first time it is read, it could have anything in it.
  * Issuing the erase subsector command at that address -- if that is
    successful then the flash would be programmed to all 0xFF bytes.
  * Re-read our test address -- this verifies that now the same address
    that we had read before, now has all 0xFF bytes.
  * Write our test data into this address -- if successful it changes
    the flash to match our test data.
  * Re-read our test address -- this verifies that our test data has
    shown up there.

With that code, all the basics for reading and writing the flash using
the Quad SPI port are known to work. The demo then goes into a simple
loop where you can type the following:

  * **-** or **_** to *move back* 256 bytes in the flash, read and display it.
  * **+** or **=** to *move forward* 256 bytes in the flash, read and display it.
  * **m** to *mark chunk* using a unique data set
  * **e** to *erase subsector* which will erase the current subsector

As with the other demos, since this uses the console code in the utility directory
you can always type ^C to restart the code from the beginning.

