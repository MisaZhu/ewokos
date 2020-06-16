#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/basic_math.h>
#include <sys/keydef.h>
#include <string.h>
#include <console/console.h>

#define T_W 2 /*tab width*/

static void cons_draw_char(console_t* console, graph_t* g, int32_t x, int32_t y, char c) {
	graph_draw_char(g, x, y, c, console->font, console->fg_color);
}

static void cons_clear(console_t* console, graph_t* g) {
	graph_clear(g, console->bg_color);
}

int32_t console_reset(console_t* console, uint32_t w, uint32_t h) {
	//save content data
	int old_size = console->state.size;
	int old_total = console->content.cols* console->content.rows;
	int old_cols = console->content.cols;
	int old_start_row = console->state.start_row;
	char* old_data = NULL;
	if(old_total > 0 && console->content.data != NULL) {
		old_data = (char*)malloc(old_total);
		memcpy(old_data, console->content.data, old_total);
	}

	console->state.size = 0;
	console->state.start_row = 0;
	console->state.current_row = 0;
	console->content.cols = div_u32(w, console->font->w)-1;
	console->content.rows = div_u32(h, console->font->h);
	uint32_t data_size = console->content.rows*console->content.cols;
	if(console->content.data != NULL) {
		free(console->content.data);
		console->content.data = NULL;
	}

	if(data_size > 0) {
		console->content.data = (char*)malloc(data_size);
		memset(console->content.data, 0, data_size);
	}

	if(old_data == NULL)
		return 0;

	//restore old data
	if(old_start_row > 0 && old_size > old_cols) {
		old_start_row++;
		old_size -= old_cols;
	}

	int i = 0;
	while(i < old_size) {
		int at = (old_cols * old_start_row) + (i++);
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
	console->w = 0;
	console->h = 0;
	console->bg_color = argb(0xff, 0x0, 0x0, 0x0);
	console->fg_color = argb(0xff, 0xaa, 0xaa, 0xaa);
	console->font = font_by_name("8x16");
	memset(&console->content, 0, sizeof(content_t));
	return 0;
}

void console_close(console_t* console) {
	free(console->content.data);
	console->state.size = 0;
	console->content.data = NULL;
}

static inline uint32_t get_at(console_t* console, uint32_t i) {
	uint32_t at = i + (console->content.cols * console->state.start_row);
	uint32_t total = console->content.cols* console->content.rows;
	if(at >= total)
		at -=  total;
	return at;
}

void console_refresh(console_t* console, graph_t* g) {
	cons_clear(console, g);
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < console->state.size) {
		uint32_t at = get_at(console, i);
		char c = console->content.data[at];
		if(c != 0 && c != '\n') {
			cons_draw_char(console, g, x*console->font->w, y*console->font->h, console->content.data[at]);
		}
		x++;
		if(x >= console->content.cols) {
			y++;
			x = 0;
		}
		i++;
	}	
}

void console_clear(console_t* console) {
	console->state.size = 0;
	console->state.start_row = 0;
	console->state.current_row = 0;
}

static void move_line(console_t* console) {
	console->state.current_row--;
	console->state.start_row++;
	if(console->state.start_row >= console->content.rows)
		console->state.start_row = 0;
	console->state.size -= console->content.cols;
}

void console_put_char(console_t* console, char c) {
	if(c == '\r')
		c = '\n';

	if(c == CONSOLE_LEFT) { //backspace
		if(console->state.size > 0) {
			console->state.size--;
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
		uint32_t x =  console->state.size - (console->state.current_row*console->content.cols);
		while(x < console->content.cols) {
			uint32_t at = get_at(console, console->state.size);
			console->content.data[at] = c;
			c = 0;
			console->state.size++;
			x++;
		}
		c = '\n';
		console->state.current_row++;
	}
	else {
		if(c <= 27) 
			return;
		uint32_t x =  console->state.size - (console->state.current_row*console->content.cols) + 1;
		if(x == console->content.cols) {
			console->state.current_row++;
		}
	}

	if((console->state.current_row) >= console->content.rows) {
		move_line(console);
	}
	
	if(c != '\n') {
		uint32_t at = get_at(console, console->state.size);
		console->content.data[at] = c;
		console->state.size++;
	}
}

void console_put_string(console_t* console, const char* s) {
	while(*s != 0) {
		console_put_char(console, *s);
		s++;
	}
}
