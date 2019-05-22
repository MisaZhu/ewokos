#include <userman.h>
#include <ipc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>

static int _userman_pid = -1;

#define CHECK_KSERV_USERMAN \
	if(_userman_pid < 0) \
		_userman_pid = syscall1(SYSCALL_KSERV_GET_BY_NAME, (int)"serv.userman"); \
	if(_userman_pid < 0) \
		return -1; 

int userman_auth(const char* name, const char* passwd) {
	int uid = -1;
	CHECK_KSERV_USERMAN

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_str(in, name);
	proto_add_str(in, passwd);
	int32_t res = ipc_call(_userman_pid, 0, in, out);
	proto_free(in);
	if(res == 0) {
		uid = proto_read_int(out);
	}
	return uid;
}
