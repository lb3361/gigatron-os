#include "main.h"
#include "ff.h"
#include "bank.h"
#include "loader.h"
#include "diskio.h"

/* Maximum number of files per directory */
#define MAXFILES 64

/* Blank using videotop */
#define BLANK_WITH_VIDEOTOP 1
/* Blank with mode 3 */
#define BLANK_WITH_MODE3 2


/** MEMORY USAGE
***    0x200:  Start routine
***    0x300:  Bank testing routines
***    0x8000: Page zero mirror (used while loading GT1 that target page zero)
***    0x8100: Junk page (data received from SPI while writing)
***    0x8200: 0xFF page (data sent to SPI while reading)
***    0x8300-0x84ff: Sector buffer for FS metadata
***    0x8500-0x86ff: Sector buffer for reading GT1 files
***    0x8700-0x87ff: Available
***    0x8800-0xfbff: Code, data and heap
***    0xfc00-0xffff: Stack only
***
*** MAP OVERLAY
***                size    addr   step    end   flags
*** segments = [ (0x7400, 0x8800, None,   None,   0),
***              (0x00fa, 0x0200, None,   None,   7),
***              (0x00fa, 0x0300, None,   None,   7) ]
**/

#define JUNK_PAGE  (void*)0x8100
#define FF_PAGE    (void*)0x8200
#define FS_BUFFER  (void*)0x8300
#define FIL_BUFFER (void*)0x8500


#if _GLCC_VER < 105041
# error This program requires a more recent version of GLCC.
#endif

char cbuf[256];
int nfiles;
char *files[MAXFILES];
FATFS fatfs;
jmp_buf jmpbuf;

void videoTopReset(void)
{
#if BLANK_WITH_VIDEOTOP
  videoTop_v5 = 0;
#elif BLANK_WITH_MODE3
  SYS_SetMode(BLANK_WITH_MODE3);
#endif
}

void videoTopBlank(void)
{
#if BLANK_WITH_VIDEOTOP
  videoTop_v5 = 220;
#elif BLANK_WITH_MODE3
  SYS_SetMode(3);
#endif
}

void faterr(int res)
{
  static char buf[16];
  char ibuf[8];
  strcpy(buf, "Fat error ");
  strcat(buf, itoa(res, ibuf, 10));
  maindialog(buf);
}

void *safe_malloc(size_t sz)
{
  void *p;
  if (! (p = malloc(sz)))
    _exitm(EXIT_FAILURE, "Out of memory");
  return p;
}

static void prline(int fgbg, int y, const char *s, int l)
{
  console_state.fgbg = fgbg;
  console_state.cx = 0;
  console_state.cy = y;
  if (s)
    console_print(s, l);
}

static int getbtn(void)
{
  register char ch = buttonState;
  static char fcnt;
  static char last;
  if (last != ch) {
    last = ch;
    fcnt = frameCount + 16;
    if (ch != 255)
      return ch;
  } else if (((frameCount - fcnt) & 0xfe) == 0) {
    /* Autorepeat up and down arrows */
    if (last == 0xfb || last == 0xf7) {
      fcnt = frameCount + 8;
      return last;
    }
  }
  return -1;
}



/* ------------ Directory reading ------------ */

static void freefiles(void)
{
  register int i;
  register char **f = files;
  for (i = 0; i != MAXFILES; i++)
    free(f[i]);
  memset(f, 0, sizeof(files));
  nfiles = 0;
}

static void dir(void)
{
  register int l;
  register int k = 0;
  register char **f = files;
  register char *fk = 0;
  register FRESULT res;
  DIR dir;
  FILINFO info;

  if (nfiles)
    freefiles();
  if ((res = f_opendir(&dir, cbuf)) != FR_OK)
    faterr(res);
  fk = "...";
  if (fk) {
    f[0] = safe_malloc(strlen(fk)+1);
    strcpy(f[0], fk);
    k++;
  }
  for(;;) {
    if (k > MAXFILES)
      break;
    if ((res = f_readdir(&dir, &info)) != FR_OK)
      faterr(res);
    if (! (l = strlen(info.fname)))
      break;
    f[k] = fk = safe_malloc(l + 2);
    *fk = '-';
    strcpy(fk + 1, info.fname);
    if (info.fattrib & AM_DIR)
      *fk = '/';
    else if (l - 4 > 0 && fk[l-3]=='.' && (fk[l-2]|0x20)=='g'
             && (fk[l-1]|0x20)=='t' && (fk[l]|0x20)=='1' )
      *fk = '*';
    k += 1;
  }
  f_closedir(&dir);
  nfiles = k;
}


