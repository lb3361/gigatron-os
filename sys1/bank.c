#include <stdlib.h>
#include <gigatron/sys.h>

#include "bank.h"

#if _GLCC_VER < 104010
# error This program requires a more recent version of GLCC.
#endif


int has_128k(void)
{
  static int v = -1;
  if (v < 0)
    v = _banktest((char*)0x806fu, 0xc0) && _banktest((char*)0x806fu, 0x80);
  return v;
}

int has_zbank(void)
{
  static int v = -1;
  if (v < 0)
    v = _banktest((char*)0xffu, 0x20);
  return v;
}

