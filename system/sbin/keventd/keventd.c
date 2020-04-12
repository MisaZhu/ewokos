#include <sys/syscall.h>
#include <kevent.h>
#include <sys/proto.h>
#include <sys/ipc.h>
#include <fsinfo.h>
#include <stdlib.h>
#include <unistd.h>

static void do_fsclosed(proto_t *data) {
	fsinfo_t fsinfo;
	int32_t to_pid = proto_read_int(data);
	int32_t from_pid = proto_read_int(data);
	int32_t fd = proto_read_int(data);
	int32_t fuid = proto_read_int(data);
	proto_read_to(data, &fsinfo, sizeof(fsinfo_t));

	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, fd);
	proto_add_int(&in, fuid);
	proto_add_int(&in, from_pid);
	proto_add(&in, &fsinfo, sizeof(fsinfo_t));

	ipc_call(to_pid, FS_CMD_CLOSED, &in, NULL);
	proto_clear(&in);
}

static void handle(kevent_t* kev) {
	if(kev->type == KEV_FCLOSED) {
		do_fsclosed(kev->data);
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	while(1) {
		kevent_t* kev = (kevent_t*)syscall0(SYS_GET_KEVENT);
		if(kev != NULL) {
			handle(kev);
			if(kev->data != NULL)
				proto_free(kev->data);
			free(kev);
		}
		usleep(10000);
	}
	return 0;
}
