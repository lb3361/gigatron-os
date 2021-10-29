#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <gigatron/console.h>
#include <gigatron/libc.h>
#include <gigatron/sys.h>

extern void videoTopReset(void);
extern void videoTopBlank(void);
extern void faterr(int);
extern void maindialog(const char*);

#endif
