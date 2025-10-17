#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <sysinfo.h>
#include <kevent.h>
#include <procinfo.h>
#include <ewoksys/hashmap.h>

typedef struct {
	str_t* cwd;	
	map_t envs;
} proc_info_t;

static proc_info_t *_proc_info_table = NULL;
static uint32_t _max_proc_table_num = 0;

typedef struct {
	bool occupied;	
	bool actived;
	int32_t disp_index;	
} ux_t;

static ux_t _uxs[UX_MAX];

static void core_init(void) {
	int32_t i;

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	_max_proc_table_num = sysinfo.max_task_num;
	_proc_info_table = (proc_info_t*)malloc(_max_proc_table_num*sizeof(proc_info_t));

	for(i = 0; i<_max_proc_table_num; i++) {
		_proc_info_table[i].cwd = str_new("/");
		_proc_info_table[i].envs = hashmap_new();
	}

	for(i = 0; i<UX_MAX; i++) {
		memset(&_uxs[i], 0, sizeof(ux_t));
	}
}

static map_t _ipc_servs = NULL; //pids of ipc_servers
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
	if(pid < 0 || pid >= _max_proc_table_num)
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
	if(pid < 0 || pid >= _max_proc_table_num)
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

static void do_proc_enable_ux(int pid, proto_t* in) {
	int disp_index = proto_read_int(in);
	int index = proto_read_int(in);
	if(index < 0 || index >= UX_MAX)
		return;
	_uxs[index].occupied = true;
	_uxs[index].disp_index = disp_index;
}

static bool check_disp_index(int32_t disp_index, int32_t i) {
	return (_uxs[i].disp_index < 0 || _uxs[i].disp_index == disp_index) ;
}

static uint32_t get_active_ux(int32_t disp_index) {
	for(int i = 0; i<UX_MAX; i++) {
		if(_uxs[i].occupied &&
				check_disp_index(disp_index, i) &&
				_uxs[i].actived)
			return i;
	}
	return 0;
}

static void do_proc_next_ux(int32_t disp_index) {
	int32_t ux_index = get_active_ux(disp_index);
	int i = ux_index + 1;
	while(i != ux_index) {
		if(i >= UX_MAX)
			i = 0;
		if(_uxs[i].occupied && check_disp_index(disp_index, i)) {
			_uxs[ux_index].actived = false;
			_uxs[i].actived = true;
			return;
		}
		i++;
	}
}

static void do_proc_prev_ux(int32_t disp_index) {
	int32_t ux_index = get_active_ux(disp_index);
	int i = ux_index - 1;
	while(i != ux_index) {
		if(i < 0)
			i = UX_MAX - 1;
		if(_uxs[i].occupied && check_disp_index(disp_index, i)) {
			_uxs[ux_index].actived = false;
			_uxs[i].actived = true;
			return;
		}
		i--;
	}
}

static void do_proc_set_active_ux(int pid, proto_t* in) {
	int disp_index = proto_read_int(in);
	int index = proto_read_int(in);
	if(index < 0 || index >= UX_MAX)
		return;

	if(_uxs[index].occupied && check_disp_index(disp_index, index)) {
		int32_t ux_index = get_active_ux(disp_index);
		_uxs[ux_index].actived = false;
		_uxs[index].actived = true;
	}
}

static void do_proc_get_ux(int pid, proto_t* in, proto_t* out) {
	int disp_index = proto_read_int(in);
	int32_t ux_index = get_active_ux(disp_index);
	PF->addi(out, ux_index);
}

static str_t* env_get(map_t envs, const char* key) {
	str_t* ret = NULL;
	if(hashmap_get(envs, key, (void**)&ret) == MAP_OK) {
		return ret;
	}
	return NULL;
}

static void set_env(map_t envs, const char* key, const char* val) {
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
	if(pid < 0 || pid >= _max_proc_table_num)
		return;
	const char* key = proto_read_str(in);
	const char* val = proto_read_str(in);

	set_env(_proc_info_table[pid].envs, key, val);	
	PF->clear(out)->addi(out, 0);
}

