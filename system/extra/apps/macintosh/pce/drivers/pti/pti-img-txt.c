/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/drivers/pti/pti-img-txt.c                                *
 * Created:     2020-04-25 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2020 Hampa Hug <hampa@hampa.ch>                          *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pti.h"
#include "pti-io.h"
#include "pti-img-txt.h"


static int txt_parse_pti (FILE *fp, pti_img_t *img, int top);


static
int txt_skip_line (FILE *fp)
{
	int c;

	while ((c = fgetc (fp)) != EOF) {
		if ((c == 0x0a) || (c == 0x0d)) {
			return (0);
		}
	}

	return (0);
}

static
int txt_getc (FILE *fp)
{
	int c;

	while ((c = fgetc (fp)) != EOF) {
		if ((c == ' ') || (c == '\t')) {
			;
		}
		else if ((c == 0x0a) || (c == 0x0d)) {
			;
		}
		else if ((c == '#') || (c == ';')) {
			if (txt_skip_line (fp)) {
				return (EOF);
			}
		}
		else {
			return (c);
		}
	}

	return (EOF);
}

static
int txt_parse_ident (FILE *fp, char *str, unsigned max)
{
	int      c;
	unsigned i;

	i = 0;

	if ((c = txt_getc (fp)) == EOF) {
		return (1);
	}

	if ((c >= 'a') && (c <= 'z')) {
		;
	}
	else if ((c >= 'A') && (c <= 'Z')) {
		;
	}
	else {
		ungetc (c, fp);
		return (1);
	}

	str[i++] = c;

	while (i < max) {
		if ((c = fgetc (fp)) == EOF) {
			break;
		}

		if ((c >= 'a') && (c <= 'z')) {
			;
		}
		else if ((c >= 'A') && (c <= 'Z')) {
			;
		}
		else if ((c >= '0') && (c <= '9')) {
			;
		}
		else if ((c == '-') || (c == '_')) {
			;
		}
		else {
			ungetc (c, fp);
			break;
		}

		str[i++] = c;
	}

	if (i >= max) {
		return (1);
	}

	str[i] = 0;

	return (0);
}

static
int txt_parse_hex (FILE *fp, unsigned long *val)
{
	int      c, r;
	unsigned d;

	r = 1;
	*val = 0;

	while (1) {
		if ((c = fgetc (fp)) == EOF) {
			return (r);
		}

		if ((c >= '0') && (c <= '9')) {
			d = c - '0';
		}
		else if ((c >= 'A') && (c <= 'F')) {
			d = c - 'A' + 10;
		}
		else if ((c >= 'a') && (c <= 'f')) {
			d = c - 'a' + 10;
		}
		else {
			ungetc (c, fp);
			return (r);
		}

		*val = (*val << 4) + d;
		r = 0;
	}
}

static
int txt_parse_ulong (FILE *fp, unsigned long *val)
{
	int c;

	if ((c = txt_getc (fp)) == EOF) {
		return (1);
	}

	if (c == '$') {
		return (txt_parse_hex (fp, val));
	}

	if ((c >= '0') && (c <= '9')) {
		*val = c - '0';
	}
	else {
		ungetc (c, fp);
		return (1);
	}

	if ((c = fgetc (fp)) == EOF) {
		return (0);
	}

	if ((c == 'x') || (c == 'X')) {
		return (txt_parse_hex (fp, val));
	}

	while ((c >= '0') && (c <= '9')) {
		*val = 10 * *val + (c - '0');

		if ((c = fgetc (fp)) == EOF) {
			return (0);
		}
	}

	ungetc (c, fp);

	return (0);
}

static
int txt_parse_clock (FILE *fp, unsigned long *val)
{
	char str[64];

	if (txt_parse_ulong (fp, val) == 0) {
		return (0);
	}

	if (txt_parse_ident (fp, str, 64)) {
		return (1);
	}

	if (strcmp (str, "PET") == 0) {
		*val = PTI_CLOCK_VIC20_NTSC;
	}
	else if (strcmp (str, "VIC20-NTSC") == 0) {
		*val = PTI_CLOCK_VIC20_NTSC;
	}
	else if (strcmp (str, "VIC20-PAL") == 0) {
		*val = PTI_CLOCK_VIC20_PAL;
	}
	else if (strcmp (str, "C64-NTSC") == 0) {
		*val = PTI_CLOCK_C64_NTSC;
	}
	else if (strcmp (str, "C64-PAL") == 0) {
		*val = PTI_CLOCK_C64_PAL;
	}
	else if (strcmp (str, "C16-NTSC") == 0) {
		*val = PTI_CLOCK_C16_NTSC;
	}
	else if (strcmp (str, "C16-PAL") == 0) {
		*val = PTI_CLOCK_C16_PAL;
	}
	else if (strcmp (str, "PC-PIT") == 0) {
		*val = PTI_CLOCK_PC_PIT;
	}
	else if (strcmp (str, "PC-CPU") == 0) {
		*val = PTI_CLOCK_PC_CPU;
	}
	else {
		return (1);
	}

	return (0);
}

