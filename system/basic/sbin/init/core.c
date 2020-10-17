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
#include <usinterrupt.h>

static map_t* _global = NULL;
static map_t* _ipc_servs = NULL; //pids of ipc_servers

static proto_t* global_get(const char* key) {
	proto_t* ret;
	hashmap_get(_global, key, (void**)&ret);
	return ret;
}

static proto_t* global_set(const char* key, void* data, uint32_t size) {
	proto_t* v = global_get(key);
	if(v != NULL) {
		PF->copy(v, data, size);
	}
	else {
		v = proto_new(data, size);
		hashmap_put(_global, key, v);
	}
	return v;
}

static void global_del(const char* key) {
	proto_t* v = global_get(key);
	hashmap_remove(_global, key);
	proto_free(v);
}

static void do_global_set(proto_t* in) {
	const char* key = proto_read_str(in);
	int32_t size;
	void* data = proto_read(in, &size);
	if(data != NULL) {
		global_set(key, data, size);
	}
}

static void do_global_get(proto_t* in, proto_t* out) {
	const char* key = proto_read_str(in);
	proto_t * v= global_get(key);
	if(v != NULL)
		PF->addi(out, 0)->add(out, v->data, v->size);
	else
		PF->addi(out, -1);
}

static void do_global_del(proto_t* in) {
	const char* key = proto_read_str(in);
	global_del(key);
}

/*----------------------------------------------------------*/

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
	case CORE_CMD_GLOBAL_SET:
		do_global_set(in);
		return;
	case CORE_CMD_GLOBAL_DEL: 
		do_global_del(in);
		return;
	case CORE_CMD_GLOBAL_GET:
		do_global_get(in, out);
		return;
	}
}

/*----kernel event -------*/

static void do_proc_created(proto_t *data) {
	proto_read_int(data); //read father pid
	int cpid = proto_read_int(data);
	proto_reset(data);

	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		ipc_call(pid, VFS_PROC_CLONE, data, NULL);
	}

	pid = get_ipc_serv(IPC_SERV_PROC);
	if(pid > 0) {
		ipc_call(pid, PROC_CMD_CLONE, data, NULL);
	}

	proc_wakeup(cpid);
}

static void do_proc_exit(proto_t *data) {
	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		ipc_call(pid, VFS_PROC_EXIT, data, NULL);
	}

	pid = get_ipc_serv(IPC_SERV_PROC);
	if(pid > 0) {
		ipc_call(pid, PROC_CMD_EXIT, data, NULL);
	}
}

static void do_usint_ps2_key(proto_t* data) {
	int32_t key_scode = proto_read_int(data);
	int32_t pid = get_ipc_serv(IPC_SERV_PS2_KEYB);
	if(pid < 0)
		return;

	proto_t in;
	PF->init(&in)->addi(&in, key_scode);
	ipc_call(pid, FS_CMD_INTERRUPT, &in, NULL);
	PF->clear(&in);
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
	switch(kev->type) {
	case KEV_PROC_EXIT:
		do_proc_exit(kev->data);
		return;
	case KEV_US_INT:
		do_user_space_int(kev->data);
		return;
	case KEV_PROC_CREATED:
		do_proc_created(kev->data);
		return;
	}
}

void core(void) {
	_global = hashmap_new();
	_ipc_servs = hashmap_new();

	ipc_serv_run(handle_ipc, NULL, IPC_NON_BLOCK);
	syscall0(SYS_CORE_READY);

	while(1) {
		kevent_t* kev = (kevent_t*)syscall0(SYS_GET_KEVENT);
		if(kev != NULL) {
			handle_event(kev);
			if(kev->data != NULL)
				proto_free(kev->data);
			free(kev);
		}
		usleep(10000);
	}

	hashmap_free(_global);
	hashmap_free(_ipc_servs);
}

