#include <fb/fb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <sys/proto.h>
#include <sys/vfs.h>

#ifdef __cplusplus 
extern "C" { 
#endif

int fb_size(int fd, int *w, int *h) {
	proto_t out;
	PF->init(&out);
	if(vfs_fcntl(fd, 0, NULL, &out) != 0) { //get fb size
		PF->clear(&out);
		return -1;
	}
	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

graph_t* graph_from_fb(int fd, int *dma_id) {
	int id, w, h;
	void* gbuf;
	graph_t* g;

	*dma_id = 0;

	if(fb_size(fd, &w, &h) != 0 ||
			w <= 0 || h <= 0)
		return NULL;

	id = vfs_dma(fd, NULL);
	if(id <= 0) {
		return NULL;
	}

	gbuf = shm_map(id);
	if(gbuf == NULL) {
		return NULL;
	}
	g = graph_new(gbuf, w, h);
	*dma_id = id;
	return g;
}

#ifdef __cplusplus
}
#endif

