#include <stddef.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/core.h>

int chdir(const char* path) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, path);
	int res = ipc_call(get_cored_pid(), CORE_CMD_SET_CWD, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res != 0)
			errno = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

