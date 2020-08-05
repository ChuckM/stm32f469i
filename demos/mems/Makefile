#

OBJS = mems.o signal.o reticle.o ../util/dma2d.o ../util/sbrk.o \
        ../util/lcd.o ../util/hexdump.o ../util/console.o ../util/clock.o \
        ../util/sdram.o ../util/retarget.o

BINARY = main

DEVICE = STM32F469NI

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

