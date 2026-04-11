/*****************************************************************************

 qprintf.c - module to emulate a printf with a possible quiet (disable mode.)

 A global variable GifNoisyPrint controls the printing of this routine

*****************************************************************************/
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright (C) Eric S. Raymond <esr@thyrsus.com>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "gif_lib.h"

bool GifNoisyPrint = false;

/*****************************************************************************
 Same as fprintf to stderr but with optional print.
******************************************************************************/
void GifQprintf(char *Format, ...) {
	va_list ArgPtr;

	va_start(ArgPtr, Format);

	if (GifNoisyPrint) {
		char Line[128];
		(void)vsnprintf(Line, sizeof(Line), Format, ArgPtr);
		(void)fputs(Line, stderr);
	}

	va_end(ArgPtr);
}

void PrintGifError(int ErrorCode) {
	const char *Err = GifErrorString(ErrorCode);

	if (Err != NULL) {
		fprintf(stderr, "GIF-LIB error: %s.\n", Err);
	} else {
		fprintf(stderr, "GIF-LIB undefined error %d.\n", ErrorCode);
	}
}

/* end */
