#include <stdlib.h>
#include <string.h>
#include <gigatron/console.h>
#include <gigatron/sys.h>

#ifndef NAME
# error "Must define string NAME"
#endif

#define _STR(s) #s
#define STR(s) _STR(s)

static const char *name = STR(NAME);

int main(void)
{
  char buf[8];
  register int i;
  register void *p = 0;
  
  while ((p = SYS_ReadRomDir(p, buf)))
    if (! strncmp(buf, name, 8))
      SYS_Exec(p, (void*)0x200); /* found it */

  /* scroll one line */
  for (i = 0; i != 120; i++)
    videoTable[i+i] = (i < 112) ? videoTable[i+i+16] : i-104;
  _console_clear((char*)0x800, 0x03, 8);
  _console_printchars(0x0003, (char*)0x800, "not found in this ROM.", 26);
  return 10; 
}
