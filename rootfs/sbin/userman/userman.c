#include <unistd.h>
#include <ipc.h>
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

static void doAuth(package_t* pkg) {
	int uid = -1;
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* user = proto_read_str(proto);
	const char* passwd = proto_read_str(proto);
	proto_free(proto);

	uid = login(user, passwd);
	syscall2(SYSCALL_SET_UID, pkg->pid, uid);
	ipc_send(pkg->id, pkg->type, &uid, 4);
}

static void handle(package_t* pkg, void* p) {
	(void)p;

	switch(pkg->type) {
	case 0: //auth
		doAuth(pkg);
		break;
	}
}

void _start() {
	if(kserv_get_pid("kserv.userman") >= 0) {
    printf("Panic: 'kserv.userman' process has been running already!\n");
		exit(0);
	}

	if(!kserv_run("kserv.userman", handle, NULL))
		exit(0);
}
