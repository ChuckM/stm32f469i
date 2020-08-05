#

OBJS = ../util/lcd.o ../util/hexdump.o ../util/console.o \
	   ../util/clock.o ../util/sdram.o ../util/retarget.o ../util/sbrk.o

BINARY = dma2d

DEVICE = STM32F469HI

#LDSCRIPT = ../stm32f469i-discovery.ld
#LDSCRIPT = ./stm32f4-disco.ld

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

