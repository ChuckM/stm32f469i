/*
 * Easier to use clock functions
 */
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "../util/util.h"
#include "../util/helpers.h"

/* Various bit streams not currently defined in rcc.h */
#define RCC_PLLCFGR_PLLQ_MASK			0xf
#define RCC_PLLCFGR_PLLSRC_MASK			1
#define RCC_PLLCFGR_PLLP_MASK			0x3
#define RCC_PLLCFGR_PLLN_MASK			0x1ff
#define RCC_PLLCFGR_PLLM_MASK			0x3f
#define RCC_PLLCFGR_PLL_MASK			0x0f037fff
#define RCC_CFGR_PPRE_MASK			0x7
#define RCC_CFGR_PPRE2_MASK			0x7
#define RCC_CFGR_PPRE1_MASK			0x7
#define RCC_CFGR_HPRE_MASK			0xf
#define RCC_CFGR_SW_MASK			0x3
#define HSI_PLLCFGR_PLL_MASK	0x0f037fff
#define RCC_PLLCFGR_PLLSRC_SHIFT		22


/* The internal clock frequency on the F4 chip */
#define HSI_FREQUENCY		16000000

struct pll_parameters _clock_parameters;


/*
 * Turn on the indicated clock and wait for it to become ready.
 */
static inline void
osc_on(enum rcc_osc osc)
{
	switch (osc) {
	case RCC_PLL:
		RCC_CR |= RCC_CR_PLLON;
		break;
	case RCC_HSE:
		RCC_CR |= RCC_CR_HSEON;
		break;
	case RCC_HSI:
		RCC_CR |= RCC_CR_HSION;
		break;
	case RCC_LSE:
		RCC_BDCR |= RCC_BDCR_LSEON;
		break;
	case RCC_LSI:
		RCC_CSR |= RCC_CSR_LSION;
	default:
		break;
	}

	/* Now wait for it to be ready */
	switch (osc) {
	case RCC_PLL:
		while ((RCC_CR & RCC_CR_PLLRDY) == 0);
		break;
	case RCC_HSE:
		while ((RCC_CR & RCC_CR_HSERDY) == 0);
		break;
	case RCC_HSI:
		while ((RCC_CR & RCC_CR_HSIRDY) == 0);
		break;
	case RCC_LSE:
		while ((RCC_BDCR & RCC_BDCR_LSERDY) == 0);
		break;
	case RCC_LSI:
		while ((RCC_CSR & RCC_CSR_LSIRDY) == 0);
	default:
		break;
	}
}

static inline void osc_off(enum rcc_osc osc)
{
	switch (osc) {
	case RCC_PLL:
		RCC_CR &= ~RCC_CR_PLLON;
		break;
	case RCC_HSE:
		RCC_CR &= ~RCC_CR_HSEON;
		break;
	case RCC_HSI:
		RCC_CR &= ~RCC_CR_HSION;
		break;
	case RCC_LSE:
		RCC_BDCR &= ~RCC_BDCR_LSEON;
		break;
	case RCC_LSI:
		RCC_CSR &= ~RCC_CSR_LSION;
	default:
		break;
	}
}

/*
 * Set the system clock to the indicated oscillator
 * and wait for it to become active.
 */
static inline void
set_sysclk(enum rcc_osc clk)
{
	uint32_t clk_bits = RCC_CFGR_SW_HSI;
	switch (clk) {
		case RCC_HSI:
			clk_bits = RCC_CFGR_SW_HSI;
			break;
		case RCC_HSE:
			clk_bits = RCC_CFGR_SW_HSE;
			break;
		case RCC_PLL:
			clk_bits = RCC_CFGR_SW_PLL;
			break;
		default:
			clk_bits = RCC_CFGR_SW_HSI;
			break;
	}
	RCC_CFGR = (RCC_CFGR & (~RCC_CFGR_SW_MASK)) |
			   (clk_bits & RCC_CFGR_SW_MASK);
	/* wait for the switch */
	while (((RCC_CFGR >> RCC_CFGR_SWS_SHIFT) & RCC_CFGR_SW_MASK) != clk_bits) ;
}

