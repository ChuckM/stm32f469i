MEMS Demo
----------

**under construction**

Demonstrate the use of the MEMS microphones 
on the board. If it is working you will see the audio in a sort of
oscilloscope type display on the screen.

The microphones are of type MP34DT01TR and two are connected using 'PDM'
mode. U2 and U6 are the default microphones connected to DFSDM peripheral
in the STM32F469. The PDM Clock signal on PD13 and PDM Data signal on PD6. 
(SPI3? That is on the hardware block diagram)

The code sets up a serial stream to just constantly sample the microphones
and another stream to create a video display based on the values sampled.

