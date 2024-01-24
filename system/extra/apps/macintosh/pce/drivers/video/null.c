/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/drivers/video/null.c                                     *
 * Created:     2003-10-18 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2003-2015 Hampa Hug <hampa@hampa.ch>                     *
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


#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <drivers/video/terminal.h>
#include <drivers/video/null.h>

static terminal_t *TERM;

static
void null_free (null_t *nt)
{
}

static
void null_del (null_t *nt)
{
	if (nt != NULL) {
		null_free (nt);
		free (nt);
	}
}

static
int null_open (null_t *nt, unsigned w, unsigned h)
{
	printf("%s\n", __func__);
	return (0);
}

static
int null_close (null_t *nt)
{
	printf("%s\n", __func__);
	return (0);
}

static
int null_set_msg_trm (null_t *nt, const char *msg, const char *val)
{
	printf("set msg: %s\n", msg);
	if (strcmp (msg, "term.grab") == 0) {
		return (0);
	}
	else if (strcmp (msg, "term.release") == 0) {
		return (0);
	}
	else if (strcmp (msg, "term.title") == 0) {
		return (0);
	}
	else if (strcmp (msg, "term.fullscreen.toggle") == 0) {
		return (0);
	}
	else if (strcmp (msg, "term.fullscreen") == 0) {
		return (0);

	}else if (strcmp (msg, "term.set_border_x") == 0) {
		return(0);

	}else if (strcmp (msg, "term.set_border_y") == 0) {
		return(0);

	}

	return (-1);
}

extern void repaint(uint32_t* fb);

static void null_update (null_t *vt)
{
	terminal_t     *trm = &vt->trm;	
	repaint((uint32_t*)trm->buf);
}

static
void null_check (null_t *vt)
{
}

static
void null_init (null_t *nt, ini_sct_t *ini)
{
	trm_init (&nt->trm, nt);

	nt->trm.del = (void *) null_del;
	nt->trm.open = (void *) null_open;
	nt->trm.close = (void *) null_close;
	nt->trm.set_msg_trm = (void *) null_set_msg_trm;
	nt->trm.update = (void *) null_update;
	nt->trm.check = (void *) null_check;

	TERM = &nt->trm;
}

terminal_t *null_new (ini_sct_t *ini)
{
	null_t *nt;

	nt = malloc (sizeof (null_t));
	if (nt == NULL) {
		return (NULL);
	}

	null_init (nt, ini);

	return (&nt->trm);
}

void update_mouse(int x, int y, uint8_t but){
	if(TERM != NULL){
		trm_set_mouse(TERM, x, y, but);
	}
}