/* update the ppre2 divisor */
static inline void
set_ppre2(uint32_t ppre2)
{
	RCC_CFGR = (RCC_CFGR & (~(RCC_CFGR_PPRE_MASK << RCC_CFGR_PPRE2_SHIFT))) |
		   ((ppre2 & RCC_CFGR_PPRE_MASK) << RCC_CFGR_PPRE2_SHIFT);
}

/* update the ppre1 divisor */
static inline void
set_ppre1(uint32_t ppre1)
{
	RCC_CFGR = (RCC_CFGR & (~(RCC_CFGR_PPRE_MASK << RCC_CFGR_PPRE1_SHIFT))) |
		   ((ppre1 & RCC_CFGR_PPRE_MASK) << RCC_CFGR_PPRE1_SHIFT);
}

/* update the hpre divisor */
static inline void
set_hpre(uint32_t hpre)
{
	RCC_CFGR = (RCC_CFGR & (~(RCC_CFGR_HPRE_MASK << RCC_CFGR_HPRE_SHIFT))) |
		   ((hpre & RCC_CFGR_HPRE_MASK) << RCC_CFGR_HPRE_SHIFT);
}

/*
 * Simple helper to compute backwards from the PLL setting to the value
 * it will generate if turned on as PLL Clock.
 */
static
uint32_t get_pll_frequency(uint32_t hse_frequency)
{
	uint32_t pll_freq;
	uint32_t pllreg = RCC_PLLCFGR;

	pll_freq = (pllreg & RCC_PLLCFGR_PLLSRC) ? hse_frequency : HSI_FREQUENCY;
	pll_freq = (pll_freq / ((pllreg >> RCC_PLLCFGR_PLLM_SHIFT) & RCC_PLLCFGR_PLLM_MASK)) *
				((pllreg >> RCC_PLLCFGR_PLLN_SHIFT) & RCC_PLLCFGR_PLLN_MASK);
	switch ((pllreg >> RCC_PLLCFGR_PLLP_SHIFT)  & RCC_PLLCFGR_PLLP_MASK) {
		default:
		case 0: pll_freq = pll_freq >> 1; 	/* (/2) */
			break;
		case 1: pll_freq = pll_freq >> 2;	/* (/4) */
			break;
		case 2: pll_freq = pll_freq / 6;	/* (/6) */
			break;
		case 3: pll_freq = pll_freq >> 3;	/* (/8) */
			break;
	}
	return pll_freq;
}

/*
 * These for the STM32F411RE, others nominally 84Mhz/42Mhz but they seem
 * to work.
 */
#define RCC_APB2_MAX_CLOCK		100000000
#define RCC_APB1_MAX_CLOCK		50000000

/* These are the constants used in the table in V8 of the F4 reference manual */
#define FLASH_WS_2V7			30000000
#define FLASH_WS_2V4			24000000
#define FLASH_WS_2V1			22000000
#define FLASH_WS_1V8			20000000

/*--------------------------------------------------------------------*/
/** @brief Configure the F4 HSI Clock
 *
 *	This function will set the chip to using the internal HSI clock
 * as its system clock source. This source is fixed at 16Mhz. It will
 * also set the APB1 and APB2 prescalers to 'none'.
 */
void
hsi_clock_setup(uint32_t frequency __attribute__((unused)))
{
	if ((RCC_CFGR & RCC_CFGR_SW_MASK) == RCC_CFGR_SW_HSI) {
		return; // nothing to do
	}

	/* First make sure we won't halt, switch to generic HSI mode */
	osc_on(RCC_HSI);

	/* Select HSI as SYSCLK source. Reset APB prescalers */
	set_sysclk(RCC_HSI);

	set_hpre(RCC_CFGR_HPRE_DIV_NONE);
	set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
	set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
	rcc_ahb_frequency = HSI_FREQUENCY;
	rcc_apb1_frequency = HSI_FREQUENCY;
	rcc_apb2_frequency = HSI_FREQUENCY;
	osc_off(RCC_HSE);
}

/**
 * @brief Configure the F4 HSE Clock
 *
 * @param[in] hse_frequency Frequency of the external clock in Hz
 *
 * This function configures the chip to run with the external
 * crystal as its system clock. It uses the frequency information
 * provided to set up the APB1 and APB2 clocks.
 */
