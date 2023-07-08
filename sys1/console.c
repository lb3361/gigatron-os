#include "main.h"


/* 1) CONSOLE GEOMETRY
   Override console_info and _console_reset to change the console
   geometry. We want to preserve the top two lines of the screen and
   include spacing to leave room for drawing three horizontal lines
   at y coordinate 17, 28, 119 */

static char *lines[] =
  { screenMemory[34/2], screenMemory[56/2], screenMemory[238/2] };

const struct console_info_s console_info =
  { 12, 26,
    { 38, 60, 76, 92, 108, 124,
      140, 156, 172, 188, 204, 220 } };

void _console_reset(int fgbg)
{
  char i;
  char fg; 
  int *table = (int*)videoTable;
  if (fgbg >= 0)
    _console_clear(screenMemory[16], fgbg, 104);
  for (i=8; i!=128; i++)
    *table++ = i;
  /* Draws the lines if fg != 0 */
  if (fg = (fgbg >> 8)) {
    for (i=0; i!=3; i++)
      memset(lines[i], fg, 160);
  }
}


/* 2) PROGRAM EXIT
   Define exitm_msgfunc to be called from _exitm. Display the error
   message on the first line and wait 1.5 seconds. Then try to call
   the main menu program from rom. */

static void exitm_msgfunc(int retcode, const char *s)
{
  videoTopReset();
  if (retcode && s) {
    console_state.fgbg = 3;
    console_state_set_cycx(12);
    console_state_set_wrap(0x101);
    console_print(s, 26);
    console_clear_to_eol();
  }
}


/* 3) CONSOLE INITIALIZATION
   Override the _console_setup initialization function. Set the
   exitm_msgfunc function.  Set a console state that does not wrap or
   scroll lines. Clear the screen with console_reset. */

void _console_setup(void)
{
  console_state_set_wrap(0);
  _exitm_msgfunc = exitm_msgfunc;
  _console_reset(CONSOLE_DEFAULT_FGBG);
}
