Graphics Demo
-------------

This demo creates some simple graphic images on the LCD
attached to the board.

The interesting  bit here is that this version of the ST Micro
F4 series sports a MIPI Compliant DSI interface. What that
means is that you can drive an 800 x 480 display with just
four dedicated pins on the SOC chip. That is pretty cool,
and it frees up pins for other things. The down side is that
its pretty stupidly complicated.

To make the display work you first have to configure the DSI
Host. This sends things to the display on your behalf. Then
you have to configure the LTDC, internally the DSI is connected
to the LTDC through something called the DSI Wrapper. Once
the LTDC is configured you can configure the DSI Wrapper.

Now that the thing is plumbed, you can send set up codes
to the LCD using the DSI "generic" packet interface.
I used a small perl script (`extract-init-from-code.pl`)
which read through the code in the STM32Cube source for
the display, and pulled out the 181 different packets you
have to send (and exactly right) to get the display to
wake up. Apparently nobody has heard of serial eeproms for
configuration data between the controller IC and the display
**that is soldered too it.** 

## Operation

This demo will talk to you on the serial port the board
presents to the host computer. Serial details are 576008N1
for use with gnu screen or hyperterminal or putty or whatever.