void
hse_clock_setup(uint32_t hse_frequency)
{
	osc_on(RCC_HSE);
	set_sysclk(RCC_HSE);
	/* Select the crystal as our clock source */
	set_hpre(RCC_CFGR_HPRE_DIV_NONE);
	set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
	set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
	rcc_ahb_frequency = hse_frequency;
	rcc_apb1_frequency = hse_frequency;
	rcc_apb2_frequency = hse_frequency;
	osc_off(RCC_HSI);
}
	

/**
 * @brief Configure the F4 PLL Clock
 *
 * @param[in] clock_speed A combination of bits put together by the `PLL_CONFIG_BITS` macro
 * @param[in] input_freq The frequency of an external clock (if used)
 *
 * This function will configure the PLL to generate a specific output frequency (generally
 * higher than the input frequency) based on a formula in the data sheet. A typical configuration
 * might be `RCC_16MHZ_HSI_168MHZ_PLL` which would configure the PLL to use the internal
 * 16Mhz high speed clock and configure the PLL to clock the CPU at 168Mhz.
 *
 * In addtion to the core clock speed the function also sets the APB1 and APB2 peripheral clocks
 * to their maximum values. This involves changing prescalers to divide down the system clock
 * if it is running faster than the peripheral maximums (50Mhz and 100Mhz respectively).
 *
 * The clock setting code assumes >2.6V operation and assumes you are in
 * "normal" power mode (not power saving). If you are running in low power mode you
 * will need to increase FLASH wait states.
 */
void
pll_clock_setup(uint32_t pll_bits, uint32_t input_freq)
{
	uint32_t flash_ws;
	uint32_t apb_div;

	/* First make sure we won't halt, switch to generic HSI mode */
	hsi_clock_setup(HSI_FREQUENCY);

	/* At this point we're running on HSI at HSI_FREQUENCY clocks */
	
	/* Clear old bits, reset source to HSI, then OR in new bits */
	RCC_PLLCFGR = (RCC_PLLCFGR & ~(RCC_PLLCFGR_PLL_MASK | RCC_PLLCFGR_PLLSRC))
								 | pll_bits;

	/* Check to see if we're using the HSE 8Mhz defines */
	if (pll_bits & RCC_PLLCFGR_PLLSRC) {
		osc_on(RCC_HSE);
	}

	/* Enable PLL oscillator and wait for it to stabilize. */
	osc_on(RCC_PLL);

	/* figure out what frequency we are going to be set too */
	rcc_ahb_frequency = get_pll_frequency(
		(pll_bits & RCC_PLLCFGR_PLLSRC) ? input_freq : HSI_FREQUENCY);

	/* Set APB2 and APB1 to maximum frequency possible given this HCLK */
	/* NB: Assumes internal clock < 200Mhz, APB2 max 100Mhz */
	apb_div = (rcc_ahb_frequency + (RCC_APB2_MAX_CLOCK - 1)) / RCC_APB2_MAX_CLOCK;
	switch (apb_div) {
		case 1:
			set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
			/* rcc_apb2_frequency = freq; */
			rcc_apb2_frequency = rcc_ahb_frequency;
			break;
		case 2:
			set_ppre2(RCC_CFGR_PPRE_DIV_2);
			rcc_apb2_frequency = rcc_ahb_frequency >> 1;
			break;
		case 3:
			set_ppre2(RCC_CFGR_PPRE_DIV_4);
			rcc_apb2_frequency = rcc_ahb_frequency >> 2;
			break;
		default:
			set_ppre2(RCC_CFGR_PPRE_DIV_8);
			rcc_apb2_frequency = rcc_ahb_frequency >> 3;
			break;
	}

	/* This rounds up to the smallest safe divisor */
	apb_div = (rcc_ahb_frequency + (RCC_APB1_MAX_CLOCK - 1)) / RCC_APB1_MAX_CLOCK;
	switch (apb_div) {
		case 1:
			set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
			rcc_apb1_frequency = rcc_ahb_frequency;
			break;
		case 2:
			set_ppre1(RCC_CFGR_PPRE_DIV_2);
			rcc_apb1_frequency = rcc_ahb_frequency >> 1;
			break;
		case 3:	
		case 4:
			set_ppre1(RCC_CFGR_PPRE_DIV_4);
			rcc_apb1_frequency = rcc_ahb_frequency >> 2;
			break;
		default:
			set_ppre1(RCC_CFGR_PPRE_DIV_8);
			rcc_apb1_frequency = rcc_ahb_frequency >> 3;
			break;
	}

	/* Compute flash wait states (at > 2.6V) */
	flash_ws = ((rcc_ahb_frequency + (FLASH_WS_2V7 - 1)) / FLASH_WS_2V7) & 0x7;
	/* Configure flash settings. */
	flash_set_ws(FLASH_ACR_ICEN | FLASH_ACR_DCEN | flash_ws);

	/* Select PLL as SYSCLK source. */
	set_sysclk(RCC_PLL);
}