static
int txt_parse_rep (FILE *fp, pti_img_t *img)
{
	unsigned long i, idx1, idx2, cnt;
	unsigned long clk;
	int           level;

	if (txt_parse_ulong (fp, &cnt)) {
		return (1);
	}

	if ((txt_getc (fp)) != '{') {
		return (1);
	}

	idx1 = img->pulse_cnt;

	if (txt_parse_pti (fp, img, 0)) {
		return (1);
	}

	idx2 = img->pulse_cnt;

	if (cnt > 0) {
		cnt -= 1;
	}

	while (cnt-- > 0) {
		for (i = idx1; i < idx2; i++) {
			pti_pulse_get (img->pulse[i], &clk, &level);

			if (pti_img_add_pulse (img, clk, level)) {
				return (1);
			}
		}
	}

	return (0);
}

static
int txt_parse_text (FILE *fp, pti_img_t *img)
{
	int           c, quote, start;
	unsigned      n;
	unsigned char buf[256];

	n = 0;

	start = 0;
	quote = 0;

	if (img->comment_size > 0) {
		buf[n++] = 0x0a;
	}

	while ((c = fgetc (fp)) != EOF) {
		if (start == 0) {
			if ((c == ' ') || (c == '\t')) {
				continue;
			}

			if ((c == '"') || (c == '\'')) {
				quote = c;
				start = 1;
				continue;
			}
		}

		if (c == '\\') {
			if ((c = fgetc (fp)) == EOF) {
				return (1);
			}

			if (c == 'n') {
				c = '\n';
			}
			else if (c == 't') {
				c = '\t';
			}
		}
		else if (quote) {
			if (c == quote) {
				quote = 0;
				break;
			}
		}
		else {
			if ((c == 0x0d) || (c == 0x0a)) {
				break;
			}
		}

		start = 1;

		buf[n++] = c;

		if (n >= 256) {
			if (pti_img_add_comment (img, buf, n)) {
				return (1);
			}

			n = 0;
		}
	}

	if (quote) {
		return (1);
	}

	if (pti_img_add_comment (img, buf, n)) {
		return (1);
	}

	return (0);
}

static
int txt_parse_pti (FILE *fp, pti_img_t *img, int top)
{
	int           c, v, s;
	unsigned long val;
	char          str[64];

	while ((c = txt_getc (fp)) != EOF) {
		if (((c >= 'a') && (c <= 'z')) || ((c >= 'A' && c <= 'Z'))) {
			ungetc (c, fp);

			if (txt_parse_ident (fp, str, 64)) {
				return (1);
			}

			if (strcasecmp (str, "CLOCK") == 0) {
				if (txt_parse_clock (fp, &val)) {
					return (1);
				}

				if (img == NULL) {
					return (1);
				}

				pti_img_set_clock (img, val);
			}
			else if (strcasecmp (str, "REP") == 0) {
				if (txt_parse_rep (fp, img)) {
					return (1);
				}
			}
			else if (strcasecmp (str, "TEXT") == 0) {
				if (txt_parse_text (fp, img)) {
					return (1);
				}
			}
			else if (top) {
				if (top && (strcasecmp (str, "END") == 0)) {
					return (0);
				}
				else if (strcasecmp (str, "PTI") == 0) {
					if (txt_parse_ulong (fp, &val)) {
						return (1);
					}

					if (val != 0) {
						fprintf (stderr,
							"pti: bad version (%lu)\n", val
						);
						return (1);
					}
				}
			}
			else {
				return (1);
			}
		}
		else if ((c == '+') || (c == '-') || (c == '=') || (c == '/')) {
			s = c;

			if (txt_parse_ulong (fp, &val)) {
				return (1);
			}

			if (img == NULL) {
				return (1);
			}

			if (s == '/') {
				if (pti_img_add_pulse (img, val / 2, -1)) {
					return (1);
				}

				if (pti_img_add_pulse (img, val - val / 2, 1)) {
					return (1);
				}
			}
			else {
				if (s == '+') {
					v = 1;
				}
				else if (s == '-') {
					v = -1;
				}
				else {
					v = 0;
				}

				if (pti_img_add_pulse (img, val, v)) {
					return (1);
				}
			}
		}
		else if ((top == 0) && (c == '}')) {
			return (0);
		}
		else {
			return (1);
		}
	}

	return (0);
}

