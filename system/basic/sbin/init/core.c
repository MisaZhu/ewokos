#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/core.h>
#include <sys/vfsc.h>
#include <sys/proc.h>
#include <hashmap.h>
#include <kevent.h>

static map_t* _ipc_servs = NULL; //pids of ipc_servers

static int get_ipc_serv(const char* key) {
	int32_t *v;
	if(hashmap_get(_ipc_servs, key, (void**)&v) == MAP_MISSING) {
		return -1;
	}
	return *v;
}

static void do_ipc_serv_get(proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		PF->addi(out, -1);
		return;
	}
	PF->addi(out, get_ipc_serv(ks_id));
}

static void do_ipc_serv_reg(int pid, proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		PF->addi(out, -1);
		return;
	}

	int32_t* v = (int32_t*)malloc(sizeof(int32_t));
	*v = pid;
	if(hashmap_put(_ipc_servs, ks_id, v) != MAP_OK) {
		PF->addi(out, -1);
		return;
	}
	PF->addi(out, 0);
}

static void do_ipc_serv_unreg(int pid, proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		PF->addi(out, -1);
		return;
	}

	int32_t *v;
	if(hashmap_get(_ipc_servs, ks_id, (void**)&v) == MAP_MISSING) {
		PF->addi(out, -1);
		return;
	}

	if(*v != pid) {
		PF->addi(out, -1);
		return;
	}

	hashmap_remove(_ipc_servs, ks_id);
	free(v);
	PF->addi(out, 0);
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;

	switch(cmd) {
	case CORE_CMD_IPC_SERV_REG: //regiester ipc_server pid
		do_ipc_serv_reg(pid, in, out);
		return;
	case CORE_CMD_IPC_SERV_UNREG: //unregiester ipc_server pid
		do_ipc_serv_unreg(pid, in, out);
		return;
	case CORE_CMD_IPC_SERV_GET: //get ipc_server pid
		do_ipc_serv_get(in, out);
		return;
	}
}

/*----kernel event -------*/

static void do_proc_created(kevent_t* kev) {
	int cpid = kev->data[1];
	proto_t data;
	PF->init(&data)->addi(&data, kev->data[0])->addi(&data, kev->data[1]);

	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		ipc_call(pid, VFS_PROC_CLONE, &data, NULL);
	}

	pid = get_ipc_serv(IPC_SERV_PROC);
	if(pid > 0) {
		ipc_call(pid, PROC_CMD_CLONE, &data, NULL);
	}
	PF->clear(&data);
	proc_wakeup(cpid);
}

static void do_proc_exit(kevent_t* kev) {
	proto_t data;
	PF->init(&data)->addi(&data, kev->data[0]);
	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		ipc_call(pid, VFS_PROC_EXIT, &data, NULL);
	}

	pid = get_ipc_serv(IPC_SERV_PROC);
	if(pid > 0) {
		ipc_call(pid, PROC_CMD_EXIT, &data, NULL);
	}
	PF->clear(&data);
}

static void handle_event(kevent_t* kev) {
	switch(kev->type) {
	case KEV_PROC_EXIT:
		do_proc_exit(kev);
		return;
	case KEV_PROC_CREATED:
		do_proc_created(kev);
		return;
	}
}

void core(void) {
	_ipc_servs = hashmap_new();

	ipc_serv_run(handle_ipc, NULL, IPC_NON_BLOCK);
	syscall0(SYS_CORE_READY);

	while(1) {
		kevent_t* kev = (kevent_t*)syscall0(SYS_GET_KEVENT);
		if(kev != NULL) {
			handle_event(kev);
			free(kev);
		}
		usleep(10000);
	}

	hashmap_free(_ipc_servs);
}

