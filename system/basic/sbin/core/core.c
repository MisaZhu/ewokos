#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/proc.h>
#include <kevent.h>
#include <procinfo.h>
#include <ewoksys/hashmap.h>

typedef struct {
	str_t* cwd;	
	map_t* envs;
} proc_info_t;

static proc_info_t _proc_info_table[PROC_MAX];

static void core_init(void) {
	int32_t i;

	for(i = 0; i<PROC_MAX; i++) {
		_proc_info_table[i].cwd = str_new("/");
		_proc_info_table[i].envs = hashmap_new();
	}
}

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

static void do_proc_get_cwd(int pid, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;
	PF->clear(out)->addi(out, 0)->adds(out, CS(_proc_info_table[pid].cwd));
}

static int get_fsinfo_by_name(const char* fname, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in)->adds(&in, fname);
	PF->init(&out);
	int pid = get_ipc_serv(IPC_SERV_VFS);
	int res = ipc_call(pid, VFS_GET_BY_NAME, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res != 0) {
			if(info != NULL)
				proto_read_to(&out, info, sizeof(fsinfo_t));
			res = 0;
		}
		else
			res = -1;
	}
	PF->clear(&out);
	return res;	
}

static void do_proc_set_cwd(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;

	const char* s = proto_read_str(in);
	fsinfo_t info;
	if(get_fsinfo_by_name(s, &info) != 0) {
		PF->addi(out, ENOENT);
		return;
	}
	
	if(vfs_check_access(pid, &info, X_OK) != 0) {
		PF->addi(out, EPERM);
		return;
	}

	str_cpy(_proc_info_table[pid].cwd, s);
	PF->clear(out)->addi(out, 0);
}

static str_t* env_get(map_t* envs, const char* key) {
	str_t* ret = NULL;
	if(hashmap_get(envs, key, (void**)&ret) == MAP_OK) {
		return ret;
	}
	return NULL;
}

static void set_env(map_t* envs, const char* key, const char* val) {
	str_t* v = env_get(envs, key);
	if(v != NULL) {
		str_cpy(v, val);
	}
	else {
		v = str_new(val);
		hashmap_put(envs, key, v);
	}
}

static void do_proc_set_env(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;
	const char* key = proto_read_str(in);
	const char* val = proto_read_str(in);

	set_env(_proc_info_table[pid].envs, key, val);	
	PF->clear(out)->addi(out, 0);
}

static int get_envs(const char* key, any_t data, any_t arg) {
	proto_t* out = (proto_t*)arg;
	str_t* v = (str_t*)data;

	PF->adds(out, key)->adds(out, v->cstr);
	return MAP_OK;
}

static void do_proc_get_envs(int pid, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;

	PF->clear(out)->addi(out, hashmap_length(_proc_info_table[pid].envs));
	hashmap_iterate(_proc_info_table[pid].envs, get_envs, out);
}

static void do_proc_get_env(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;
	const char* key = proto_read_str(in);
	str_t* v = env_get(_proc_info_table[pid].envs, key);
	if(v == NULL) {
		return;
	}
	PF->clear(out)->addi(out, 0)->adds(out, v->cstr);
}

static int copy_envs(const char* key, any_t data, any_t arg) {
	map_t* to = (map_t*)arg;
	str_t* v = (str_t*)data;
	set_env(to, key, v->cstr);
	return MAP_OK;
}

static int free_envs(const char* key, any_t data, any_t arg) {
	map_t* map = (map_t*)arg;
	str_t* v = (str_t*)data;
	hashmap_remove(map, key);
	str_free(v);
	return MAP_OK;
}

static void do_proc_clone(proto_t* in) {
	int fpid = proto_read_int(in);
	int cpid = proto_read_int(in);
	if(fpid < 0 || fpid >= PROC_MAX ||
			cpid < 0 || cpid >= PROC_MAX)
		return;
	str_cpy(_proc_info_table[cpid].cwd, CS(_proc_info_table[fpid].cwd));	
	hashmap_iterate(_proc_info_table[cpid].envs, free_envs, _proc_info_table[cpid].envs);	
	hashmap_free(_proc_info_table[cpid].envs);
	_proc_info_table[cpid].envs = hashmap_new();
	hashmap_iterate(_proc_info_table[fpid].envs, copy_envs, _proc_info_table[cpid].envs);	
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	pid = proc_getpid(pid);

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
	case CORE_CMD_SET_CWD:
		do_proc_set_cwd(pid, in, out);
		return;
	case CORE_CMD_GET_CWD:
		do_proc_get_cwd(pid, out);
		return;
	case CORE_CMD_SET_ENV:
		do_proc_set_env(pid, in, out);
		return;
	case CORE_CMD_GET_ENV:
		do_proc_get_env(pid, in, out);
		return;
	case CORE_CMD_GET_ENVS:
		do_proc_get_envs(pid, out);
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
		ipc_call_wait(pid, VFS_PROC_CLONE, &data);
	}

	do_proc_clone(&data);
	PF->clear(&data);
	proc_wakeup(cpid);
}

static void do_proc_exit(kevent_t* kev) {
	proto_t data;
	PF->init(&data)->addi(&data, kev->data[0]);
	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		ipc_call_wait(pid, VFS_PROC_EXIT, &data);
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

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	core_init();
	_ipc_servs = hashmap_new();

	ipc_serv_run(handle_ipc, NULL, NULL, IPC_NON_BLOCK);
	syscall0(SYS_CORE_READY);

	while(1) {
		kevent_t kev;
		if(syscall1(SYS_GET_KEVENT, (int32_t)&kev) == 0) {
			ipc_disable();
			handle_event(&kev);
			ipc_enable();
		}
		else
			usleep(2000);
	}

	hashmap_free(_ipc_servs);
	return 0;
}

