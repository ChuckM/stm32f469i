#
#

OBJS = ../util/led.o ../util/console.o ../util/clock.o \
	   ../util/retarget.o ../util/hexdump.o 

BINARY = qspi

DEVICE = STM32F469HI

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx

include ../../Makefile.include

