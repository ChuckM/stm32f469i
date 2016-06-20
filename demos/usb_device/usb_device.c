/*
 * usb_device - A libopencm3 example for configuring USB_OTG FS as a serial device
 *
 * Copyright (c) 2016 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This example is based on the STM32F469-Dicovery hardware which has a USB OTG
 * circuit fitted on the board. Because it does not use an external PHY it is
 * constrained to fullspeed (rather than optionally high speed). The example
 * explicitly does not re-use the code from the Blacksphere project (Gareth's code)
 * in order to explain the register level calls of LOC3 rather than a particular
 * implementation of those.
 *
 * One other note, it is using the retarget code which is in ../util so the clock
 * speed, SysTick interrupt, and a "console" on the MBED compatible serial port
 * is already set up at 115,200 baud 8N1 format.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
/* #include <libopencm3/stm32/usb_otg.h> */
#include "usb_otg.h"

void setup_usb(void);
void init_usb(void);

/*
 * As a "device" we really only care about DP and DM being operated
 * by the USB device. Neither switching on VBUS nor ID are 
 * particularly useful.
 */
void setup_usb(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_OTGFS);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

}

/*
 * Here we initialize the device registers to set up the USB peripheral
 * as a USB device (as opposed to a host).
 */
void init_usb(void)
{
    OTG_GINTSTS(USB_FS) = OTG_GINTSTS_MMIS;

}

int main(void)
{
    printf("USB Device Demo\n");
    printf("Setting up CDC device\n");
    setup_usb();
    return 0;
}