#define warn(n)

static const int pllp_candidates[4] = {2, 4, 6, 8};

static uint32_t compute_pll_bits(int desired_frequency, int input_frequency);

/*
 * This function takes a 'desired' frequency in Hz, and an 'input'
 * frequency in Hz, and computes values for the PLL multipliers on
 * the STM32F4. If 'input' is left at 0, the code assumes you are
 * using the HSI oscillator, if it is provided it assumes you are
 * using an external (HSE) oscillator.
 *
 * The output is a set of bits suitable for feeding into the rcc_pll_clock_setup().
 */
static uint32_t
compute_pll_bits(int desired_frequency, int input_frequency)
{
	uint32_t pllr, pllm, plln, pllq, pllp, vco;
	int i, src;

	src = 1;
	if (input_frequency == 0) {
		src = 0;
		input_frequency = HSI_FREQUENCY;
	}
	pllp = 0;
	for (i = 0; (i < 4) && (pllp == 0); i++) {
		vco = desired_frequency * pllp_candidates[i];
		/* if VCO lands in the "good" range its a winner */
		if ((vco >= 192000000) && (vco <= 433000000)) {
			pllp = pllp_candidates[i];
		}
	}
	if (pllp == 0) {
		return 0; /* can't make that frequency */
	}
	vco = desired_frequency * pllp;
	/*
	 * Compute divisor for closest to 2,000,000 Mhz without going over
	 * later, if this version of pllm doesn't work, we can increment it
	 * by 1 until the division would cause the input frequency to the
	 * PLL to drop below 1MHz.
	 */
	pllm = (input_frequency + 1999999) / 2000000;
	plln = (vco * pllm) / input_frequency;

	/* now check to see if plln is an integral multiplier */
	if (((input_frequency * plln)/pllm) != vco) {
		plln = 0;
		/* if not, try other versions of pllm that might get us there */
		while (((input_frequency / pllm) >= 1000000)  && (plln == 0)) {
			uint32_t tplln;
			tplln = (vco * pllm) / input_frequency;
			if ((input_frequency * tplln) / pllm == vco) {
				/* found one! */
				plln = tplln;
				break;
			}
			pllm++;
		}
		/* no value of PLLM works either, so we're stuck */
		if (plln == 0) {
			return 0; /* Can't get there from here */
		}
        }
	pllq = vco / 48000000;
	if ((pllq * 48000000) != vco) {
		/* not sure what the right answer here is, the chip will work but USB won't */
		warn("won't work for USB\n");
	}

	pllr = vco / 60000000;
	_clock_parameters.pllr = pllr;
	_clock_parameters.pllm = pllm;
	_clock_parameters.plln = plln;
	_clock_parameters.pllp = pllp;
	_clock_parameters.pllq = pllq;
	_clock_parameters.pllp_real = ((pllp / 2) - 1);
	_clock_parameters.src = src;
	return (PLL_CONFIG_BITS(pllr, ((pllp / 2) - 1), plln, pllq, pllm, src));
}

/* Set up a timer to create 1mS ticks. */
static void
systick_setup(int tick_rate)
{
    /* clock rate / 1000 to get 1mS interrupt rate */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload((rcc_ahb_frequency) / tick_rate);
	systick_clear();
	systick_counter_enable();
    /* this done last */
    systick_interrupt_enable();
}

