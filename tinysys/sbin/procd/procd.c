#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <sys/kserv.h>
#include <mstr.h>
#include <sys/proto.h>
#include <sys/kserv.h>
#include <sys/syscall.h>
#include <procinfo.h>
#include <hashmap.h>

typedef struct {
	str_t* cwd;	
	map_t* envs;
} proc_info_t;

static proc_info_t _proc_info_table[PROC_MAX];

static void procd_init(void) {
	int32_t i;

	for(i = 0; i<PROC_MAX; i++) {
		_proc_info_table[i].cwd = str_new("/");
		_proc_info_table[i].envs = hashmap_new();
	}
}

static void do_proc_get_cwd(int pid, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= PROC_MAX)
		return;
	PF->clear(out)->addi(out, 0)->adds(out, CS(_proc_info_table[pid].cwd));
}

static void do_proc_set_cwd(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	const char* s = proto_read_str(in);
	if(pid < 0 || pid >= PROC_MAX)
		return;
	str_cpy(_proc_info_table[pid].cwd, s);
	PF->clear(out)->addi(out, 0);
}

static proto_t* env_get(map_t* envs, const char* key) {
	proto_t* ret = NULL;
	if(hashmap_get(envs, key, (void**)&ret) == MAP_OK)
		return ret;
	return NULL;
}

static void set_env(map_t* envs, const char* key, const char* val) {
	proto_t* v = env_get(envs, key);
	if(v != NULL) {
		PF->clear(v)->adds(v, val);
	}
	else {
		v = proto_new(NULL, 0);
		PF->adds(v, val);
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
	proto_t* v = (proto_t*)data;
	const char* vs = proto_read_str(v);

	PF->adds(out, key)->adds(out, vs);
	proto_reset(v);
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

	proto_t* v = env_get(_proc_info_table[pid].envs, key);
	if(v == NULL) {
		return;
	}
	PF->clear(out)->addi(out, 0)->adds(out, proto_read_str(v));
	proto_reset(v);
}

static int copy_envs(const char* key, any_t data, any_t arg) {
	map_t* to = (map_t*)arg;
	proto_t* d = (proto_t*)data;
	const char* v = proto_read_str(d);
	set_env(to, key, v);
	proto_reset(d);
	return MAP_OK;
}

static int free_envs(const char* key, any_t data, any_t arg) {
	if(key == NULL)
		return MAP_OK;

	map_t* map = (map_t*)arg;
	proto_t* d = (proto_t*)data;
	hashmap_remove(map, key);
	proto_free(d);
	return MAP_OK;
}

static void do_proc_clone(int32_t pid, proto_t* in) {
	(void)pid;
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

static void do_proc_exit(int32_t pid, proto_t* in) {
	(void)pid;
	int cpid = proto_read_int(in);
	if(cpid < 0)
		return;
	
	if(cpid < 0 || cpid >= PROC_MAX)
		return;
}

static void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	pid = getpid_raw(pid);

	switch(cmd) {
	case PROC_CMD_SET_CWD:
		do_proc_set_cwd(pid, in, out);
		break;
	case PROC_CMD_GET_CWD:
		do_proc_get_cwd(pid, out);
		break;
	case PROC_CMD_SET_ENV:
		do_proc_set_env(pid, in, out);
		break;
	case PROC_CMD_GET_ENV:
		do_proc_get_env(pid, in, out);
		break;
	case PROC_CMD_GET_ENVS:
		do_proc_get_envs(pid, out);
		break;
	case PROC_CMD_CLONE:
		do_proc_clone(pid, in);
		break;
	case PROC_CMD_EXIT:
		do_proc_exit(pid, in);
		break;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	procd_init();

	if(kserv_reg(KSERV_PROC) != 0) {
		kprintf(false, "reg proc kserv error!\n");
		return -1;
	}

	kserv_run(handle, NULL, false);

	while(true) {
		sleep(1);
	}
	return 0;
}
