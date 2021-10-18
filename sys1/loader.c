#include <stdlib.h>
#include <string.h>
#include <gigatron/console.h>
#include <gigatron/libc.h>
#include <gigatron/sys.h>

#include "ff.h"
#include "loader.h"

extern void faterr(FRESULT);
extern void *safe_malloc(size_t);
extern void *mainmenuptr;
extern void videoTopReset(void);
extern void videoTopBlank(void);


static void exec_pgm(void *ramptr)
{
  extern char SYS_ExpanderControl_v4_40;

  static char code[] = { 0x59, 0x7c,   /* LDI 0x7c */
                         0xb4, 0xfa,   /* SYS 40   */
                         0x63,         /* POP      */
                         0xff };       /* RET      */

  videoTopReset();
  memcpy((void*)0xf0, code, sizeof(code));
  vSP = 0xfc;
  sysFn = (unsigned int)&SYS_ExpanderControl_v4_40;
  *(void**)0xfe = (void*)0x1f0;
  *(void**)0xfc = ramptr;
  ((void(*)(void))0xf0)();
}



static BYTE *addr = 0;
static UINT len = 0;

static UINT load_gt1_stream(const BYTE *p, UINT n)
{
  if (n == 0)
    return len > 0;
  if (n > len)
    n = len;
  _memcpyext(0x70, addr, p, n);
  addr += n;
  len -= n;
  return n;
}


FRESULT load_gt1(const char *s, int exec)
{
  register FIL *fp = 0;
  register FRESULT res;
  UINT br;
  char buf[4];
  char mask = channelMask_v4 | 0x3;
  char nozp = 0;
  void *execaddr = 0;

  videoTopBlank();
  
  /* mask channels for pages 2 3 4 */
  channelMask_v4 &= 0xf8;

  /* alloctate buffer */
  fp = safe_malloc(sizeof(FIL));
  //memset(fp, 0, sizeof(fp));

  /* Open file */
  if ((res = f_open(fp, s, FA_READ)) != FR_OK)
    goto error;
  
  /* Parse GT1 */
  for(;;) {
    if ((res = f_read(fp, buf, 3, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (br < 3)
      goto error;
    if (nozp && buf[0] == 0)
      break;
    nozp = 1;
    addr = (BYTE*)((buf[0] << 8) + buf[1]);
    if (! (len = buf[2]))
      len = 256;
    /* writing to pages 1,2,3,4 */
    switch(buf[0]) {
    case 2:
      if (len + buf[1] >= 0xfa)
        mask &= 0xf8;
      break;
    case 3:
      mainmenuptr = 0;
    case 4:
      if (len + buf[1] >= 0xfa)
        mask &= 0xf9;
      break;
    }
    /* process the segment */
    if ((res = f_forward(fp, load_gt1_stream, len, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (len != 0)
      goto error;
  }
  /* All went well */
  f_close(fp);
  free(fp);
  execaddr = (void*)((buf[1] << 8) + buf[2]);
  channelMask_v4 = mask;
  if (exec && execaddr)
    exec_pgm(execaddr);
  return FR_OK;
  /* Error */
 error:
  channelMask_v4 = mask | 0x3;
  free(fp);
  return res;
}
  
