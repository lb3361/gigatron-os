SHELL=/bin/sh
ROM=v6
GIGAROM=../../gigatron-rom
CC=glcc
CFLAGS=
LDFLAGS=-map=64k,./system.ovl

PGMS=system7.gt1 system.gt1

SRCS=main.c diskio.c ff.c bank.c console.c loader.c
ASMS=spi.s bankasm.s loaderasm.s

all: ${PGMS}

clean: FORCE
	-rm ${PGMS} 2>/dev/null

${PGMS}: ff.h ffconf.h diskio.h

system7.gt1: ${SRCS} ${ASMS} system.ovl
	${CC} -rom=dev7 ${CFLAGS} ${LDFLAGS} ${SRCS} ${ASMS} -o $@
	cp $@ binaries/

system.gt1: ${SRCS} ${ASMS} system.ovl
	${CC} -rom=v6-- ${CFLAGS} ${LDFLAGS} ${SRCS} ${ASMS} -o $@
	cp $@ binaries/


FORCE:

.PHONY: FORCE

### Dependencies (gcc -D_GLCC_VER=104000 -MM *.c)

bank.o: bank.c bank.h
console.o: console.c main.h loader.h
dialog.o: dialog.c
diskio.o: diskio.c ff.h ffconf.h diskio.h spi.h
ff.o: ff.c ff.h ffconf.h diskio.h
loader.o: loader.c main.h ff.h ffconf.h loader.h
main.o: main.c main.h ff.h ffconf.h bank.h loader.h diskio.h

