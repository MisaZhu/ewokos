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

int main(int argc, char* argv[]) {
	const char* kserv_name =  "kserv.userman";
	if(argc >= 2)
		kserv_name = argv[1];

	if(kserv_get_by_name(kserv_name) >= 0) {
    printf("Panic: '%s' process has been running already!\n", kserv_name);
		return -1;
	}
	
	if(kserv_register(kserv_name) != 0) {
    printf("Panic: '%s' service register failed!\n", kserv_name);
		return -1;
	}

	if(kserv_ready() != 0) {
    printf("Panic: '%s' service can not get ready!\n", kserv_name);
		return -1;
	}

	return kserv_run(handle, NULL);
}
