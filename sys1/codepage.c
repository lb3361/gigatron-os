/*
/ Copyright (C) 2021, lb3361, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
*/


#include "ff.h"

#if FF_USE_LFN	/* This module will be blanked if non-LFN configuration */

/* The gigatron only understands ascii.
   We call this code page 101. */

WCHAR ff_uni2oem (	/* Returns OEM code character, zero on error */
	DWORD	uni,	/* UTF-16 encoded character to be converted */
	WORD	cp      /* Code page for the conversion */
)
{
	WCHAR c = 0;
	if (uni < 0x80)
		c = (WCHAR)uni;
	return c;
}

WCHAR ff_oem2uni (	/* Returns Unicode character in UTF-16, zero on error */
	WCHAR	oem,	/* OEM code to be converted */
	WORD	cp		/* Code page for the conversion */
)
{
	WCHAR c = 0;
	if (oem < 0x80)
		c = oem;
	return c;
}


DWORD ff_wtoupper (	/* Returns up-converted code point */
	DWORD uni		/* Unicode code point to be up-converted */
)
{
	/* Just doing the ascii */
	if (uni >= 'a' && uni <= 'z')
		uni = uni + 'A' - 'a';
	return uni;
}

#endif /* #if FF_USE_LFN */

/* Local Variables: */
/* mode: c */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
