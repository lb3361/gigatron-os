#include "main.h"
#include "ff.h"
#include "loader.h"

extern void _exec_pgm(void*);
extern void _exec_rom(void*);


#if USE_C_LOAD_GT1_STREAM

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
  load_gt1_addr = addr = addr + n;
  load_gt1_len = len = len - n;
  return n;
}

#else

/* Implemented in loadasm.s */
extern BYTE *load_gt1_addr;
extern UINT load_gt1_len;
extern UINT load_gt1_stream(register const BYTE *p, register UINT n);

#endif


static BYTE load_gt1_channelmask;

static int prep(const char *buf)
{
  register BYTE *pcm;
  register int len;
  register int b0 = buf[0];
  register int b1 = buf[1];
  len = ((buf[2] - 1) & 0xff) + 1;
  load_gt1_len = len;
  load_gt1_addr = (BYTE*)((b0 << 8) + b1);
  /* illegal page crossing indicates a corrupted gt1 */
  if ((b1 = b1 + len - 1) >> 8)
    return 1;
  /* check channel mask
     writing 1fe-1ff, 2fe-2ff -> channelmask &= 0xfc
     writing 3fe-3ff, 4fe-4ff -> channelmask &= 0xfe 
     this is done *before* loading the data. */
  if (((b1 + 2) & 0xfe) || ((b0 - 1) & 0xfc))
    return 0;
  if (b0 - 2 >= 0)
    channelMask_v4 &= 0xfe;
  else
    channelMask_v4 &= 0xfc;
  return 0;
}

int load_gt1_from_fs(const char *s, void *sectorbuffer)
{
  FIL fil;
  register FIL *fp = &fil;
  register FRESULT res;
  register int segments = 0;
  UINT br;
  char buf[4];
  void *execaddr = 0;

  /* blank screen for speed */
  videoTopBlank();
  /* prepare zero page mirror */
  memcpy((void*)0x8030, (void*)0x0030, 0x100-0x30);
  /* open file */
  fp->buf = sectorbuffer;
  if ((res = f_open(fp, s, FA_READ)) != FR_OK)
    goto error;
  /* parse */
  channelMask_v4 |= 3;
  for(;;) {
    if ((res = f_read(fp, buf, 3, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (br < 3)
      goto error;
    if (buf[0] == 0 && segments > 0)
      break;
    segments++;
    if (prep(buf))
      goto error;
    if ((res = f_forward(fp, load_gt1_stream, load_gt1_len, &br)) != FR_OK)
      goto error;
    res = FR_INT_ERR;
    if (load_gt1_len != 0)
      goto error;
  }
  /* test that we've reached EOF */
  if ((res = f_read(fp, buf, 1, &br)) != FR_OK || br != 0)
    goto error;
  /* execute */
  f_close(fp);
  videoTopReset();
  _exec_pgm((void*)((buf[1] << 8) + buf[2]));
  return FR_OK;
  /* error */
 error:
  channelMask_v4 |= 0x3;
  return res;
}
  

int load_gt1_from_rom(register const char *name)
{
  char buf[8];
  register void *p = 0;
  while ((p = SYS_ReadRomDir(p, buf)))
    if (! strncmp(buf, name, 8))
      _exec_rom(p);
  return -1;
}
