/*	$NetBSD: keyname.c,v 1.8 2019/06/11 10:17:24 blymn Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <netbsd_sys/cdefs.h>

#include <stdlib.h>
#include <string.h>

#include "curses.h"
#include "curses_private.h"

#define KEYNAMEMAX (size_t) 14	/* "KEY_BACKSPACE\0" */
static char name[KEYNAMEMAX + 1];

#define KT_ENTRY(START, KV, KEY) [KV - START] = KEY

static const char keytab1[][5] = {
	KT_ENTRY(0x101, KEY_BREAK, "BREAK"),
	KT_ENTRY(0x101, KEY_DOWN, "DOWN"),
	KT_ENTRY(0x101, KEY_UP, "UP"),
	KT_ENTRY(0x101, KEY_LEFT, "LEFT"),
	KT_ENTRY(0x101, KEY_RIGHT, "RIGHT"),
	KT_ENTRY(0x101, KEY_HOME, "HOME"),
};

static const char keytab2[][6] = {
	KT_ENTRY(0x148, KEY_DL, "DL"),
	KT_ENTRY(0x148, KEY_IL, "IL"),
	KT_ENTRY(0x148, KEY_DC, "DC"),
	KT_ENTRY(0x148, KEY_IC, "IC"),
	KT_ENTRY(0x148, KEY_EIC, "EIC"),
	KT_ENTRY(0x148, KEY_CLEAR, "CLEAR"),
	KT_ENTRY(0x148, KEY_EOS, "EOS"),
	KT_ENTRY(0x148, KEY_EOL, "EOL"),
	KT_ENTRY(0x148, KEY_SF, "SF"),
	KT_ENTRY(0x148, KEY_SR, "SR"),
	KT_ENTRY(0x148, KEY_NPAGE, "NPAGE"),
	KT_ENTRY(0x148, KEY_PPAGE, "PPAGE"),
	KT_ENTRY(0x148, KEY_STAB, "STAB"),
	KT_ENTRY(0x148, KEY_CTAB, "CTAB"),
	KT_ENTRY(0x148, KEY_CATAB, "CATAB"),
	KT_ENTRY(0x148, KEY_ENTER, "ENTER"),
	KT_ENTRY(0x148, KEY_SRESET, "SRESET"),
	KT_ENTRY(0x148, KEY_RESET, "RESET"),
	KT_ENTRY(0x148, KEY_PRINT, "PRINT"),
	KT_ENTRY(0x148, KEY_LL, "LL"),
	KT_ENTRY(0x148, KEY_A1, "A1"),
	KT_ENTRY(0x148, KEY_A3, "A3"),
	KT_ENTRY(0x148, KEY_B2, "B2"),
	KT_ENTRY(0x148, KEY_C1, "C1"),
	KT_ENTRY(0x148, KEY_C3, "C3"),
	KT_ENTRY(0x148, KEY_BTAB, "BTAB"),
	KT_ENTRY(0x148, KEY_BEG, "BEG"),
	KT_ENTRY(0x148, KEY_CANCEL, "CANCEL"),
	KT_ENTRY(0x148, KEY_CLOSE, "CLOSE"),
};

