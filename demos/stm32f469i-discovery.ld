/* Linker script for ST STM32F469IDISCOVERY (STM32F469H6, 2048K flash, 184K RAM). */

/* Define memory regions. */
MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 184K /*SRAM1, SRAM2, SRAM3 */
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f4.ld

