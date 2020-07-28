#
# Example makefile
#

OBJS = bold-font.o regular-font.o ../util/lcd.o ../util/hexdump.o \
		 ../util/console.o \
		../util/clock.o ../util/sdram.o ../util/retarget.o

BINARY = term

DEVICE = STM32F469HI

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

