/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/drivers/pti/pti-img-pti.c                                *
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
#include "pti-img-pti.h"


#define PTI_MAGIC_PTI  0x50544920
#define PTI_MAGIC_TEXT 0x54455854
#define PTI_MAGIC_TRAK 0x5452414b
#define PTI_MAGIC_DATA 0x44415441
#define PTI_MAGIC_END  0x454e4420

#define PTI_CRC_POLY   0x1edc6f41


typedef struct {
	FILE          *fp;
	pti_img_t     *img;

	uint32_t      crc;

	char          have_header;

	unsigned long bufmax;
	unsigned char *buf;
} pti_load_t;


static
uint32_t pti_crc (uint32_t crc, const void *buf, unsigned cnt)
{
	const unsigned char *src;
	static char         tab_ok = 0;
	static uint32_t     tab[256];

	if (tab_ok == 0) {
		unsigned i, j;

		for (i = 0; i < 256; i++) {
			tab[i] = (uint32_t) i << 24;

			for (j = 0; j < 8; j++) {
				if (tab[i] & 0x80000000) {
					tab[i] = (tab[i] << 1) ^ PTI_CRC_POLY;
				}
				else {
					tab[i] = tab[i] << 1;
				}
			}
		}

		tab_ok = 1;
	}

	src = buf;

	while (cnt-- > 0) {
		crc = (crc << 8) ^ tab[((crc >> 24) ^ *(src++)) & 0xff];
	}

	return (crc & 0xffffffff);
}

static
int pti_read_crc (pti_load_t *pti, void *buf, unsigned long cnt)
{
	if (pti_read (pti->fp, buf, cnt)) {
		return (1);
	}

	pti->crc = pti_crc (pti->crc, buf, cnt);

	return (0);
}

static
unsigned char *pti_alloc (pti_load_t *pti, unsigned long size)
{
	unsigned char *tmp;

	if (pti->bufmax < size) {
		if ((tmp = realloc (pti->buf, size)) == NULL) {
			return (NULL);
		}

		pti->buf = tmp;
		pti->bufmax = size;
	}

	return (pti->buf);
}

static
void pti_free (pti_load_t *pti)
{
	free (pti->buf);

	pti->buf = NULL;
	pti->bufmax = 0;
}

static
int pti_skip_chunk (pti_load_t *pti, unsigned long size)
{
	unsigned      cnt;
	unsigned char *buf;

	if ((buf = pti_alloc (pti, 4096)) == NULL) {
		return (1);
	}

	size += 4;

	while (size > 0) {
		cnt = (size < 4096) ? size : 4096;

		if (pti_read_crc (pti, buf, cnt)) {
			return (1);
		}

		size -= cnt;
	}

	if (pti->crc != 0) {
		fprintf (stderr, "pti: crc error\n");
		return (1);
	}

	return (0);
}

static
int pti_load_header (pti_load_t *pti, unsigned long size)
{
	unsigned      n;
	unsigned char buf[16];
	unsigned long vers, clock, flags;

	n = 12;

	if (size < 8) {
		return (1);
	}
	else if (size < 12) {
		n = 8;
	}

	if (pti_read_crc (pti, buf, n)) {
		return (1);
	}

	vers = pti_get_uint32_be (buf, 0);

	if (vers != 0) {
		fprintf (stderr, "pti: unknown version number (%lu)\n", vers);
		return (1);
	}

	clock = pti_get_uint32_be (buf, 4);

	pti_img_set_clock (pti->img, clock);

	flags = (n >= 12) ? pti_get_uint32_be (buf, 8) : 0;

	if (flags) {
		fprintf (stderr, "pti: unknown flags (0x%08lx)\n", flags);
	}

	if (pti_skip_chunk (pti, size - n)) {
		return (1);
	}

	return (0);
}

static
int pti_load_comment (pti_load_t *pti, unsigned size)
{
	unsigned      i, j, k, d;
	unsigned char *buf;

	if (size == 0) {
		return (pti_skip_chunk (pti, size));
	}

	if ((buf = pti_alloc (pti, size)) == NULL) {
		return (1);
	}

	if (pti_read_crc (pti, buf, size)) {
		return (1);
	}

	i = 0;
	j = size;

	while (i < j) {
		if ((buf[i] == 0x0d) || (buf[i] == 0x0a)) {
			i += 1;
		}
		else if (buf[i] == 0x00) {
			i += 1;
		}
		else {
			break;
		}
	}

	while (j > i) {
		if ((buf[j - 1] == 0x0d) || (buf[j - 1] == 0x0a)) {
			j -= 1;
		}
		else if (buf[j - 1] == 0x00) {
			j += 1;
		}
		else {
			break;
		}
	}

	if (i == j) {
		return (pti_skip_chunk (pti, 0));
	}

	k = i;
	d = i;

	while (k < j) {
		if (buf[k] == 0x0d) {
			if (((k + 1) < j) && (buf[k + 1] == 0x0a)) {
				k += 1;
			}
			else {
				buf[d++] = 0x0a;
			}
		}
		else {
			buf[d++] = buf[k];
		}

		k += 1;
	}

	j = d;

	if (pti->img->comment_size > 0) {
		unsigned char c;

		c = 0x0a;

		if (pti_img_add_comment (pti->img, &c, 1)) {
			return (1);
		}
	}

	if (pti_img_add_comment (pti->img, buf + i, j - i)) {
		return (1);
	}

	if (pti_skip_chunk (pti, 0)) {
		return (1);
	}

	return (0);
}

