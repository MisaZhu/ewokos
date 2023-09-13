#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <string.h>
#include <textview/textview.h>

#ifdef __cplusplus
extern "C" {
#endif

#define T_W 2 /*tab width*/

static uint32_t font_width(textview_t* textview) {
	if(textview->font_fixed == 0)
		textview->font_fixed = textview->font_size/2;
	uint16_t fontw = textview->font_fixed;
	if(fontw == 0)
		font_char_size('i', &textview->font, &fontw, NULL);
	if(fontw == 0)
		fontw = 8; //minimize width 
	textview->font_fixed = fontw;
	return textview->font_fixed;
}

static void tv_draw_char(textview_t* textview, graph_t* g, int32_t x, int32_t y, UNICODE16 c, int32_t *w) {
	if(textview->font_fixed > 0 && c < 128) {
		graph_draw_char_font_fixed(g, x, y, c, &textview->font, textview->fg_color, textview->font_fixed, 0);
		*w = textview->font_fixed;
	}
	else {
		graph_draw_char_font(g, x, y, c, &textview->font, textview->fg_color, w, NULL); 
	}
}

static void clear_lines(line_t* head) {
	line_t* l = head;
	while(l != NULL) {
			line_t* p = l;
			l = l->next;

			if(p->line != NULL)
				free(p->line);
			free(p);
	}
}

int32_t textview_reset(textview_t* textview, uint32_t w, uint32_t h) {
	if(textview->font.id < 0)
		return -1;
	textview->w = w;
	textview->h = h;
	//save content data
	line_t* head = textview->content.head;
	textview->content.head = textview->content.tail = textview->content.start = NULL;
	textview->content.cols = w / font_width(textview);
	textview->content.rows = h / textview->font.max_size.y;

	line_t* l = head;
	while(l != NULL) {
		if(l->line != NULL) {
			for(int i=0; i<l->size; i++) {	
				UNICODE16 c = l->line[i];
				if(c != 0) {
					textview_put_char(textview, c);
				}
			}
		}
		l = l->next;
	}

	clear_lines(head);
	return 0;
}

int32_t textview_init(textview_t* textview) {
	textview->w = 0;
	textview->h = 0;
	textview->last_x = 0;
	textview->last_y = 0;
	textview->font_size = 8;
	textview->bg_color = argb(0xff, 0x0, 0x0, 0x0);
	textview->fg_color = argb(0xff, 0xaa, 0xaa, 0xaa);
	textview->font.id = -1;
	memset(&textview->content, 0, sizeof(content_t));
	return 0;
}

void textview_close(textview_t* textview) {
	textview_clear(textview);
}

static void to_last_line(textview_t* textview) {
	line_t* l = textview->content.tail;
	for(int i=1; i<textview->content.rows; i++) {
		if(l->prev == NULL)
			break;
		l = l->prev;
	}
	textview->content.start = l;
}

void textview_roll(textview_t* textview, int32_t rows) {
	if(rows > 0) {//forward
		line_t* l = textview->content.start;
		while(rows >= 0) {
			if(l == NULL)
				break;
			l = l->next;
			rows--;
		}
		if(l == NULL)
			to_last_line(textview);
		else
			textview->content.start = l;
		return;
	}

	//backward
	rows = -rows;
	line_t* l = textview->content.start;
	while(rows >= 0) {
		if(l == NULL)
			break;
		l = l->prev;
		rows--;
	}
	if(l == NULL)
		l = textview->content.head;
	textview->content.start = l;
}

void textview_refresh_content(textview_t* textview, graph_t* g) {
	if(textview->font.id < 0)
		return;

	int32_t x = 0;
	int32_t y = 0;
	int32_t w = 0;
	int32_t h = textview->font.max_size.y;

	line_t* l = textview->content.start;
	while(l != NULL) {
		if(l->line != NULL) {
			for(int i=0; i<l->size; i++) {	
				UNICODE16 c = l->line[i];
				if(c != 0 && c != '\n') {
					tv_draw_char(textview, g, x, y*h, c, &w);
					x += w;
				}
			}
		}
		l = l->next;
		if(l != NULL) {
			y++;
			x = 0;
		}
	}
	textview->last_x = x;
	textview->last_y = y*h;
}

void textview_refresh(textview_t* textview, graph_t* g) {
	graph_clear(g, textview->bg_color);
	textview_refresh_content(textview, g);
}

void textview_clear(textview_t* textview) {
	clear_lines(textview->content.head);
	textview->content.head =
			textview->content.start =
			textview->content.tail = NULL;
}

static line_t* new_line(textview_t* textview) {
	line_t* line = (line_t*)calloc(sizeof(line_t), 1);
	line->line = (UNICODE16*)calloc(textview->content.cols*2, 1);
	return line;
}

static void move_line(textview_t* textview) {
	line_t* nline = new_line(textview);
	if(textview->content.tail == NULL)
		textview->content.head = textview->content.start = nline;
	else
		textview->content.tail->next = nline;

	nline->prev = textview->content.tail;
	textview->content.tail = nline;
}

void textview_put_char(textview_t* textview, UNICODE16 c) {
	if(textview->content.tail == NULL)
		move_line(textview);
	if(textview->content.tail == NULL)
		return;

	if(c == '\r')
		c = '\n';

	if(c == '\t') {
		uint32_t x = 0;
		while(x < T_W) {
			textview_put_char(textview, ' ');
			x++;
		}
		return;
	}

	uint16_t cw;
	font_char_size(c, &textview->font, &cw, NULL);

	if((textview->content.tail->size+2) >= textview->content.cols || 
			(textview->content.tail->last_x+cw) >= textview->w)
		move_line(textview);

	textview->content.tail->line[textview->content.tail->size] = c;
	textview->content.tail->size++;
	textview->content.tail->last_x += cw;
	if(c == '\n') {
		move_line(textview);
	}
}

void textview_put_string(textview_t* textview, const char* str, int len) {
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		textview_put_char(textview, out[i]);
	}
	free(out);
}

#ifdef __cplusplus
}
#endif