/* ------------ SD0/SD1/ROM dialog ------------ */

#define CY1 4
#define CY2 6
#define CX1 9
#define CX2 17

static void rect(int fg, int bg)
{
  int i = console_info.offset[CY1] - 8;
  int j = console_info.offset[CY2+1] + 8;
  int k = CX1 * 6 - 4;
  int l = (CX2 - CX1) * 6 + 18;
  int c = fg;
  while(1) {
    char *addr = (char*)((videoTable[i])<<8) + k;
    char *eaddr = addr + l;
    if (i == j)
      c = fg;
    memset(addr, c, l);
    addr[0] = addr[l] = fg;
    c = bg;
    if (i == j)
      break;
    i += 2;
  }
}

void maindialog(const char *s)
{
  int i;
  int fgbg;
  int sel = 0;
  static char *lines[] = { " SD0:/    ", " SD1:/    ", " ROM menu "  };
  videoTopReset();
  if (s) {
    prline(0x0003, 0, cbuf, 4);
    console_print(" ", 1);
    console_print(s, 24);
    console_clear_to_eol();
  }
  for(;;) {
  redisplay:
    rect(0x3f, 0x15);
    for (i = 0; i < 3; i++) {
      console_state.fgbg = (i == sel) ? 0x003f : 0x3f15;
      console_state.cy = CY1 + i;
      console_state.cx = CX1;
      console_print(lines[i], CX2-CX1+2);
    }
  btn:
    switch(getbtn()) {
    case '\n':
    case buttonA ^ 0xff:
    case buttonRight ^ 0xff:
      if (sel == 2) {
        _console_reset(0x20);
        load_gt1_from_rom("Main");
      } else {
        freefiles();
        strcpy(cbuf,"SDx:/");
        cbuf[2] = '0' + sel;
      }
    case 0x1b:
    case buttonB ^ 0xff:
    case buttonLeft ^ 0xff:
      longjmp(jmpbuf, 1);
    case buttonUp ^ 0xff:
      if (sel > 0)
        sel -= 1;
      goto redisplay;
    case buttonDown ^ 0xff:
      if (sel < 2)
        sel += 1;
      goto redisplay;
    }
    goto btn;
  }
}

#undef CY1
#undef CY2
#undef CX1
#undef CX2


/* ------------ Browsing ------------ */


static void dispdir(int line)
{
  register char *s = cbuf+5;
  register int l = strlen(s);
  prline(CONSOLE_DEFAULT_FGBG, line, 0, 26);
  console_print(cbuf, 5);
  if (l < 21) {
    console_print(s, 21);
    console_clear_to_eol();
  } else {
    console_print("...", 3);
    console_print(s + l - 18, 18);
  }
}

typedef enum { A_NONE, A_DIR, A_PARENT, A_GT1 } action_t;

static action_t action(register const char *s)
{
  register int t = 0;
  register int l = strlen(s);
  register int r = strlen(cbuf);
  if (l + r - (int)sizeof(cbuf) >= 0)
    return A_NONE;

  if (s[0] == '.') {
    /* parent dir */
    s = strrchr(cbuf + 5, '/');
    if (s)
      *(char*)s = 0;
    else
      cbuf[5] = 0;
    return A_PARENT;
  }
  if (s[0] == '/') {
    /* this is a directory */
    if (cbuf[5] == 0)
      s += 1;
    strcpy(cbuf + r, s);
    return A_DIR;
  }
  if (s[0] == '*') {
    /* this is a gt1 file */
    if (cbuf[5])
      cbuf[r++] = '/';
    strcpy(cbuf + r, s + 1);
    return A_GT1;
  }
  return 0;
}

static void dispfile(register int i, register int sel, register const char *s)
{
  register int color;
  if (s[0] == '-')
    color = (sel) ? 0x153f : 0x2a20;
  else if (sel)
    color = 0x003f;
  else if (s[0] == '*')
    color = 0x3c20;
  else
    color = 0x3f20;
  prline(color, i + 1, s + 1, 26);
  if (s[0] == '/')
    console_print("/", 1);
  console_clear_to_eol();
  console_state.fgbg = CONSOLE_DEFAULT_FGBG;
}

