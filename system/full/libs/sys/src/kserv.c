#include <sys/kserv.h> 
#include <sys/core.h> 
#include <sys/ipc.h> 
#include <sys/proc.h> 

int kserv_reg(const char* kserv_id) {
	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, kserv_id);
	if(ipc_call(CORED_PID, CORE_CMD_KSERV_REG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int kserv_get(const char* kserv_id) {
	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, kserv_id);
	if(ipc_call(CORED_PID, CORE_CMD_KSERV_GET, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int kserv_unreg(const char* kserv_id) {
	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, kserv_id);
	if(ipc_call(CORED_PID, CORE_CMD_KSERV_UNREG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

static kserv_handle_t _kserv_handle;

static void handle_ipc(int pid, int cmd, void* p) {
	proto_t* in = ipc_get_arg();

	proto_t out;
	PF->init(&out, NULL, 0);

	_kserv_handle(pid, cmd, in, &out, p);

	proto_free(in);
	ipc_set_return(&out);
	PF->clear(&out);
	ipc_end();
}

void kserv_run(kserv_handle_t handle, void* p, bool prefork) {
	_kserv_handle = handle;

	proc_ready_ping();
	ipc_setup(handle_ipc, p, prefork);
}
