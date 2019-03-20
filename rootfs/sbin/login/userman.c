#include <userman.h>
#include <ipc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>

static int _usermanPid = -1;

#define CHECK_KSERV_USERMAN \
	if(_usermanPid < 0) \
		_usermanPid = syscall1(SYSCALL_KSERV_GET, (int)"kserv.userman"); \
	if(_usermanPid < 0) \
		return -1; 


int usermanAuth(const char* name, const char* passwd) {
	int uid = -1;
	CHECK_KSERV_USERMAN

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, name);
	proto_add_str(proto, passwd);
	package_t* pkg = ipc_req(_usermanPid, 0, 0, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL)	
		return -1;

	uid = *(int*)get_pkg_data(pkg);
	free(pkg);
	return uid;
}

