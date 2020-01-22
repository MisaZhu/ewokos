#include <basic_math.h>
#include <kstring.h>
#include <console.h>
#include <graph.h>
#include <mm/kmalloc.h>

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
	//save content data
	int old_size = console->content.size;
	int old_total = console->content.total;
	int old_line_w = console->content.line_w;
	int old_start_line = console->content.start_line;
	char* old_data = (char*)kmalloc(old_total);
	memcpy(old_data, console->content.data, old_total);

	console->content.size = 0;
	console->content.start_line = 0;
	console->content.line = 0;
	console->content.line_w = div_u32(console->g->w, console->font->w)-1;
	console->content.line_num = div_u32(console->g->h, console->font->h)-1;
	if(mod_u32(console->g->h, console->font->h) != 0)
		console->content.line_num--;
	uint32_t data_size = console->content.line_num*console->content.line_w;
	console->content.total = data_size;
	if(console->content.data != NULL)
		kfree(console->content.data);
	console->content.data = (char*)kmalloc(data_size);
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
		console_put_char(console, c);
		if(mod_u32(i, old_line_w) == 0) {
			console_put_char(console, '\n');
		}
	}
	kfree(old_data);
	return 0;
}

int32_t console_init(console_t* console) {
	console->g = NULL;
	console->fg_color = argb(0xff, 0xff, 0xff, 0xff);
	console->bg_color = argb(0xff, 0x00, 0x00, 0x00);
	console->font = font_by_name("8x16");
	memset(&console->content, 0, sizeof(content_t));
	return 0;
}

void console_close(console_t* console) {
	kfree(console->content.data);
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

	if((console->content.line) > console->content.line_num) {
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
