#include <graph/graph_fb.h>
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

graph_t* graph_from_fb(int fd, int *dma_id) {
	int id, w, h;
	void* gbuf;
	graph_t* g;

	*dma_id = 0;

	id = vfs_dma(fd, NULL);
	if(id <= 0) {
		return NULL;
	}

	gbuf = shm_map(id);
	if(gbuf == NULL) {
		return NULL;
	}

	proto_t out;
	PF->init(&out, NULL, 0);
	if(vfs_fcntl(fd, 0, NULL, &out) != 0) { //get fb size
		shm_unmap(id);
		return NULL;
	}
	w = proto_read_int(&out);
	h = proto_read_int(&out);
	g = graph_new(gbuf, w, h);
	PF->clear(&out);

	*dma_id = id;
	return g;
}

#ifdef __cplusplus
}
#endif

