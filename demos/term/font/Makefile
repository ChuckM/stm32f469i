CFLAGS= -I/usr/include/freetype2
LIBS= -lm -lfreetype 

makefont: makefont.c
	gcc -Bstatic -m64 ${CFLAGS} -o $@ $< ${LIBS}
