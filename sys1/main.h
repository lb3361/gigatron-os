#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <gigatron/console.h>
#include <gigatron/libc.h>
#include <gigatron/sys.h>

extern jmp_buf jmpbuf;

extern void videoTopReset(void);
extern void faterr(int);
extern void videoTopReset(void);
extern void videoTopBlank(void);

#endif
