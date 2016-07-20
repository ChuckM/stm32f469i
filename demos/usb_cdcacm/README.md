# README

This is the same example as the one from the libopencm3 examples 
repository, made to work on the 4x9 (32F469/32F479) series chips
from ST Micro. When I got the example working I realized that the
include file for USB OTG was created for a different chip, and in
particular bit 21 and 16 of the global configuration register really
needed to be set, these are defined as VBus detection enable (VBDEN)
and Power down control (PWRDWN) in the reference manual. 

```
     OTG_FS_GCCFG = (1 << 21) | (1 << 16);
     OTG_FS_DCTL = 0;
```

Other than that change and choosing the correct GPIOs for this board,
that was the only change. At some point there are many fixes that should
be included into the LOC3 USB stack but for what ever reason, they are
not being merged.

This example implements a USB CDC-ACM device (aka Virtual Serial Port)
to demonstrate the use of the USB device stack.

## Board connections

| Port  | Function       | Description                               |
| ----- | -------------- | ----------------------------------------- |
| `CN5` | `(USB_OTG_FS)` | USB acting as device, connect to computer |