static action_t browse(register int offset, register int selected)
{
  register int n, b, fresh1, fresh2;

 reload:
  videoTopBlank();
#if BLANK_WITH_VIDEOTOP
  dispdir(console_info.nlines -1);
#else
  console_clear_screen();
  dispdir(0);
#endif
  if (nfiles == 0)
    dir();
  fresh1 = 0;
  fresh2 = console_info.nlines - 1;
#if BLANK_WITH_VIDEOTOP
  console_clear_screen();
  dispdir(0);
#endif
  videoTopReset();
  for(;;)
    {
      if (fresh1 < fresh2) {
        for (n = fresh1; n != fresh2; n++)
          if (nfiles - n - offset > 0)
            dispfile(n, (offset + n == selected), files[offset + n]);
        fresh1 = fresh2 = 0;
      }
      n = console_info.nlines - 1;
      b = getbtn();
      if (!frameCount || !(b & 0x80))
        if (disk_ioctl(cbuf[2]-'0', DISK_CHANGED, 0))
          maindialog("Disk changed");
      switch(b)
        {
        case 0x1b:
        case buttonB ^ 0xff:
        case buttonLeft ^ 0xff:
          /**** Go to parent dir */
          return action("...");
        case 'Q':
          exit(0);
        case '\n':
        case buttonA ^ 0xff:
        case buttonRight ^ 0xff:
          /**** Execute selected entry */
          if (nfiles - selected > 0)
            switch (action(files[selected])) {
            case A_DIR:
              freefiles();
              if (browse(0, 0) != A_GT1) {
                freefiles();
                goto reload;
              }
            case A_GT1:
              return A_GT1;
            case A_PARENT:
              freefiles();
              return A_PARENT;
            }
          break;
        case buttonUp ^ 0xff:
          /**** Selection up */
          if (selected > 0) {
            selected -= 1;
            if (selected >= offset) {
              fresh1 = selected - offset;
            } else {
              console_clear_line(n);
              console_scroll(1, n + 1, -1);
              offset -= 1;
              fresh1 = 0;
            }
            fresh2 = fresh1 + 2;
          }
          break;
        case buttonDown ^ 0xff:
          /**** Selection down */
          if (selected < nfiles - 1) {
            selected += 1;
            if (offset + n - selected > 0) {
              fresh1 = selected - offset - 1;
            } else {
              console_clear_line(1);
              console_scroll(1, n + 1, +1);
              offset += 1;
              fresh1 = n - 2;
            }
            fresh2 = fresh1 + 2;
          }
          break;
        }
    }
}


int main(void)
{
  int i;
  int autoexec;
  FRESULT res;
  const char *s;

  /* switch to bank 3 */
  if (! has_128k()) {
    console_state.fgbg = 0x300;
    _exitm(EXIT_FAILURE, "Not a 128K Gigatron!");
  } else {
    int curbank = ctrlBits_v5 & 0xc0;
    videoTopBlank();
    _memcpyext(0xc0 | (curbank>>2), (char*)0x8800u, (char*)0x8800u, 0x7800u);
    SYS_ExpanderControl(ctrlBits_v5 | 0xc0);
  }

  /* set restart buffer */
  autoexec = 1;
  if (! setjmp(jmpbuf))
    strcpy(cbuf,"SD0:/");
  else {
    videoTopBlank();
    console_state.fgbg = CONSOLE_DEFAULT_FGBG;
    console_clear_screen();
    autoexec = 0;
  }

  /* Initialize FF_PAGE */
  memset(FF_PAGE,0xff,256);

  /* mount card */
  f_mount(0, "0:", 0);
  f_mount(0, "1:", 0);
  fatfs.win = FS_BUFFER;
  res = f_mount(&fatfs, cbuf, 1);
  if (res != FR_OK)
    maindialog("Mount failed");

  /* search for autoexec.gt1 */
  if (autoexec && (buttonState & buttonB)) {
    dir();
    for (i = 0; i != nfiles; i++)
      if (! strcmp(files[i], "*autoexec.gt1"))
        if (action(files[i]) == A_GT1)
          goto exec;
  }

  /* otherwise, browse */
  if (browse(0, 0) != A_GT1)
    maindialog(0);

 exec:
  /* Prep screen */
  _console_reset(CONSOLE_DEFAULT_FGBG & 0xff);
  if ((s = strrchr(cbuf, '/')))
    s += 1;
  else
    s = cbuf;
#if BLANK_WITH_VIDEOTOP
  prline(CONSOLE_DEFAULT_FGBG, console_info.nlines-1, "\x82 ", 26);
#else
  prline(CONSOLE_DEFAULT_FGBG, 3, "\x82 ", 26);
#endif
  console_print(s, 24);

  /* Go */
  freefiles();
  if ((res = load_gt1_from_fs(cbuf, FIL_BUFFER)) == FR_INT_ERR)
    _exitm(EXIT_FAILURE, "Corrupted GT1");
  if (res != FR_OK)
    faterr(res);
  return 0;
}