static
int pti_decode_data (pti_load_t *pti, unsigned char *buf, unsigned long size)
{
	unsigned      tag, cnt;
	unsigned long clk;
	int           level;

	if (pti_read_crc (pti, buf, size)) {
		return (1);
	}

	while (size > 0) {
		size -= 1;

		tag = *(buf++);

		cnt = ((tag >> 4) & 3) + 1;
		clk = tag & 0x0f;

		if (cnt > size) {
			return (1);
		}

		size -= cnt;

		while (cnt-- > 0) {
			clk = (clk << 8) | *(buf++);
		}

		switch (tag & 0xc0) {
		case 0x00:
			level = 1;
			break;

		case 0x40:
			level = 0;
			break;

		case 0x80:
			level = 0;
			break;

		case 0xc0:
			level = -1;
			break;

		default:
			return (1);
		}

		if ((tag & 0xc0) == 0x80) {
			if (pti_img_add_pulse (pti->img, clk / 2, -1)) {
				return (1);
			}

			if (pti_img_add_pulse (pti->img, clk - (clk / 2), 1)) {
				return (1);
			}
		}
		else {
			if (pti_img_add_pulse (pti->img, clk, level)) {
				return (1);
			}
		}
	}

	return (0);
}

static
int pti_load_data (pti_load_t *pti, unsigned long size)
{
	unsigned char *buf;

	if ((buf = pti_alloc (pti, size)) == NULL) {
		return (1);
	}

	if (pti_decode_data (pti, buf, size)) {
		return (1);
	}

	if (pti_skip_chunk (pti, 0)) {
		return (1);
	}

	return (0);
}

static
int pti_load_image (pti_load_t *pti)
{
	unsigned long type, size;
	unsigned char buf[8];

	pti->have_header = 0;

	while (1) {
		pti->crc = 0;

		if (pti_read_crc (pti, buf, 8)) {
			return (1);
		}

		type = pti_get_uint32_be (buf, 0);
		size = pti_get_uint32_be (buf, 4);

		if (type == PTI_MAGIC_END) {
			if (pti_skip_chunk (pti, size)) {
				return (1);
			}

			return (0);
		}
		else if (type == PTI_MAGIC_PTI) {
			if (pti->have_header) {
				return (1);
			}

			if (pti_load_header (pti, size)) {
				return (1);
			}

			pti->have_header = 1;
		}
		else if (type == PTI_MAGIC_TEXT) {
			if (pti->have_header == 0) {
				return (1);
			}

			if (pti_load_comment (pti, size)) {
				return (1);
			}
		}
		else if (type == PTI_MAGIC_DATA) {
			if (pti->have_header == 0) {
				return (1);
			}

			if (pti_load_data (pti, size)) {
				return (1);
			}
		}
		else {
			if (pti->have_header == 0) {
				return (1);
			}

			if (pti_skip_chunk (pti, size)) {
				return (1);
			}
		}
	}

	return (1);
}

pti_img_t *pti_load_pti (FILE *fp)
{
	int        r;
	pti_load_t pti;

	pti.fp = fp;
	pti.img = NULL;
	pti.buf = NULL;
	pti.bufmax = 0;

	if ((pti.img = pti_img_new()) == NULL) {
		return (NULL);
	}

	r = pti_load_image (&pti);

	pti_free (&pti);

	if (r) {
		pti_img_del (pti.img);
		return (NULL);
	}

	pti_img_clean (pti.img);

	return (pti.img);
}


static
int pti_save_header (FILE *fp, const pti_img_t *img)
{
	unsigned char buf[24];

	pti_set_uint32_be (buf, 0, PTI_MAGIC_PTI);
	pti_set_uint32_be (buf, 4, 12);
	pti_set_uint32_be (buf, 8, 0);
	pti_set_uint32_be (buf, 12, pti_img_get_clock (img));
	pti_set_uint32_be (buf, 16, 0);
	pti_set_uint32_be (buf, 20, pti_crc (0, buf, 20));

	if (pti_write (fp, buf, 24)) {
		return (1);
	}

	return (0);
}

static
int pti_save_end (FILE *fp, const pti_img_t *img)
{
	unsigned char buf[16];

	pti_set_uint32_be (buf, 0, PTI_MAGIC_END);
	pti_set_uint32_be (buf, 4, 0);
	pti_set_uint32_be (buf, 8, pti_crc (0, buf, 8));

	if (pti_write (fp, buf, 12)) {
		return (1);
	}

	return (0);
}

