Graphics Demo
-------------

This demo creates some simple graphic images on the LCD
attached to the board, other demos are more sophisticated.

The relevant bit here is that this version of the ST Micro
F4 series sports a MIPI Compliant DSI interface. What that
means is that you can drive an 800 x 480 display with just
four pins on the chip. That is pretty cool, but in order
to use it you have to first configure the LTDC to display
your screen, and then you have to program the DSI unit
to take that output and encode it as a MIPI stream and
send it over to the display. That makes screen setup more
complicated than one might hope.

Setup is needlessly complex, I used a small perl script
to extract the initialization instructions that are sent
to the LCD. That resulted in a pretty simple state machine
for initializing the LCD display through the DSI's "Generic"
packet interface. 

