#include <userman.h>
#include <pmessage.h>
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

	ProtoT* proto = protoNew(NULL, 0);
	protoAddStr(proto, name);
	protoAddStr(proto, passwd);
	PackageT* pkg = preq(_usermanPid, 0, 0, proto->data, proto->size, true);
	protoFree(proto);
	if(pkg == NULL)	
		return -1;

	uid = *(int*)getPackageData(pkg);
	free(pkg);
	return uid;
}

