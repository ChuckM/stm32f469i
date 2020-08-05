#

OBJS = ../util/clock.o ../util/console.o ../util/retarget.o \
		../util/i2c.o ../util/sbrk.o ../util/lcd.o ../util/sdram.o

BINARY = touch_poll

DEVICE = STM32F469HI

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

