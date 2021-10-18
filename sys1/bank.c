#include <stdlib.h>
#include <gigatron/sys.h>

#include "bank.h"

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

int set_zbank(int ok)
{
  if (! has_zbank())
    return -1;
  if (!(ctrlBits_v5 & 0x20) != !!ok)
    _change_zbank();
  return !(ctrlBits_v5 & 0x20);
}