static int get_envs(map_t map, const char* key, any_t data, any_t arg) {
	proto_t* out = (proto_t*)arg;
	str_t* v = (str_t*)data;

	PF->adds(out, key)->adds(out, v->cstr);
	return MAP_OK;
}

static void do_proc_get_envs(int pid, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= _max_proc_table_num)
		return;

	PF->clear(out)->addi(out, hashmap_length(_proc_info_table[pid].envs));
	hashmap_iterate(_proc_info_table[pid].envs, get_envs, out);
}

static void do_proc_get_env(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= _max_proc_table_num)
		return;
	const char* key = proto_read_str(in);
	str_t* v = env_get(_proc_info_table[pid].envs, key);
	if(v == NULL) {
		return;
	}
	PF->clear(out)->addi(out, 0)->adds(out, v->cstr);
}

static int copy_envs(map_t map, const char* key, any_t data, any_t arg) {
	map_t to = (map_t)arg;
	str_t* v = (str_t*)data;
	set_env(to, key, v->cstr);
	return MAP_OK;
}

static int free_envs(map_t map, const char* key, any_t data, any_t arg) {
	str_t* v = (str_t*)data;
	hashmap_remove(map, key);
	str_free(v);
	return MAP_OK;
}

static void do_proc_clone(int fpid, int cpid) {
	if(fpid < 0 || fpid >= _max_proc_table_num ||
			cpid < 0 || cpid >= _max_proc_table_num)
		return;
	str_cpy(_proc_info_table[cpid].cwd, CS(_proc_info_table[fpid].cwd));	
	hashmap_iterate(_proc_info_table[cpid].envs, free_envs, NULL);	
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
	case CORE_CMD_SET_ACTIVE_UX:
		do_proc_set_active_ux(pid, in);
		return;
	case CORE_CMD_ENABLE_UX:
		do_proc_enable_ux(pid, in);
		return;
	case CORE_CMD_NEXT_UX:
		do_proc_next_ux((uint32_t)in);
		return;
	case CORE_CMD_PREV_UX:
		do_proc_prev_ux((uint32_t)in);
		return;
	case CORE_CMD_GET_UX:
		do_proc_get_ux(pid, in, out);
		return;
	}
}

/*----kernel event -------*/

static void do_proc_created(kevent_t* kev) {
	int fpid = kev->data[0];
	int cpid = kev->data[1];

	int pid = get_ipc_serv(IPC_SERV_VFS);
	if(pid > 0) {
		proto_t data;
		PF->init(&data)->addi(&data, fpid)->addi(&data, cpid);
		int res = ipc_call_wait(pid, VFS_PROC_CLONE, &data);
		if(res == 0) {
			PF->clear(&data);
			do_proc_clone(fpid, cpid);
			proc_wakeup_pid(cpid, cpid);
			proc_wakeup_pid(fpid, cpid);
		}
	}
	else {
		do_proc_clone(fpid, cpid);
		proc_wakeup_pid(cpid, cpid);
		proc_wakeup_pid(fpid, cpid);
	}
}

static void do_proc_exit(kevent_t* kev) {
	int pid = kev->data[0];
	proto_t data;
	PF->init(&data)->addi(&data, pid);
	int vfs_pid = get_ipc_serv(IPC_SERV_VFS);
	if(vfs_pid > 0) {
		ipc_call_wait(vfs_pid, VFS_PROC_EXIT, &data);
	}

	if(_proc_info_table[pid].envs != NULL) {
		hashmap_iterate(_proc_info_table[pid].envs, free_envs, NULL);	
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
		if(syscall1(SYS_GET_KEVENT, (ewokos_addr_t)&kev) == 0) {
			ipc_disable();
			handle_event(&kev);
			ipc_enable();
		}
		else
			proc_usleep(50000);
	}

	hashmap_free(_ipc_servs);
	free(_proc_info_table);
	return 0;
}

