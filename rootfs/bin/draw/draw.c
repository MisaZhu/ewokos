#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <kstring.h>
#include <graph/graph.h>

void fbtest() {
	graph_t* g = graphOpen("/dev/fb0");
	if(g == NULL) {
		return;
	}

	int32_t i = 0;
	char s[32];
	while(i<500) {
		clear(g, rgb(100, 100, 200));
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)", i++);
		fill(g, 10, 10, font_big.w* strlen(s) + 20, font_big.h + 20, rgb(0, 0, 0));
		drawText(g, 20, 20, s, &font_big, rgb(255, 255, 255));
		graphFlush(g);
	}

	graphClose(g);
}

int main() {
	fbtest();
	return 0;
}