static
int txt_load (FILE *fp, pti_img_t *img)
{
	unsigned long val;
	char          str[16];

	if (txt_parse_ident (fp, str, 16)) {
		return (1);
	}

	if (strcasecmp (str, "PTI") != 0) {
		return (1);
	}

	if (txt_parse_ulong (fp, &val)) {
		return (1);
	}

	if (val != 0) {
		fprintf (stderr, "pti: bad version (%lu)\n", val);
		return (1);
	}

	if (txt_parse_pti (fp, img, 1)) {
		return (1);
	}

	return (0);
}

pti_img_t *pti_load_txt (FILE *fp)
{
	pti_img_t *img;

	if ((img = pti_img_new()) == NULL) {
		return (NULL);
	}

	if (txt_load (fp, img)) {
		pti_img_del (img);
		return (NULL);
	}

	pti_img_clean (img);

	return (img);
}


int pti_txt_save_comment (FILE *fp, const pti_img_t *img)
{
	unsigned long       i, n;
	int                 start;
	const unsigned char *p;

	if (img->comment_size == 0) {
		return (0);
	}

	p = img->comment;
	n = img->comment_size;

	while ((n > 0) && ((*p == 0x0a) || (*p == 0x0d))) {
		p += 1;
		n -= 1;
	}

	while ((n > 0) && ((p[n - 1] == 0x0a) || (p[n - 1] == 0x0d))) {
		n -= 1;
	}

	start = 0;

	fputc ('\n', fp);

	for (i = 0; i < n; i++) {
		if (start == 0) {
			fputs ("TEXT \"", fp);
			start = 1;
		}

		if (p[i] == 0x0a) {
			if (start == 0) {
				continue;
			}

			fputs ("\"\n", fp);
			start = 0;
		}
		else if ((p[i] == '"') || (p[i] == '\\')) {
			fputc ('\\', fp);
			fputc (p[i], fp);
		}
		else {
			fputc (p[i], fp);
		}
	}

	if (start) {
		fputs ("\"\n", fp);
	}

	return (0);
}

int pti_save_txt (FILE *fp, const pti_img_t *img)
{
	unsigned      j;
	unsigned long i;
	unsigned long clk;
	int           val;
	unsigned      col;
	char          buf[256];

	fprintf (fp, "PTI 0\n\n");
	fprintf (fp, "CLOCK %lu\n", pti_img_get_clock (img));

	if (pti_txt_save_comment (fp, img)) {
		return (1);
	}

	fputc ('\n', fp);

	col = 0;

	for (i = 0; i < img->pulse_cnt; i++) {
		pti_pulse_get (img->pulse[i], &clk, &val);

		j = 0;

		do {
			buf[j++] = '0' + (clk % 10);
			clk = clk / 10;
		} while (clk > 0);

		if (val < 0) {
			buf[j++] = '-';
		}
		else if (val > 0) {
			buf[j++] = '+';
		}
		else {
			buf[j++] = '=';
		}

		while (j < 4) {
			buf[j++] = ' ';
		}

		if (((col == 7) && (val < 0)) || ((col > 0) && (val == 0))) {
			fputc ('\n', fp);
			col = 0;
		}
		else if (col > 0) {
			fputc (' ', fp);
		}

		while (j-- > 0) {
			fputc (buf[j], fp);
		}

		col += 1;

		if ((col >= 8) || (val == 0)) {
			fputc ('\n', fp);
			col = 0;
		}
	}

	if (col > 0) {
		fputc ('\n', fp);
	}

	return (0);
}


int pti_probe_txt_fp (FILE *fp)
{
	return (0);
}

int pti_probe_txt (const char *fname)
{
	int  r;
	FILE *fp;

	if ((fp = fopen (fname, "rb")) == NULL) {
		return (0);
	}

	r = pti_probe_txt_fp (fp);

	fclose (fp);

	return (r);
}
