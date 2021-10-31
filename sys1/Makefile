SHELL=/bin/sh
GIGAROM=../../gigatron-rom
CC=glcc
CFLAGS=-rom=dev
LDFLAGS=-map=64k,./system.ovl
PGMS=system.gt1

OBJS=main.o diskio.o ff.o bank.o console.o loader.o
ASMS=spi.s bankasm.s loaderasm.s

all: ${PGMS}

clean: FORCE
	-rm ${PGMS} ${OBJS} 2>/dev/null

${OBJS}: ff.h ffconf.h diskio.h

system.gt1: ${OBJS} ${ASMS} system.ovl
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJS} ${ASMS} -o $@
	cp $@ binaries/

patch: system.gt1
	cp ${PGMS} binaries/
	cp ${GIGAROM}/dev.rom binaries

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

