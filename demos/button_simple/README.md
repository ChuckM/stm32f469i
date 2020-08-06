Simple Button Example
---------------------

Simple button is similar to minimal blink, although in this case I take
advantage of the retarget module (see the util directory README).

The "wakeup" button (it is blue in color)  on the STM32F469I-DISCO is
connected to GPIO pin `PA0`. This code sets up the GPIO pin as an input
and waits for it to go to the 'active' state (button pressed), it then
updates its `press_count` and using that cycles through turning on each
LED in turn. It then waits for the button to go 'inactive' before resuming.

Of course failing to wait for it to go inactive means you will rip through
the loop as fast as the code can go (it's sending text out to the serial
port at 57600 baud so that limits the overall length of the loop.)

**Normally** this technique of looping on activation and then deactivation
would not be recommended because the button can make several intermittent
activations or inactivations as it changes state because the button contacts
are physically just two conductors separated by a spring. However, on this
board ST Micro has put a capacitor and resistor across the button to "damp
out" those spurious state changes. This "debounces" the button. Without
that RC network you would end up with spurious button presses so you would
have to debounce the button in software. 