#define KEYTAB3_START KEY_COMMAND
static const char keytab3[][9] = {
	KT_ENTRY(KEYTAB3_START, KEY_COMMAND, "COMMAND"),
	KT_ENTRY(KEYTAB3_START, KEY_COPY, "COPY"),
	KT_ENTRY(KEYTAB3_START, KEY_CREATE, "CREATE"),
	KT_ENTRY(KEYTAB3_START, KEY_END, "END"),
	KT_ENTRY(KEYTAB3_START, KEY_EXIT, "EXIT"),
	KT_ENTRY(KEYTAB3_START, KEY_FIND, "FIND"),
	KT_ENTRY(KEYTAB3_START, KEY_HELP, "HELP"),
	KT_ENTRY(KEYTAB3_START, KEY_MARK, "MARK"),
	KT_ENTRY(KEYTAB3_START, KEY_MESSAGE, "MESSAGE"),
	KT_ENTRY(KEYTAB3_START, KEY_MOVE, "MOVE"),
	KT_ENTRY(KEYTAB3_START, KEY_NEXT, "NEXT"),
	KT_ENTRY(KEYTAB3_START, KEY_OPEN, "OPEN"),
	KT_ENTRY(KEYTAB3_START, KEY_OPTIONS, "OPTIONS"),
	KT_ENTRY(KEYTAB3_START, KEY_PREVIOUS, "PREVIOUS"),
	KT_ENTRY(KEYTAB3_START, KEY_REDO, "REDO"),
	KT_ENTRY(KEYTAB3_START, KEY_REFERENCE, "REFERENCE"),
	KT_ENTRY(KEYTAB3_START, KEY_REFRESH, "REFRESH"),
	KT_ENTRY(KEYTAB3_START, KEY_REPLACE, "REPLACE"),
	KT_ENTRY(KEYTAB3_START, KEY_RESTART, "RESTART"),
	KT_ENTRY(KEYTAB3_START, KEY_RESUME, "RESUME"),
	KT_ENTRY(KEYTAB3_START, KEY_SAVE, "SAVE"),
	KT_ENTRY(KEYTAB3_START, KEY_SBEG, "SBEG"),
	KT_ENTRY(KEYTAB3_START, KEY_SCANCEL, "SCANCEL"),
	KT_ENTRY(KEYTAB3_START, KEY_SCOMMAND, "SCOMMAND"),
	KT_ENTRY(KEYTAB3_START, KEY_SCOPY, "SCOPY"),
	KT_ENTRY(KEYTAB3_START, KEY_SCREATE, "SCREATE"),
	KT_ENTRY(KEYTAB3_START, KEY_SDC, "SDC"),
	KT_ENTRY(KEYTAB3_START, KEY_SDL, "SDL"),
	KT_ENTRY(KEYTAB3_START, KEY_SELECT, "SELECT"),
	KT_ENTRY(KEYTAB3_START, KEY_SEND, "SEND"),
	KT_ENTRY(KEYTAB3_START, KEY_SEOL, "SEOL"),
	KT_ENTRY(KEYTAB3_START, KEY_SEXIT, "SEXIT"),
	KT_ENTRY(KEYTAB3_START, KEY_SFIND, "SFIND"),
	KT_ENTRY(KEYTAB3_START, KEY_SHELP, "SHELP"),
	KT_ENTRY(KEYTAB3_START, KEY_SHOME, "SHOME"),
	KT_ENTRY(KEYTAB3_START, KEY_SIC, "SIC"),
	KT_ENTRY(KEYTAB3_START, KEY_SLEFT, "SLEFT"),
	KT_ENTRY(KEYTAB3_START, KEY_SMESSAGE, "SMESSAGE"),
	KT_ENTRY(KEYTAB3_START, KEY_SMOVE, "SMOVE"),
	KT_ENTRY(KEYTAB3_START, KEY_SNEXT, "SNEXT"),
	KT_ENTRY(KEYTAB3_START, KEY_SOPTIONS, "SOPTIONS"),
	KT_ENTRY(KEYTAB3_START, KEY_SPREVIOUS, "SPREVIOUS"),
	KT_ENTRY(KEYTAB3_START, KEY_SPRINT, "SPRINT"),
	KT_ENTRY(KEYTAB3_START, KEY_SREDO, "SREDO"),
	KT_ENTRY(KEYTAB3_START, KEY_SREPLACE, "SREPLACE"),
	KT_ENTRY(KEYTAB3_START, KEY_SRIGHT, "SRIGHT"),
	KT_ENTRY(KEYTAB3_START, KEY_SRSUME, "SRSUME"),
	KT_ENTRY(KEYTAB3_START, KEY_SSAVE, "SSAVE"),
	KT_ENTRY(KEYTAB3_START, KEY_SSUSPEND, "SSUSPEND"),
	KT_ENTRY(KEYTAB3_START, KEY_SUNDO, "SUNDO"),
	KT_ENTRY(KEYTAB3_START, KEY_SUSPEND, "SUSPEND"),
	KT_ENTRY(KEYTAB3_START, KEY_UNDO, "UNDO"),
	KT_ENTRY(KEYTAB3_START, KEY_MOUSE, "MOUSE"),
};

static char *make_key(const char* s, size_t l) {
	memcpy(name, "KEY_", 4);
	memcpy(name+4,s,l);
	name[4+l]=0;
	return name;
}

/*
 * keyname --
 *	Return name of key or NULL;
 */
char *
keyname(int key)
{
	if (key < 0 || key == 0x100 /* No name. */)
		return "-1";

	/* Control codes */
	if (key < 0x20) {
		name[0] = '^';
		name[1] = (char) (key + 64);	/* Offset of '@' */
		name[2] = '\0';
		return name;
	}

	/* "Normal" keys */
	if (key < 0x7F) {
		name[0] = (char) key;
		name[1] = '\0';
		return name;
	}

	/* Delete key */
	if (key == 0x7F) return "^?";

	/* Meta + control codes */
	if (key < 0x9F) {
		memcpy(name, "M-^", 4);
		name[3] = (char) (key - 64);	/* Offset of '@' */
		name[4] = '\0';
		return name;
	}

	/* Meta + "normal" keys */
	if (key < 0xFF) {
		name[0] = 'M';
		name[1] = '-';
		name[2] = (char) (key - 128);
		name[3] = '\0';
		return name;
	}

	/* Meta + delete key */
	if (key == 0xFF) return "M-^?";

	/* Key names.  Synchronise this with curses.h. */
	if (key <= 0x106)
		return make_key(keytab1[key-0x101], 5);

	if (key == 0x107)
		return "KEY_BACKSPACE";

	/* Function key block (64 keys). */
	if (key < 0x148) {
		snprintf(name, sizeof name, "KEY_F(%d)", key - 0x108);
		return name;
	}
	if(key < KEYTAB3_START)
		return make_key(keytab2[key-0x148], 6);

	if(key <= 0x199)
		return make_key(keytab3[key-KEYTAB3_START], 9);

	if(key == KEY_RESIZE)
		return "KEY_RESIZE";

	/* No more names. */
	return "UNKNOWN KEY";
}
/*
 * key_name --
 *	Return name of key or NULL;
 */
char *
key_name(wchar_t key)
{
#ifndef HAVE_WCHAR
	return NULL;
#else
	char *s = keyname((int) key);
	if (s[0] == 'M' && s[1] == '-') {
		/* Remove the "M-" */
		name[0] = s[2];
		name[1] = 0;
		s = name;
	}
	return s;
#endif /* HAVE_WCHAR */
}
