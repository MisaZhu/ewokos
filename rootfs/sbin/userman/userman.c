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

static int32_t doAuth(int32_t pid, proto_t* in, proto_t* out) {
	int uid = -1;
	const char* user = proto_read_str(in);
	const char* passwd = proto_read_str(in);

	uid = login(user, passwd);
	syscall2(SYSCALL_SET_UID, pid, uid);
	proto_add_int(out, uid);
	return 0;
}

static int32_t ipccall(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	(void)p;

	switch(call_id) {
	case 0: //auth
		return doAuth(pid, in, out);
	}
	return -1;
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

	return kserv_run(ipccall, NULL, NULL, NULL);
}
