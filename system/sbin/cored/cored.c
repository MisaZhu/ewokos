#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <fsinfo.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/kserv.h>
#include <sys/core.h>
#include <hashmap.h>
#include <kevent.h>
#include <usinterrupt.h>

static map_t* _global = NULL;
static map_t* _kservs = NULL; //pids of kservers

static proto_t* global_get(const char* key) {
	proto_t* ret;
	hashmap_get(_global, (char*)key, (void**)&ret);
	return ret;
}

static proto_t* global_set(const char* key, void* data, uint32_t size) {
	proto_t* v = global_get(key);
	if(v != NULL) {
		proto_copy(v, data, size);
	}
	else {
		v = proto_new(data, size);
		hashmap_put(_global, (char*)key, v);
	}
	return v;
}

static void global_del(const char* key) {
	proto_t* v = global_get(key);
	hashmap_remove(_global, (char*)key);
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
		proto_copy(out, v->data, v->size);
}

static void do_global_del(proto_t* in) {
	const char* key = proto_read_str(in);
	global_del(key);
}

/*----------------------------------------------------------*/

static int get_kserv(const char* key) {
	int32_t *v;
	if(hashmap_get(_kservs, (char*)key, (void**)&v) == MAP_MISSING) {
		return -1;
	}
	return *v;
}

static void do_kserv_get(proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		proto_add_int(out, -1);
		return;
	}
	proto_add_int(out, get_kserv(ks_id));
}

static void do_kserv_reg(int pid, proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		proto_add_int(out, -1);
		return;
	}

	int32_t* v = (int32_t*)malloc(sizeof(int32_t));
	*v = pid;
	if(hashmap_put(_kservs, (char*)ks_id, v) != MAP_OK) {
		proto_add_int(out, -1);
		return;
	}
	proto_add_int(out, 0);
}

static void do_kserv_unreg(int pid, proto_t* in, proto_t* out) {
	const char* ks_id = proto_read_str(in);
	if(ks_id[0] == 0) {
		proto_add_int(out, -1);
		return;
	}

	int32_t *v;
	if(hashmap_get(_kservs, (char*)ks_id, (void**)&v) == MAP_MISSING) {
		proto_add_int(out, -1);
		return;
	}

	if(*v != pid) {
		proto_add_int(out, -1);
		return;
	}

	hashmap_remove(_kservs, (char*)ks_id);
	free(v);
	proto_add_int(out, 0);
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;

	switch(cmd) {
	case CORE_CMD_KSERV_REG: //regiester kserver pid
		do_kserv_reg(pid, in, out);
		return;
	case CORE_CMD_KSERV_UNREG: //unregiester kserver pid
		do_kserv_unreg(pid, in, out);
		return;
	case CORE_CMD_KSERV_GET: //get kserver pid
		do_kserv_get(in, out);
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
	int32_t pid = get_kserv(KSERV_PS2_KEYB);
	if(pid < 0)
		return;

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
	switch(kev->type) {
	case KEV_FCLOSED:
		do_fsclosed(kev->data);
		return;
	case KEV_US_INT:
		do_user_space_int(kev->data);
		return;
	case KEV_GLOBAL_SET:
		do_global_set(kev->data);
		return;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_global = hashmap_new();
	_kservs = hashmap_new();
	
	kserv_run(handle_ipc, NULL, true);

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

	hashmap_free(_global);
	hashmap_free(_kservs);
	return 0;
}
