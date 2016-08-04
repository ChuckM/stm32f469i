/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Now this is just the clock setup code from systick-blink as it is the
 * transferrable part.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

/* Common function descriptions */
#include "../util/clock.h"

/* milliseconds since boot */
static volatile uint32_t system_millis;
/* millisecond count down when sleeping */
static volatile uint32_t system_delay;

/* Called when systick fires */
void sys_tick_handler(void)
{
	system_millis++;
	if (system_delay != 0) {
		system_delay--;
	}
}

/* simple sleep for delay milliseconds */
void msleep(uint32_t delay)
{
	system_delay = delay;
	while (system_delay > 0);
}

/* Getter function for the current time */
uint32_t mtime(void)
{
	return system_millis;
}

#define FAST_CLOCK

/*
 * clock_setup(void)
 *
 * This function sets up both the base board clock rate
 * and a 1khz "system tick" count. The SYSTICK counter is
 * a standard feature of the Cortex-M series.
 */
void clock_setup(void)
{

#ifdef FAST_CLOCK
	/* Base board frequency, set to 180Mhz */
	/* PLL_M = 8, PLL_Q = 7, PLL_R = 2, PLL_N = 360, PLL_P 2 */
	/* HCLK no divide, PCLK2 div 2 (90Mhz), PCLK1 (div4) 45Mhz */
	/* LOC3 doesn't know about DSI or 180Mhz clock rates yet */
	rcc_osc_on(RCC_HSI);
	rcc_wait_for_osc_ready(RCC_HSI);
	rcc_set_sysclk_source(RCC_CFGR_SW_HSI);
	rcc_osc_on(RCC_HSE);
	rcc_wait_for_osc_ready(RCC_HSE);
	rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
	/* Slow AHB bus is 45Mhz */
	rcc_set_ppre1(RCC_CFGR_PPRE_DIV_4);
	/* Fast AHB bus is 90Mhz */
	rcc_set_ppre2(RCC_CFGR_PPRE_DIV_2);
	/* updated to include PLL R, note M is shared by all PLLs */
	/* VCO freq = 360Mhz, Sysclk is VCO/PLLP(2) so 180Mhz */
	rcc_set_main_pll_hse(8, 360, 2, 7, 2);
	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	flash_set_ws(FLASH_ACR_ICE | FLASH_ACR_DCE | FLASH_ACR_LATENCY_5WS);
	rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
	rcc_wait_for_sysclk_status(RCC_PLL);
	rcc_ahb_frequency = 180000000;
	rcc_apb1_frequency = 45000000;
	rcc_apb2_frequency = 90000000;
	rcc_osc_off(RCC_HSI);
#else
	/* Maximum supported speed by LOC3, no DSI support */
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
#endif

	/* clock rate / 168000 to get 1mS interrupt rate */
	systick_set_reload(168000);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();

	/* this done last */
	systick_interrupt_enable();
}
