#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <graph/graph.h>
#include <graph/font.h>

void fbtest() {
	font_t* font = get_font("16x32");
	graph_t* g = graph_open("/dev/fb0");
	if(g == NULL) {
		return;
	}

	int i = 0;
	char s[32];
	while(i<200) {
		clear(g, rgb(100, 100, 200));
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)", i++);
		fill(g, 10, 10, font->w* strlen(s) + 20, font->h + 20, rgb(0, 0, 0));
		draw_text(g, 20, 20, s, font, rgb(255, 255, 255));
		graph_flush(g);
		yield();
	}

	graph_close(g);
}

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		fbtest();
	}
	else {
		int i = 0;
		while(i++ < atoi(arg)) {
			int pid = fork();
			if(pid == 0) {
				fbtest();
				return 0;
			}
		}
		while(true) {
			yield();
		}
	}	
	return 0;
}

