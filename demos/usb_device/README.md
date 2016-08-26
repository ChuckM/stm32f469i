USB Device Example
------------------

This is a bit different, the current USB support in
libopencm3 is in a state of transistion, I was looking
to get a 'by the registers' example done and have
created a single `usb_otg.h` file rather than the two
that are currently in the library. 

Its a work in progress and is not yet complete.

There is a working example of an updated USB OTG
include file in my branch of loc3, and now I'm 
thinking the following API for `usb_device_init`.

Initialize the GPIO pins, and the USB core.

Attach device specific functions to USB core and then
enable it. `usb_poll` would be called in `sys_tick`
on a 1Khz schedule.

