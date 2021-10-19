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


static BYTE *addr = 0;
static UINT len = 0;

static UINT load_gt1_stream(register const BYTE *p, register UINT n)
{
  if (n == 0)
    return len > 0;
  if (n > len)
    n = len;
  if (*((char*)&addr + 1))
    _memcpyext(0x70, addr, p, n);
  else
    memcpy(addr + 0x8000u, p, n); /* Page zero mirrored in 0x8000 */
  addr += n;
  len -= n;
  return n;
}

FRESULT load_gt1(register const char *s, register int exec)
{
  register FIL *fp = 0;
  register FRESULT res;
  UINT br;
  char buf[4];
  register char mask = channelMask_v4 | 0x3;
  register char initial = 1;
  register void *execaddr = 0;

  videoTopBlank();
  memcpy((void*)0x8030, (void*)0x0030, 0x100-0x30);
  
  /* mask channels for pages 2 3 4 */
  channelMask_v4 &= 0xf8;

  /* allocate buffer */
  fp = safe_malloc(sizeof(FIL));

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
    if (buf[0] == 0 && !initial)
      break;
    initial = 0;
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
  videoTopReset();
  if (exec && execaddr)
    _exec_pgm(execaddr);
  return FR_OK;
  /* Error */
 error:
  channelMask_v4 = mask | 0x3;
  free(fp);
  return res;
}
  
