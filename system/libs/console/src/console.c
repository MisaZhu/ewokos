#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/basic_math.h>
#include <sys/keydef.h>
#include <string.h>
#include <console/console.h>

#define T_W 2 /*tab width*/

static void cons_draw_char(console_t* console, int32_t x, int32_t y, char c) {
	if(console->g == NULL)
		return;
	draw_char(console->g, x, y, c, console->font, console->fg_color);
}

static void cons_clear(console_t* console) {
	if(console->g == NULL)
		return;
	clear(console->g, console->bg_color);
}

int32_t console_reset(console_t* console) {
	if(console->g == NULL)
		return 0;
	//save content data
	int old_size = console->content.size;
	int old_total = console->content.total;
	int old_line_w = console->content.line_w;
	int old_start_line = console->content.start_line;
	char* old_data = (char*)malloc(old_total);
	memcpy(old_data, console->content.data, old_total);

	console->content.size = 0;
	console->content.start_line = 0;
	console->content.line = 0;
	console->content.line_w = div_u32(console->g->w, console->font->w)-1;
	console->content.line_num = div_u32(console->g->h, console->font->h);
	uint32_t data_size = console->content.line_num*console->content.line_w;
	console->content.total = data_size;
	if(console->content.data != NULL)
		free(console->content.data);
	console->content.data = (char*)malloc(data_size);
	memset(console->content.data, 0, data_size);
	cons_clear(console);

	//restore old data
	if(old_start_line > 0 && old_size > old_line_w) {
		old_start_line++;
		old_size -= old_line_w;
	}

	int i = 0;
	while(i < old_size) {
		int at = (old_line_w * old_start_line) + (i++);
		if(at >= old_total)
			at -= old_total;
		char c = old_data[at];
		if(c != 0)
			console_put_char(console, c);
	}
	free(old_data);
	return 0;
}

int32_t console_init(console_t* console) {
	console->g = NULL;
	console->bg_color = argb(0xff, 0x0, 0x0, 0x0);
	console->fg_color = argb(0xff, 0xaa, 0xaa, 0xaa);
	console->font = font_by_name("8x16");
	memset(&console->content, 0, sizeof(content_t));
	return 0;
}

void console_close(console_t* console) {
	free(console->content.data);
	console->content.size = 0;
	console->content.data = NULL;
	console->g = NULL;
}

static uint32_t get_at(console_t* console, uint32_t i) {
	uint32_t at = i + (console->content.line_w * console->content.start_line);
	if(at >= console->content.total)
		at -=  console->content.total;
	return at;
}

void console_refresh(console_t* console) {
	if(console->g == NULL)
		return;
	cons_clear(console);
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < console->content.size) {
		uint32_t at = get_at(console, i);
		char c = console->content.data[at];
		if(c != 0 && c != '\n') {
			cons_draw_char(console, x*console->font->w, y*console->font->h, console->content.data[at]);
		}
		x++;
		if(x >= console->content.line_w) {
			y++;
			x = 0;
		}
		i++;
	}	
}

void console_clear(console_t* console) {
	console->content.size = 0;
	console->content.start_line = 0;
	console->content.line = 0;
	console_refresh(console);
}

static void move_line(console_t* console) {
	console->content.line--;
	console->content.start_line++;
	if(console->content.start_line >= console->content.line_num)
		console->content.start_line = 0;
	console->content.size -= console->content.line_w;
	console_refresh(console);
}

void console_put_char(console_t* console, char c) {
	if(c == '\r')
		c = '\n';

	if(c == CONSOLE_LEFT) { //backspace
		if(console->content.size > 0) {
			console->content.size--;
			console_refresh(console);
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
	if(c == '\n') { //new line.
		uint32_t x =  console->content.size - (console->content.line*console->content.line_w);
		while(x < console->content.line_w) {
			uint32_t at = get_at(console, console->content.size);
			console->content.data[at] = c;
			c = 0;
			console->content.size++;
			x++;
		}
		c = '\n';
		console->content.line++;
	}
	else {
		if(c <= 27) 
			return;
		uint32_t x =  console->content.size - (console->content.line*console->content.line_w) + 1;
		if(x == console->content.line_w) {
			console->content.line++;
		}
	}

	if((console->content.line) >= console->content.line_num) {
		move_line(console);
	}
	
	if(c != '\n') {
		uint32_t at = get_at(console, console->content.size);
		console->content.data[at] = c;
		int32_t x = (console->content.size - (console->content.line*console->content.line_w)) * console->font->w;
		int32_t y = console->content.line * console->font->h;
		cons_draw_char(console, x, y, c);
		console->content.size++;
	}
}

void console_put_string(console_t* console, const char* s) {
	while(*s != 0) {
		console_put_char(console, *s);
		s++;
	}
}
