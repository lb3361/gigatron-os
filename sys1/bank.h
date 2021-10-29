#ifndef BANKTEST_H
#define BANKTEST_H

/* from bank.c */
extern int has_128k(void);
extern int has_zbank(void);
extern int set_zbank(int);

/* from bankasm.s */
extern int _banktest(char *addr, char bitmask);
extern int _change_zbank(void);

#endif
