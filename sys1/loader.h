#ifndef LOADER_H
#define LOADER_H


extern FRESULT load_gt1(const char *s, void *buffer);

extern void _exec_pgm(void *ramaddr);

#endif