static
int pti_save_comment (FILE *fp, const pti_img_t *img)
{
	unsigned long       i, j;
	unsigned long       crc;
	const unsigned char *src;
	unsigned char       *buf;
	unsigned char       hdr[8];

	if (img->comment_size == 0) {
		return (0);
	}

	if ((buf = malloc (img->comment_size + 2)) == NULL) {
		return (1);
	}

	src = img->comment;

	buf[0] = 0x0a;

	i = 0;
	j = 1;

	while (i < img->comment_size) {
		if ((src[i] == 0x0d) || (src[i] == 0x0a)) {
			i += 1;
		}
		else if (src[i] == 0x00) {
			i += 1;
		}
		else {
			break;
		}
	}

	while (i < img->comment_size) {
		if (src[i] == 0x0d) {
			if (((i + 1) < img->comment_size) && (src[i + 1] == 0x0a)) {
				i += 1;
			}
			else {
				buf[j++] = 0x0a;
			}
		}
		else {
			buf[j++] = src[i];
		}

		i += 1;
	}

	while (j > 1) {
		if ((buf[j - 1] == 0x0a) || (buf[j - 1] == 0x00)) {
			j -= 1;
		}
		else {
			break;
		}
	}

	if (j == 1) {
		free (buf);
		return (0);
	}

	buf[j++] = 0x0a;

	pti_set_uint32_be (hdr, 0, PTI_MAGIC_TEXT);
	pti_set_uint32_be (hdr, 4, j);

	crc = pti_crc (0, hdr, 8);

	if (pti_write (fp, hdr, 8)) {
		return (1);
	}

	crc = pti_crc (crc, buf, j);

	if (pti_write (fp, buf, j)) {
		return (1);
	}

	pti_set_uint32_be (hdr, 0, crc);

	if (pti_write (fp, hdr, 4)) {
		return (1);
	}

	return (0);
}

static
int pti_save_data (FILE *fp, const pti_img_t *img)
{
	int           r;
	unsigned long clk, clk2;
	int           level, level2;
	unsigned      tag;
	unsigned long i, n, max;
	unsigned char *buf, *tmp;

	if (img->pulse_cnt == 0) {
		return (0);
	}

	max = 4096;

	if ((buf = malloc (max)) == NULL) {
		return (1);
	}

	pti_set_uint32_be (buf, 0, PTI_MAGIC_DATA);

	i = 0;
	n = 8;

	while (i < img->pulse_cnt) {
		pti_pulse_get (img->pulse[i++], &clk, &level);

		if (clk == 0) {
			continue;
		}

		if ((n + 16) > max) {
			max *= 2;

			if ((tmp = realloc (buf, max)) == 0) {
				free (buf);
				return (1);
			}

			buf = tmp;
		}

		clk2 = 0;
		level2 = 0;

		if (i < img->pulse_cnt) {
			pti_pulse_get (img->pulse[i], &clk2, &level2);
		}

		if ((level < 0) && (level2 > 0) && (clk == ((clk + clk2) / 2))) {
			i += 1;
			tag = 0x80;
			clk += clk2;
		}
		else if (level < 0) {
			tag = 0xc0;
		}
		else if (level > 0) {
			tag = 0x00;
		}
		else {
			tag = 0x40;
		}

		if (clk < (1UL << 12)) {
			buf[n++] = tag | 0x00 | ((clk >> 8) & 0x0f);

		}
		else if (clk < (1UL << 20)) {
			buf[n++] = tag | 0x10 | ((clk >> 16) & 0x0f);
			buf[n++] = (clk >> 8) & 0xff;
		}
		else if (clk < (1UL << 28)) {
			buf[n++] = tag | 0x20 | ((clk >> 24) & 0x0f);
			buf[n++] = (clk >> 16) & 0xff;
			buf[n++] = (clk >> 8) & 0xff;
		}
		else {
			buf[n++] = tag | 0x30;
			buf[n++] = (clk >> 24) & 0xff;
			buf[n++] = (clk >> 16) & 0xff;
			buf[n++] = (clk >> 8) & 0xff;
		}

		buf[n++] = clk & 0xff;
	}

	pti_set_uint32_be (buf, 4, n - 8);
	pti_set_uint32_be (buf, n, pti_crc (0, buf, n));

	r = pti_write (fp, buf, n + 4);

	free (buf);

	return (r);
}

int pti_save_pti (FILE *fp, const pti_img_t *img)
{
	if (pti_save_header (fp, img)) {
		return (1);
	}

	if (pti_save_comment (fp, img)) {
		return (1);
	}

	if (pti_save_data (fp, img)) {
		return (1);
	}

	if (pti_save_end (fp, img)) {
		return (1);
	}

	return (0);
}


int pti_probe_pti_fp (FILE *fp)
{
	unsigned char buf[4];

	if (pti_read (fp, buf, 4)) {
		return (0);
	}

	if (pti_get_uint32_be (buf, 0) != PTI_MAGIC_PTI) {
		return (0);
	}

	return (1);
}

int pti_probe_pti (const char *fname)
{
	int  r;
	FILE *fp;

	if ((fp = fopen (fname, "rb")) == NULL) {
		return (0);
	}

	r = pti_probe_pti_fp (fp);

	fclose (fp);

	return (r);
}
