Microphone Usage Example
------------------------

This example samples data from the MEMS microphones and tries
to make sense of it.

So mapping out the pins and their functions here, this is for the
MEMS setup on the board:

PB3 -> (AF6) I2S #3, CK
PD13 -> (AF2) TIM4 Channel 2 (connected to Mic clock)
PD12 -> (AF2) TIM4 Channel 1 (connected to PB3) (disables SWO)
PD6 -> (AF5) I2S #3, SD

