#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv.h>
#include <stdio.h>
#include <proto.h>

static int login(const char* user, const char* passwd) {
	//TODO: login by username and password
	if(strcmp(user, passwd) != 0)
		return -1;

	if(strcmp(user, "root") == 0)
		return 0;
	return 1;
}

static void doAuth(PackageT* pkg) {
	int uid = -1;
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	const char* user = protoReadStr(proto);
	const char* passwd = protoReadStr(proto);
	protoFree(proto);

	uid = login(user, passwd);
	syscall2(SYSCALL_SET_UID, pkg->pid, uid);
	psend(pkg->id, pkg->type, &uid, 4);
}

static void handle(PackageT* pkg, void* p) {
	(void)p;

	switch(pkg->type) {
	case 0: //auth
		doAuth(pkg);
	}
}

void _start() {
	if(syscall1(SYSCALL_KSERV_GET, (int)"kserv.userman") >= 0) {
    printf("Panic: 'userman' process has been running already!\n");
		exit(0);
	}

	if(!kservRun("kserv.userman", handle, NULL))
		exit(0);
}