/*
 * And this is the utility function, set up the clock for the
 * desired frequency and enable the SysTick counter to give
 * 1mS system "ticks" (1000 Hz).
 */
uint32_t
clock_setup(uint32_t desired_frequency, uint32_t hse_frequency) {
	if (desired_frequency == hse_frequency) {
		hse_clock_setup(desired_frequency);
	} else if ((hse_frequency == 0) && (desired_frequency == HSI_FREQUENCY)) {
		hsi_clock_setup(HSI_FREQUENCY);
	} else {
		uint32_t cfgr_bits;
		cfgr_bits = compute_pll_bits(desired_frequency, hse_frequency);
		/* debug test XXX: FIXME!*/
		cfgr_bits = 0x66405a08;
		pll_clock_setup(cfgr_bits, hse_frequency);
	}
	systick_setup(1000);
	return rcc_ahb_frequency;
}

/*
 * monotonically increasing number of milliseconds from reset
 * overflows every 49 days if you're wondering
 */
static volatile uint32_t system_millis;
static volatile uint32_t delay_millis;

/* Called when systick fires */
void
sys_tick_handler(void) {
    system_millis++;
	/* simple countdown timer */
	gpio_toggle(GPIOB, GPIO15);
	if (delay_millis) {
		delay_millis--;
	}
}

/* sleep for delay milliseconds */
void
msleep(uint32_t delay)
{
	delay_millis = delay;
    while (delay_millis) ;
}

/* return the time */
uint32_t
mtime()
{
    return system_millis;
}

/*
 * time_string(uint32_t)
 *
 * Convert a number representing milliseconds into a 'time' string
 * of HHH:MM:SS.mmm where HHH is hours, MM is minutes, SS is seconds
 * and .mmm is fractions of a second.
 *
 * Uses a static buffer (not multi-thread friendly)
 */
unsigned char *
time_string(uint32_t t)
{
    static unsigned char time_string[14];
    uint16_t msecs = t % 1000;
    uint8_t secs = (t / 1000) % 60;
    uint8_t mins = (t / 60000) % 60;
    uint16_t hrs = (t /3600000);

    // HH:MM:SS.mmm\0
    // 0123456789abc
    time_string[0] = (hrs / 100) % 10 + '0';
    time_string[1] = (hrs / 10) % 10 + '0';
    time_string[2] = hrs % 10 + '0';
    time_string[3] = ':';
    time_string[4] = (mins / 10)  % 10 + '0';
    time_string[5] = mins % 10 + '0';
    time_string[6] = ':';
    time_string[7] = (secs / 10)  % 10 + '0';
    time_string[8] = secs % 10 + '0';
    time_string[9] = '.';
    time_string[10] = (msecs / 100) % 10 + '0';
    time_string[11] = (msecs / 10) % 10 + '0';
    time_string[12] = msecs % 10 + '0';
    time_string[13] = 0;
    return &time_string[0];
}

struct pll_parameters *
dump_clock(void)
{
	static struct pll_parameters p;

	uint32_t reg = RCC_PLLCFGR;
	p.pllr = RCC_GET(PLLCFGR, PLLR, reg);
	p.pllm = RCC_GET(PLLCFGR, PLLM, reg);
	p.plln = RCC_GET(PLLCFGR, PLLN, reg);
	p.pllp_real = RCC_GET(PLLCFGR, PLLP, reg);
	switch (p.pllp_real) {
		default:
			p.pllp = -1;
			break;
		case 0:
			p.pllp = 2;
			break;
		case 1:
			p.pllp = 4;
			break;
		case 2:
			p.pllp = 6;
			break;
		case 3:
			p.pllp = 8;
			break;
	}
	p.pllq = RCC_GET(PLLCFGR, PLLQ, reg);
	p.src = ((reg & RCC_PLLCFGR_PLLSRC) != 0) ? 1 : 0;
	printf("PLL Parameters: R:%d, P:%d(%d), M:%d, Q:%d, N:%d, SRC=%s\n",
			p.pllr, p.pllp, p.pllp_real, p.pllm, p.pllq, p.plln,
			(p.src == 1) ? "HSE" : "HSI");
	return &p;
}

