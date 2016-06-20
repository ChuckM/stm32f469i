
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

void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);
void console_setup(void);

/* this is for fun, if you type ^C to this example it will reset */
#define RESET_ON_CTRLC

#endif /* generic header protector */
