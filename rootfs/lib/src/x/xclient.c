#include <x/xclient.h>
#include <unistd.h>
#include <shm.h>

int32_t x_open(const char* dev_name, x_t* x) {
	int32_t fd = open(dev_name, O_RDWR);
	if(fd < 0)
		return -1;
	x->x = 0;
	x->y = 0;
	x->w = 0;
	x->h = 0;
	x->fd = fd;
	x->shm_id = -1;
	return 0;
}

void x_close(x_t* x) {
	if(x->fd >= 0)
		close(x->fd);
	if(x->shm_id >= 0)
		shm_unmap(x->shm_id);
}

void x_flush(x_t* x) {
	if(x->fd < 0)
		return;
	x_cmd(x->fd, X_CMD_FLUSH, NULL, NULL);
}

void x_set_title(x_t* x, const char* name) {
	if(x->fd < 0)
		return;
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_str(in, name);
	x_cmd(x->fd, X_CMD_SET_TITLE, in, NULL);
	proto_free(in);
}

void x_set_style(x_t* x, uint32_t style) {
	if(x->fd < 0)
		return;
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, style);
	x_cmd(x->fd, X_CMD_SET_STYLE, in, NULL);
	proto_free(in);
}

void x_set_state(x_t* x, uint32_t state) {
	if(x->fd < 0)
		return;
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, state);
	x_cmd(x->fd, X_CMD_SET_STATE, in, NULL);
	proto_free(in);
}

void x_move_to(x_t* x, int32_t tx, int32_t ty) {
	if(x->fd < 0)
		return;
	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, tx);
	proto_add_int(in, ty);
	x_cmd(x->fd, X_CMD_MOVE_TO, in, NULL);
	proto_free(in);
	x->x = tx;
	x->y = ty;
}

graph_t* x_get_graph(x_t* x) {
	if(x->fd < 0 || x->shm_id < 0 || x->w <= 0 || x->h <= 0)
		return NULL;
	void *p = shm_map(x->shm_id);
	if(p == NULL)
		return NULL;
	return graph_new(p, x->w, x->h);
}

void x_resize_to(x_t* x, int32_t w, int32_t h) {
	if(x->fd < 0 || w <= 0 || h <= 0)
		return;

	proto_t* in;
	in = proto_new(NULL, 0);
	proto_add_int(in, w);
	proto_add_int(in, h);
	if(x_cmd(x->fd, X_CMD_RESIZE_TO, in, NULL) != 0) {
		proto_free(in);
		return;
	}

	if(x->shm_id >= 0)
		shm_unmap(x->shm_id);
	x->shm_id = fs_dma(x->fd, NULL);
	if(x->shm_id < 0)
		return;
	x->w = w;
	x->h = h;
}

int32_t x_get_scr_size(const char* dev_name, gsize_t* size) {
	if(size == NULL || dev_name == NULL || dev_name[0] == 0)
		return -1;

	proto_t* out;
	out = proto_new(NULL, 0);
	if(fs_fctrl(dev_name, X_CMD_GET_SCR_SIZE, NULL, out) != 0) {
		proto_free(out);
		return -1;
	}
	uint32_t sz;
	void*p = proto_read(out, &sz);
	if(sz != sizeof(gsize_t)) {
		proto_free(out);
		return -1;
	}	
	memcpy(size, p, sz);
	proto_free(out);
	return 0;
}

int32_t x_get_event(x_t* x, x_ev_t* ev) {
	if(x->fd < 0 || ev == NULL)
		return -1;

	proto_t* out;
	out = proto_new(NULL, 0);
	if(x_cmd(x->fd, X_CMD_GET_EVENT, NULL, out) != 0) {
		proto_free(out);
		return -1;
	}
	uint32_t sz;
	void*p = proto_read(out, &sz);
	if(sz != sizeof(x_ev_t)) {
		proto_free(out);
		return -1;
	}	
	memcpy(ev, p, sz);
	proto_free(out);
	return 0;
}
