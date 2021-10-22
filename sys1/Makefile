SHELL=/bin/sh
CC=glcc
CFLAGS=-rom=dev
LDFLAGS=-map=64k,./system.ovl
PGMS=system.gt1

OBJS=main.o diskio.o ff.o bank.o console.o loader.o
ASMS=spi.s bankasm.s loaderasm.s ffasm.s

all: ${PGMS}

clean: FORCE
	-rm ${PGMS} ${OBJS} 2>/dev/null

${OBJS}: ff.h ffconf.h diskio.h

system.gt1: ${OBJS} ${ASMS} system.ovl
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJS} ${ASMS} -o $@


FORCE:

.PHONY: FORCE
