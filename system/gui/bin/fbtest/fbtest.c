#include <graph/graph.h>
#include <display/display.h>
#include <fb/fb.h>
#include <font/font.h>
#include <mouse/mouse.h>
#include <keyb/keyb.h>
#include <ewoksys/keydef.h>
#include <fcntl.h>

int main(int argc, char** argv) {
    fb_t fb;
    const char* fb_dev = get_display_fb_dev("/dev/display", 0);
	if(fb_open(fb_dev, &fb) != 0)
		return -1;

	graph_t *g = fb_fetch_graph(&fb);
    font_t* font = font_new(DEFAULT_SYSTEM_FONT, true);

    int mouse_fd = open("/dev/mouse0", O_RDONLY | O_NONBLOCK);
    int keyb_fd = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
    int x = 0, y = 0;
    bool dirty = true;
    char txt[64] = {0};
    bool quit = false;
    while(!quit) {
        if(dirty) {
            graph_clear(g, 0xffffffff);
            snprintf(txt, 63, "x:%d, y:%d", x, y);
            graph_fill_round(g, x, y, 100, 20, 4, 0xffff0000);
            graph_draw_text_font(g, x, y, txt, font, 12, 0xff000000);
            dirty = false;
            fb_flush(&fb, true);
        }

        mouse_evt_t mevt;
        keyb_evt_t kevts[KEYB_EVT_MAX];
        if(mouse_read(mouse_fd, &mevt) == 1) {
            x+= mevt.rx;
            y+= mevt.ry;
            if(x < 0) x = 0;
            if(y < 0) y = 0;
            if(x > g->w) x = g->w - 10;
            if(y > g->h) y = g->h - 10;
            dirty = true;
        }

        int n = keyb_read(keyb_fd, kevts, KEYB_EVT_MAX);
        for(int i=0; i<n; i++) {
            if(kevts[i].key == KEY_ESC)
                quit = true;
        }
        if(n <= 0)
            usleep(3000);
    }

    font_free(font);
    close(mouse_fd);
    close(keyb_fd);
    fb_close(&fb);
    return 0;
}