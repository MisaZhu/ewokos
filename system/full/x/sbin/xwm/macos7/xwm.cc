#include <stdio.h>
#include <x++/XWM.h>

using namespace Ewok;

class MacWM : public XWM {
	static void draw_title_pattern(graph_t* g, int x, int y, int w, int h, uint32_t fg) {
		int step = 3;
		y = y + step;
		int steps = h / step;

		for (int i = 0; i < steps; i++)
		{
			graph_line(g, x + 4, y, x + w - 8, y, fg);
			y += step;
		}
	}
protected:
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
		uint32_t fg, bg;
		getColor(&fg, &bg, top);
		gsize_t sz;
		get_text_size(info->title, font, &sz.w, &sz.h);
		
		grect_t rect;
		getTitle(info, &rect);

		int pw = (r->w-sz.w)/2;
		graph_fill(g, r->x, r->y, r->w, rect.h, bg);//title box
		if(top) {
			draw_title_pattern(g, r->x, r->y, pw, r->h, fg);
			draw_title_pattern(g, r->x+pw+sz.w, r->y, pw, r->h, fg);
		}
		graph_draw_text(g, r->x+pw, r->y+2, info->title, font, fg);//title
	}
};

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MacWM xwm;
	xwm.run();
	return 0;
}
