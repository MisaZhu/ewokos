#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <string.h>
#include <console/console.h>

#ifdef __cplusplus
extern "C" {
#endif

#define T_W 2 /*tab width*/

static uint32_t font_width(console_t* console) {
	uint16_t fontw = console->font_fixed;
	if(fontw == 0)
		font_char_size('i', &console->font, &fontw, NULL);
	if(fontw == 0)
		fontw = 8; //minimize width 
	return fontw;
}

static void cons_draw_char(console_t* console, graph_t* g, int32_t x, int32_t y, UNICODE16 c, int32_t *w) {
	if(console->font_fixed > 0 && c < 128) {
		graph_draw_char_font_fixed(g, x, y, c, &console->font, console->fg_color, console->font_fixed, 0);
		*w = console->font_fixed;
	}
	else {
		graph_draw_char_font(g, x, y, c, &console->font, console->fg_color, w, NULL); 
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

int32_t console_reset(console_t* console, uint32_t w, uint32_t h) {
	if(console->font.id < 0)
		return -1;
	console->w = w;
	console->h = h;
	//save content data
	line_t* head = console->content.head;
	console->content.head = console->content.tail = console->content.start = NULL;
	console->content.cols = w / font_width(console);
	console->content.rows = h / console->font.max_size.y;

	line_t* l = head;
	while(l != NULL) {
		if(l->line != NULL) {
			for(int i=0; i<l->size; i++) {	
				UNICODE16 c = l->line[i];
				if(c != 0) {
					console_put_char(console, c);
				}
			}
		}
		l = l->next;
	}

	clear_lines(head);
	return 0;
}

int32_t console_init(console_t* console) {
	console->w = 0;
	console->h = 0;
	console->font_size = 8;
	console->bg_color = argb(0xff, 0x0, 0x0, 0x0);
	console->fg_color = argb(0xff, 0xaa, 0xaa, 0xaa);
	console->font.id = -1;
	memset(&console->content, 0, sizeof(content_t));
	return 0;
}

void console_close(console_t* console) {
	console_clear(console);
}

static void to_last_line(console_t* console) {
	line_t* l = console->content.tail;
	for(int i=1; i<console->content.rows; i++) {
		if(l->prev == NULL)
			break;
		l = l->prev;
	}
	console->content.start = l;
}

void console_roll(console_t* console, int32_t rows) {
	if(rows > 0) {//forward
		line_t* l = console->content.start;
		while(rows >= 0) {
			if(l == NULL)
				break;
			l = l->next;
			rows--;
		}
		if(l == NULL)
			to_last_line(console);
		else
			console->content.start = l;
		return;
	}

	//backward
	rows = -rows;
	line_t* l = console->content.start;
	while(rows >= 0) {
		if(l == NULL)
			break;
		l = l->prev;
		rows--;
	}
	if(l == NULL)
		l = console->content.head;
	console->content.start = l;
}

void console_refresh_content(console_t* console, graph_t* g) {
	if(console->font.id < 0)
		return;

	int32_t x = 0;
	int32_t y = 0;
	int32_t w = 0;
	int32_t h = console->font.max_size.y;

	line_t* l = console->content.start;
	while(l != NULL) {
		if(l->line != NULL) {
			for(int i=0; i<l->size; i++) {	
				UNICODE16 c = l->line[i];
				if(c != 0 && c != '\n') {
					cons_draw_char(console, g, x, y*h, c, &w);
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

	//draw cursor
	if(console->show_cursor)
		graph_fill(g, x, y*h, w, h, console->fg_color);
}

void console_refresh(console_t* console, graph_t* g) {
	graph_clear(g, console->bg_color);
	console_refresh_content(console, g);
}

void console_clear(console_t* console) {
	clear_lines(console->content.head);
	console->content.head =
			console->content.start =
			console->content.tail = NULL;
}

static line_t* new_line(console_t* console) {
	line_t* line = (line_t*)calloc(sizeof(line_t), 1);
	line->line = (UNICODE16*)calloc(console->content.cols*2, 1);
	return line;
}

static void move_line(console_t* console) {
	line_t* nline = new_line(console);
	if(console->content.tail == NULL)
		console->content.head = console->content.start = nline;
	else
		console->content.tail->next = nline;

	nline->prev = console->content.tail;
	console->content.tail = nline;

	to_last_line(console);	
}

void console_put_char(console_t* console, UNICODE16 c) {
	if(console->content.tail == NULL)
		move_line(console);
	if(console->content.tail == NULL)
		return;

	if(c == '\r')
		c = '\n';

	if(c == CONSOLE_LEFT) { //backspace
		if(console->content.tail->size > 0) {
			console->content.tail->size--;
			console->content.tail->line[console->content.tail->size] = 0;
		}
		return;
	}
	else if(c == '\t') {
		uint32_t x = 0;
		while(x < T_W) {
			console_put_char(console, ' ');
			x++;
		}
		return;
	}

	uint16_t cw;
	font_char_size(c, &console->font, &cw, NULL);

	if((console->content.tail->size+2) >= console->content.cols || 
			(console->content.tail->last_x+cw) >= console->w)
		move_line(console);

	console->content.tail->line[console->content.tail->size] = c;
	console->content.tail->size++;
	console->content.tail->last_x += cw;
	if(c == '\n') {
		move_line(console);
	}
}

void console_put_string(console_t* console, const char* str, int len) {
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		console_put_char(console, out[i]);
	}
	free(out);
}

#ifdef __cplusplus
}
#endif

