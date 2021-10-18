#include <stdlib.h>
#include <string.h>
#include <gigatron/console.h>
#include <gigatron/libc.h>
#include <gigatron/sys.h>

static void console_exitm_msgfunc(int retcode, const char *s)
{
  if (s) {
    static struct console_state_s rst = {3, 0, 0, 0, 0};
    videoTop_v5 = 0;
    console_state = rst;
    console_print(s, console_info.ncolumns);
    console_clear_to_eol();
  }
}

void _console_setup(void)
{
  static struct console_state_s rst = {CONSOLE_DEFAULT_FGBG, 0, 0, 0, 0};
  console_state = rst;
  console_clear_screen();
  _exitm_msgfunc = console_exitm_msgfunc;
}
