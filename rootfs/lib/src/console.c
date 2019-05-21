#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <basic_math.h>
#include <kstring.h>
#include <sconf.h>
#include <console.h>
#include <x/xclient.h>

#define T_W 2 /*tab width*/

static void cons_draw_char(console_t* console, int32_t x, int32_t y, char c) {
	draw_char(console->g, x, y, c, console->font, console->fg_color);
}

static void cons_clear(console_t* console) {
	clear(console->g, console->bg_color);
}

int32_t console_reset(console_t* console) {
	if(console->g == NULL)
		return -1;

	console->content.size = 0;
	console->content.start_line = 0;
	console->content.line = 0;
	console->content.line_w = div_u32(console->g->w, console->font->w)-1;
	console->content.line_num = div_u32(console->g->h, console->font->h)-1;
	console->content.total = console->content.line_num * console->content.line_w;

	uint32_t data_size = console->content.line_num*console->content.line_w;
	if(console->content.data != NULL)
		free(console->content.data);
	console->content.data = (char*)malloc(data_size);
	memset(console->content.data, 0, data_size);

	cons_clear(console);
	return 0;
}

int32_t console_init(console_t* console) {
	console->g = NULL;
	console->bg_color = rgb(0x22, 0x22, 0x66);
	console->fg_color = rgb(0xaa, 0xbb, 0xaa);
	console->font = get_font_by_name("8x16");
	memset(&console->content, 0, sizeof(content_t));
	return 0;
}

void console_close(console_t* console) {
	if(console->g != NULL) {
		graph_free(console->g);
		console->g = NULL;
	}
	free(console->content.data);
	console->content.size = 0;
	console->content.data = NULL;
}

static uint32_t get_at(console_t* console, uint32_t i) {
	uint32_t at = i + (console->content.line_w * console->content.start_line);
	if(at >= console->content.total)
		at -=  console->content.total;
	return at;
}

void console_refresh(console_t* console) {
	cons_clear(console);
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < console->content.size) {
		uint32_t at = get_at(console, i);
		char c = console->content.data[at];
		if(c != ' ') {
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

	if(c == 8) { //backspace
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
			console->content.data[at] = ' ';
			console->content.size++;
			x++;
		}
		console->content.line++;
	}
	else {
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

