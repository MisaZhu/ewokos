/* $NetBSD: termcap.c,v 1.25 2023-01-31 21:11:24 andvar Exp $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <netbsd_sys/cdefs.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <term_private.h>
#include <term.h>
#include <termcap.h>

/* termcap id -> terminfo index mapping tables. */
#include "termcap_map.c"

char *UP;
char *BC;

/*
 * Upstream NetBSD resolves the termcap id -> terminfo index with a generated
 * perfect hash (termcap_hash.c). That generated file is not part of this
 * trimmed port, so we resolve the mapping with a simple linear scan over the
 * (small) termcap_map.c tables instead. The tables are identical to upstream,
 * so the observable behaviour is unchanged.
 */
static const TENTRY *
_ti_cap_find(const TENTRY *ents, size_t n, const char *id)
{
	size_t i;

	for (i = 0; i < n; i++)
		if (strcmp(id, ents[i].id) == 0)
			return &ents[i];
	return NULL;
}

int
tgetent(char *bp, const char *name)
{
	int errret;
	static TERMINAL *last = NULL;

	_DIAGASSERT(name != NULL);
	(void)bp; /* termcap buffer is unused; we drive terminfo directly. */

	/* Free the old term */
	if (cur_term != NULL) {
		if (last != NULL && cur_term != last)
			del_curterm(last);
		last = cur_term;
	}
	errret = -1;
	if (setupterm(name, STDOUT_FILENO, &errret) != 0)
		return errret;

	if (last == NULL)
		last = cur_term;

	if (pad_char != NULL)
		PC = pad_char[0];
	UP = __UNCONST(cursor_up);
	BC = __UNCONST(cursor_left);
	return 1;
}

int
tgetflag(const char *id2)
{
	const char id[] = { id2[0], id2[0] ? id2[1] : '\0', '\0' };
	const TENTRY *te;
	TERMUSERDEF *ud;
	size_t i;

	if (cur_term == NULL)
		return 0;

	te = _ti_cap_find(_ti_cap_flagids, __arraycount(_ti_cap_flagids), id);
	if (te != NULL)
		return cur_term->flags[te->ti];

	for (i = 0; i < cur_term->_nuserdefs; i++) {
		ud = &cur_term->_userdefs[i];
		if (ud->type == 'f' && strcmp(ud->id, id) == 0)
			return ud->flag;
	}
	return 0;
}

int
tgetnum(const char *id2)
{
	const char id[] = { id2[0], id2[0] ? id2[1] : '\0', '\0' };
	const TENTRY *te;
	TERMUSERDEF *ud;
	size_t i;

	if (cur_term == NULL)
		return -1;

	te = _ti_cap_find(_ti_cap_numids, __arraycount(_ti_cap_numids), id);
	if (te != NULL) {
		if (!VALID_NUMERIC(cur_term->nums[te->ti]))
			return ABSENT_NUMERIC;
		return cur_term->nums[te->ti];
	}

	for (i = 0; i < cur_term->_nuserdefs; i++) {
		ud = &cur_term->_userdefs[i];
		if (ud->type == 'n' && strcmp(ud->id, id) == 0) {
			if (!VALID_NUMERIC(ud->num))
				return ABSENT_NUMERIC;
			return ud->num;
		}
	}
	return -1;
}

char *
tgetstr(const char *id2, char **area)
{
	const char id[] = { id2[0], id2[0] ? id2[1] : '\0', '\0' };
	const TENTRY *te;
	const char *str;
	TERMUSERDEF *ud;
	size_t i;

	if (cur_term == NULL)
		return NULL;

	str = NULL;
	te = _ti_cap_find(_ti_cap_strids, __arraycount(_ti_cap_strids), id);
	if (te != NULL) {
		str = cur_term->strs[te->ti];
		if (str == NULL)
			return NULL;
	}
	if (str != NULL)
		for (i = 0; i < cur_term->_nuserdefs; i++) {
			ud = &cur_term->_userdefs[i];
			if (ud->type == 's' && strcmp(ud->id, id) == 0)
				str = ud->str;
		}

	/* XXX: FIXME
	 * We should fix sgr0(me) as it has a slightly different meaning
	 * for termcap. */

	if (str != NULL && area != NULL && *area != NULL) {
		char *s;
		s = *area;
		strcpy(*area, str);
		*area += strlen(*area) + 1;
		return s;
	}

	return __UNCONST(str);
}

char *
tgoto(const char *cm, int destcol, int destline)
{

	_DIAGASSERT(cm != NULL);
	return tiparm(cm, destline, destcol);
}
