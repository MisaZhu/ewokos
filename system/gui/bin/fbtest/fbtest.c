#include <graph/graph.h>
#include <display/display.h>
#include <fb/fb.h>
#include <font/font.h>
#include <mouse/mouse.h>
#include <fcntl.h>


void keyb_handle(uint8_t key,
        uint8_t state,
        void* p) {
    klog("keyb: %d, %d\n", key, state);
}

void mouse_handle(uint8_t state,
        uint8_t button,
        int32_t rx,
        int32_t ry,
        void* p) {
    klog("mouse: %d, %d, %d, %d\n", state, button, rx, ry);
}

int main(int argc, char** argv) {
    fb_t fb;
    const char* fb_dev = get_display_fb_dev("/dev/display", 0);
	if(fb_open(fb_dev, &fb) != 0)
		return -1;

	graph_t *g = fb_fetch_graph(&fb);
    graph_clear(g, 0xffffffff);
    
    font_t* font = font_new(DEFAULT_SYSTEM_FONT, true);
    graph_draw_text_font(g, 10, 10, "Hello!", font, 16, 0xff000000);

    graph_fill_circle(g, 100, 100, 80, 0xffff0000);


    int mouse_fd = open("/dev/mouse0", O_RDONLY);
    int keyb_fd = open("/dev/keyb0", O_RDONLY);
    while(true) {
        mouse_read(mouse_fd, mouse_handle, NULL);
        keyb_read(keyb_fd, keyb_handle, NULL);
    }

    fb_flush(&fb, true);
    sleep(1);

    font_free(font);
    close(mouse_fd);
    close(keyb_fd);
    fb_close(&fb);
    return 0;
}