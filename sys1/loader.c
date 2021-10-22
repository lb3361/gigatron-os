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




static BYTE *load_gt1_addr;
static UINT load_gt1_len;

static UINT load_gt1_stream(register const BYTE *p, register UINT n)
{
  register BYTE *addr = load_gt1_addr;
  register UINT len = load_gt1_len;
  if (n == 0)
    return len > 0;
  if ((int)addr < 0) {
    _memcpyext(0x70, addr, p, n);
  } else if ((int)addr - 255 <= 0) {
    memcpy(addr + 0x8000u, p, n);
  } else {
    memcpy(addr, p, n);
  }
  load_gt1_addr += n;
  load_gt1_len -= n;
  return n;
}

static BYTE load_gt1_channelmask;

static void prep(const char *buf)
{
  register int len;
  register int b0 = buf[0];
  register int b1 = buf[1];
  if (!(len = buf[2]))
    len = 256;
  load_gt1_len = len;
  load_gt1_addr = (BYTE*)((b0 << 8) + b1);
  if (b0 - 4 <= 0 && b0 - 2 >= 0) {
    if (b0 == 3)
      mainmenuptr = 0;
    if (b1 + len >= 0xfa) {
      if (b1 == 2)
        load_gt1_channelmask = 0;
      else
        load_gt1_channelmask &= 1;
    }
  }
}


FRESULT load_gt1(const char *s)
{
  register FIL *fp = 0;
  register FRESULT res;
  register int segments = 0;
  UINT br;
  char buf[4];
  void *execaddr = 0;

  /* blank screen for speed */
  videoTopBlank();
  
  /* prepare zero page mirror */
  memcpy((void*)0x8030, (void*)0x0030, 0x100-0x30);

  /* prepare globals */
  load_gt1_channelmask = 0x3;
  channelMask_v4 &= 0xfc;

  /* allocate buffer */
  fp = safe_malloc(sizeof(FIL));

  /* open file */
  if ((res = f_open(fp, s, FA_READ)) != FR_OK)
    goto error;
  
  /* parse */
  for(;;) {
    if ((res = f_read(fp, buf, 3, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (br < 3)
      goto error;
    if (buf[0] == 0 && segments > 0)
      break;
    segments++;
    prep(buf);
    if ((res = f_forward(fp, load_gt1_stream, load_gt1_len, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (load_gt1_len != 0)
      goto error;
  }

  /* execute */
  f_close(fp);
  free(fp);
  channelMask_v4 |= load_gt1_channelmask;
  videoTopReset();
  _exec_pgm((void*)((buf[1] << 8) + buf[2]));
  return FR_OK;

  /* error */
 error:
  channelMask_v4 |= 0x3;
  free(fp);
  return res;
}
  
