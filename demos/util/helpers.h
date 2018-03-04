/*
 * helpers.h
 *
 * A lot of the include files in libopencm3 (the STM versions anyway) can be
 * regularized such that their multi-bit register fields can be defined by
 * two values:
 *	<SUBSYSTEM>_<REGISTER>_<FIELD>_SHIFT	this is the bit shift
 *	<SUBSYSTEM>_<REGISTER>_<FIELD>_MASK		this is the number of bits in 
 *											the field as a mask, so always
 *											(2^n-1) where 'n' is the number
 *											of bits in the field.
 * By sticking with this pattern we can create some macros that will reliably
 * set values, get valuse, or return a properly shifted mask to clear off the
 * values in an easier to read form:
 *	int = <SUBSYSTEM>_GET(<REGISTER>, <FIELD>);
 *	(void) <SUBSYSTEM>_SET(<REGISTER>, <FIELD>, value);
 *  uint32_t = <SUBSYSTEM>_MASK(<REGISTER>, <FIELD>);
 *
 */
#pragma once
/*
 * Helper macros to avoid copy pasta code
 * For multi-bit fields we can use these macros to create
 * a unified SET_<reg>_<field> version and a GET_<reg_<field> version.
 */
#define LTDC_GET(reg, field, x) \
    (((x) >> LTDC_ ## reg ## _ ## field ##_SHIFT) & LTDC_ ## reg ##_ ## field ##_MASK)

#define LTDC_SET(reg, field, x) \
    (((x) & LTDC_ ## reg ##_## field ##_MASK) << LTDC_## reg ##_## field ##_SHIFT)

#define LTDC_MASK(reg, field) \
    ((LTDC_ ## reg ##_## field ##_MASK) << LTDC_## reg ##_## field ##_SHIFT)

#define DSI_GET(reg, field, x) \
    (((x) >> DSI_ ## reg ## _ ## field ##_SHIFT) & DSI_ ## reg ##_ ## field ##_MASK)

#define DSI_SET(reg, field, x) \
    (((x) & DSI_ ## reg ##_## field ##_MASK) << DSI_## reg ##_## field ##_SHIFT)

#define DSI_MASK(reg, field) \
    ((DSI_ ## reg ##_## field ##_MASK) << DSI_## reg ##_## field ##_SHIFT)


#define DMA2D_GET(reg, field, x) \
    (((x) >> DMA2D_ ## reg ## _ ## field ##_SHIFT) & DMA2D_ ## reg ##_ ## field ##_MASK)

#define DMA2D_SET(reg, field, x) \
    (((x) & DMA2D_ ## reg ##_## field ##_MASK) << DMA2D_## reg ##_## field ##_SHIFT)

#define DMA2D_MASK(reg, field) \
    ((DMA2D_ ## reg ##_## field ##_MASK) << DMA2D_## reg ##_## field ##_SHIFT)

