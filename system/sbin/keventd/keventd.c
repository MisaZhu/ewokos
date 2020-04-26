#include <sys/syscall.h>
#include <kevent.h>
#include <sys/proto.h>
#include <sys/proc.h>
#include <sys/ipc.h>
#include <sys/kserv.h>
#include <fsinfo.h>
#include <stdlib.h>
#include <unistd.h>
#include <usinterrupt.h>

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

static void do_usint_ps2_key(proto_t* data) {
	int32_t key_scode = proto_read_int(data);
	int32_t pid = syscall1(SYS_GET_USINT_PID, US_INT_PS2_KEY);

	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, key_scode);
	ipc_call(pid, IPC_SAFE_CMD_BASE, &in, NULL);
	proto_clear(&in);
}

static void do_user_space_int(proto_t *data) {
	int32_t usint = proto_read_int(data);
	switch(usint) {
	case US_INT_PS2_KEY:
		do_usint_ps2_key(data);
		return;
	}
}

static void handle_event(kevent_t* kev) {
	if(kev->type == KEV_FCLOSED) {
		do_fsclosed(kev->data);
	}
	if(kev->type == KEV_US_INT) {
		do_user_space_int(kev->data);
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	//int vfsd_pid = kserv_get(KSERV_VFS);

	proc_ready_ping();
	while(1) {
		kevent_t* kev = (kevent_t*)syscall0(SYS_GET_KEVENT);
		if(kev != NULL) {
			handle_event(kev);
			if(kev->data != NULL)
				proto_free(kev->data);
			free(kev);
		}
		usleep(0);
	}
	return 0;
}
