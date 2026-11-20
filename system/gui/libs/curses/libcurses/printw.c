/*	$NetBSD: printw.c,v 1.29 2019/06/09 07:40:14 blymn Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <netbsd_sys/cdefs.h>

#include <stdarg.h>
#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

/*
 * printw and friends.
 */

/*
 * printw --
 *	Printf on the standard screen.
 */
int
printw(const char *fmt,...)
{
	va_list ap;
	int     ret;

	va_start(ap, fmt);
	ret = vw_printw(stdscr, fmt, ap);
	va_end(ap);
	return ret;
}
/*
 * wprintw --
 *	Printf on the given window.
 */
int
wprintw(WINDOW *win, const char *fmt,...)
{
	va_list ap;
	int     ret;

	va_start(ap, fmt);
	ret = vw_printw(win, fmt, ap);
	va_end(ap);
	return ret;
}
/*
 * mvprintw, mvwprintw --
 *	Implement the mvprintw commands.  Due to the variable number of
 *	arguments, they cannot be macros.  Sigh....
 */
int
mvprintw(int y, int x, const char *fmt,...)
{
	va_list ap;
	int     ret;

	if (move(y, x) != OK)
		return ERR;
	va_start(ap, fmt);
	ret = vw_printw(stdscr, fmt, ap);
	va_end(ap);
	return ret;
}

int
mvwprintw(WINDOW * win, int y, int x, const char *fmt,...)
{
	va_list ap;
	int     ret;

	if (wmove(win, y, x) != OK)
		return ERR;

	va_start(ap, fmt);
	ret = vw_printw(win, fmt, ap);
	va_end(ap);
	return ret;
}

/*
 * vw_printw --
 *	This routine actually executes the printf and adds it to the window.
 */
int
vw_printw(WINDOW *win, const char *fmt, va_list ap)
{
	char	stackbuf[256];
	char	*buf = stackbuf;
	va_list	ap2;
	int	len, n;

	/*
	 * EwokOS libc has no open_memstream(), so format into a stack buffer
	 * and fall back to the heap for longer output. The WINDOW fp/buf fields
	 * stay unused (NULL, as set by newwin()).
	 */
	va_copy(ap2, ap);
	len = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap2);
	va_end(ap2);
	if (__predict_false(len < 0))
		return ERR;
	if (len == 0)
		return OK;
	if (__predict_false((size_t)len >= sizeof(stackbuf))) {
		buf = malloc((size_t)len + 1);
		if (__predict_false(buf == NULL))
			return ERR;
		va_copy(ap2, ap);
		n = vsnprintf(buf, (size_t)len + 1, fmt, ap2);
		va_end(ap2);
		if (__predict_false(n < 0)) {
			free(buf);
			return ERR;
		}
		len = n;
	}
	n = waddnstr(win, buf, len);
	if (buf != stackbuf)
		free(buf);
	return n;
}

__strong_alias(vwprintw, vw_printw)

