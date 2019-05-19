#include <x/xclient.h>
#include <unistd.h>

int32_t x_open(const char* dev_name) {
	return open(dev_name, O_RDWR);
}

void x_close(int32_t fd) {
	close(fd);
}

void x_move_to(int32_t fd, int32_t x, int32_t y) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x);
	proto_add_int(in, y);
	x_cmd(fd, X_CMD_MOVE_TO, in, NULL);
	proto_free(in);
}

void x_resize_to(int32_t fd, int32_t w, int32_t h) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, w);
	proto_add_int(in, h);
	x_cmd(fd, X_CMD_RESIZE_TO, in, NULL);
	proto_free(in);
}

void x_set_bg(int32_t fd, uint32_t color) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, color);
	x_cmd(fd, X_CMD_SET_BG, in, NULL);
	proto_free(in);
}

void x_set_color(int32_t fd, uint32_t color) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, color);
	x_cmd(fd, X_CMD_SET_FG, in, NULL);
	proto_free(in);
}

void x_set_font(int32_t fd, const char* name, uint32_t size, uint32_t style) {
	(void)size;
	(void)style;

	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_str(in, name);
	x_cmd(fd, X_CMD_SET_FONT, in, NULL);
	proto_free(in);
}

void x_pixel(int32_t fd, int32_t x, int32_t y) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x);
	proto_add_int(in, y);
	x_cmd(fd, X_CMD_PIXEL, in, NULL);
	proto_free(in);
}

void x_clear(int32_t fd) {
	x_cmd(fd, X_CMD_CLEAR, NULL, NULL);
}

void x_box(int32_t fd, int32_t x, int32_t y, int32_t w, int32_t h) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x);
	proto_add_int(in, y);
	proto_add_int(in, w);
	proto_add_int(in, h);
	x_cmd(fd, X_CMD_BOX, in, NULL);
	proto_free(in);
}

void x_fill(int32_t fd, int32_t x, int32_t y, int32_t w, int32_t h) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x);
	proto_add_int(in, y);
	proto_add_int(in, w);
	proto_add_int(in, h);
	x_cmd(fd, X_CMD_FILL, in, NULL);
	proto_free(in);
}

void x_line(int32_t fd, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x1);
	proto_add_int(in, y1);
	proto_add_int(in, x2);
	proto_add_int(in, y2);
	x_cmd(fd, X_CMD_LINE, in, NULL);
	proto_free(in);
}

void x_text(int32_t fd, int32_t x, int32_t y, const char* str) {
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, x);
	proto_add_int(in, y);
	proto_add_str(in, str);
	x_cmd(fd, X_CMD_STR, in, NULL);
	proto_free(in);
}

