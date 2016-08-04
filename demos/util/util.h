
/*
 * This include file describes the functions exported by clock.c
 */
#ifndef __UTIL_H
#define __UTIL_H

/*
 * Definitions for clock functions
 */
void msleep(uint32_t);
uint32_t mtime(void);
void clock_setup(void);


/*
 * Our simple console definitions
 */

typedef enum term_colors_enum {
	NONE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE
} TERM_COLOR;

char * console_color(TERM_COLOR c);
void console_color_enable(void);
void console_color_disable(void);
void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);
void console_setup(int baud);
void console_baud(int baud);
uint32_t console_getnumber(void);

/* this is for fun, if you type ^C to this example it will reset */
#define RESET_ON_CTRLC

typedef enum led_color_e {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
	ORANGE_LED
} LED_COLOR;


/* dump memory contents as hex bytes */
void hex_dump(uint32_t addr, uint8_t *data, unsigned int len);

void on_led(LED_COLOR c);
void off_led(LED_COLOR c);
void toggle_led(LED_COLOR c);
void led_init(void);

/* Defines and prototypes for the sdram code */

#define SDRAM_BASE_ADDRESS ((uint8_t *)(0xC0000000))
void sdram_init(void);

/* QSPI FLASH utility functions */
void qspi_init(void);
/* Read data from FLASH (0 - 16MB worth) */
int qspi_read_flash(uint32_t addr, uint8_t *buf, int len);
/* Write date to FLASH (0 - 16MB worth) */
int qspi_write_flash(uint32_t addr, uint8_t *buf, int len);
/* Erase a 4K block of FLASH */
void qspi_erase_block(uint32_t addr);
/* Map FLASH into the address space */
void qspi_map_flash(void);
/* Unmap FLASH from the address space */
void qspi_unmap_flash(void);

#define FLASH_BASE_ADDRESS ((uint8_t *)(0x90000000))

#endif /* generic header protector */
