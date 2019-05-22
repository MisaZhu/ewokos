#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <kserv.h>
#include <proto.h>
#include <graph/graph.h>
#include <x/xclient.h>
#include <shm.h>

static int32_t ipc_call(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	(void)pid;
	(void)call_id;
	(void)in;
	(void)out;
	(void)p;
	return 0;
}

int main(int argc, char* argv[]) {
	const char* dev_name = "/dev/xman0";
	proto_t* out = proto_new(NULL, 0);
	if(fs_fctrl(dev_name, X_CMD_REG_WM, NULL, out) != 0) {
		proto_free(out);
		return -1;
	}

	int32_t shm_id = proto_read_int(out);
	int32_t w = proto_read_int(out);
	int32_t h = proto_read_int(out);

	proto_free(out);

	void* p = shm_map(shm_id);
	if(p == NULL) {
		shm_unmap(shm_id);
		return -1;
	}

	const char* kserv_name =  "serv.xwin";
	if(argc >= 2)
		kserv_name = argv[1];

	if(kserv_get_by_name(kserv_name) >= 0) {
    printf("Panic: '%s' process has been running already!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}
	
	if(kserv_register(kserv_name) != 0) {
    printf("Panic: '%s' service register failed!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	if(kserv_ready() != 0) {
    printf("Panic: '%s' service can not get ready!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	graph_t* g = graph_new(p, w, h);
	kserv_run(ipc_call, g, NULL, NULL);
	shm_unmap(shm_id);
	graph_free(g);
	return 0;
}
