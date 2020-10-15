#include <stddef.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/proc.h>

int chdir(const char* path) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, path);
	int res = ipc_call(get_procd_pid(), PROC_CMD_SET_CWD, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

