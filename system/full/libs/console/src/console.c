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

static uint16_t _fontw = 0;
static uint32_t font_width(console_t* console) {
	if(_fontw != 0)
		return _fontw;

	font_char_size('M', &console->font, &_fontw, NULL);
	if(_fontw == 0)
		_fontw = console->font.max_size.x;
	_fontw += console->font_margin;
	return _fontw;
}

static void cons_draw_char(console_t* console, graph_t* g, int32_t x, int32_t y, UNICODE16 c, int32_t w) {
	graph_draw_char_font_fixed(g, x, y, c, &console->font, console->fg_color, w, 0);
}

static uint32_t get_data_rows(console_t* console) {
	if(console->state.start_row != 0)
		return console->content.rows;
	return console->state.current_row;
}

int32_t console_reset(console_t* console, uint32_t w, uint32_t h, uint32_t total_rows) {
	if(console->font.id < 0)
		return -1;
	console->w = w;
	console->h = h;
	//save content data
	int old_size = console->state.size;
	int old_total = console->content.cols* console->content.rows;
	int old_cols = console->content.cols;
	int old_start_row = console->state.start_row;
	UNICODE16* old_data = NULL;
	if(old_total > 0 && console->content.data != NULL) {
		old_data = (char*)malloc(old_total*2);
		memcpy(old_data, console->content.data, old_total*2);
	}

	console->state.size = 0;
	console->state.start_row = 0;
	console->state.back_offset_rows = 0;
	console->state.current_row = 0;
	int32_t font_w = font_width(console);
	if(font_w <= 0)
		font_w = 1;
	console->content.cols = w / font_w - 1;

	uint32_t min_rows = h / console->font.max_size.y;
	if(total_rows < min_rows)
		total_rows = min_rows;
	console->content.rows = total_rows;
	uint32_t data_size = console->content.rows*console->content.cols;
	if(console->content.data != NULL) {
		free(console->content.data);
		console->content.data = NULL;
	}

	if(data_size > 0) {
		console->content.data = (char*)malloc(data_size*2);
		memset(console->content.data, 0, data_size*2);
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
		UNICODE16 c = old_data[at];
		if(c != 0) {
			console_put_char(console, c);
		}
	}
	free(old_data);
	return 0;
}

int32_t console_init(console_t* console) {
	console->w = 0;
	console->h = 0;
	console->font_margin = 0;
	console->bg_color = argb(0xff, 0x0, 0x0, 0x0);
	console->fg_color = argb(0xff, 0xaa, 0xaa, 0xaa);
	console->font.id = -1;
	_fontw = 0;
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

void console_roll(console_t* console, int32_t rows) {
	if(rows > 0) {//forward
		if(console->state.back_offset_rows > rows)
			console->state.back_offset_rows -= rows;
		else
			console->state.back_offset_rows = 0;
		return;
	}
	rows = -rows;
	console->state.back_offset_rows += rows;
	uint32_t total = get_data_rows(console);
	if(console->state.back_offset_rows > total)
		console->state.back_offset_rows = total;
}

void console_refresh_content(console_t* console, graph_t* g) {
	if(console->font.id < 0)
		return;
	uint32_t g_rows = g->h / console->font.max_size.y;
	uint32_t total_rows = get_data_rows(console);
	int32_t start_row = total_rows - g_rows - console->state.back_offset_rows;
	if(start_row < 0)
		start_row = 0;
	if(start_row != 0)
		start_row++;

	uint32_t i = start_row * console->content.cols;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t w = font_width(console);
	if(w <= 0)
		w = 1;
	uint32_t h = console->font.max_size.y;
	while(i < console->state.size) {
		uint32_t at = get_at(console, i);
		UNICODE16 c = console->content.data[at];
		if(c != 0 && c != '\n') {
			cons_draw_char(console, g, x*w, y*h, console->content.data[at], w);
		}
		x++;
		if(x >= console->content.cols) {
			y++;
			x = 0;
		}
		i++;
	}	
	//draw cursor
	if(console->show_cursor)
		graph_fill(g, x*w, y*h, w, h, console->fg_color);
}

void console_refresh(console_t* console, graph_t* g) {
	graph_clear(g, console->bg_color);
	console_refresh_content(console, g);
}

void console_clear(console_t* console) {
	console->state.size = 0;
	console->state.start_row = 0;
	console->state.back_offset_rows = 0;
	console->state.current_row = 0;
}

static void move_line(console_t* console) {
	console->state.current_row--;
	console->state.start_row++;
	if(console->state.start_row >= console->content.rows)
		console->state.start_row = 0;
	console->state.size -= console->content.cols;
}

void console_put_char(console_t* console, UNICODE16 c) {
	if(console->content.data == NULL)
		return;

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

