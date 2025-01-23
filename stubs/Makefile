SHELL=/bin/sh
GLCC=glcc
CFLAGS=-rom=v5a -map=32k,bare -lc

STUBS= SDCard.gt1 Loader.gt1 Snake.gt1 Racer.gt1 Mandelbrot.gt1		\
       Pictures.gt1 Credits.gt1 Tetronis.gt1 Bricks.gt1 TinyBASIC.gt1	\
       TicTac.gt1 WozMon.gt1 Apple1.gt1 MsBasic.gt1 Horizon.gt1		\
       CardBoot.gt1 MainMenu.gt1 Reset.gt1

all: ${STUBS}

broom: FORCE

clean: broom
	-rm ${STUBS}

FORCE:

.PHONY: FORCE

SDCard.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=SDCard stub.c

Loader.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Loader stub.c

Snake.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Snake stub.c

Racer.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Racer stub.c

Mandelbrot.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Mandelbrot stub.c

Pictures.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Pictures stub.c

Credits.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Credits stub.c

Tetronis.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Tetronis stub.c

Bricks.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Bricks stub.c

TinyBASIC.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=TinyBASIC stub.c

TicTac.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=TicTac stub.c

WozMon.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=WozMon stub.c

Apple1.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Apple1 stub.c

MsBasic.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=MSBASIC stub.c

Horizon.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Egg stub.c

CardBoot.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Boot stub.c

MainMenu.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Main stub.c

Reset.gt1: stub.c
	${GLCC} ${CFLAGS} -o $@ -DNAME=Reset stub.c

