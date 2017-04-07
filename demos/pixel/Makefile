OBJS = ../util/hexdump.o ../util/console.o ../util/clock.o ../util/sdram.o ../util/retarget.o

BINARY = pixel

LDSCRIPT = ../stm32f469i-discovery.ld

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

