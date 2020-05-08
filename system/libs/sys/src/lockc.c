#include <sys/lockc.h>
#include <sys/kserv.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <unistd.h>

inline static int get_lockd_pid(void) {
	return kserv_get(KSERV_LOCK);
}

uint32_t lock_new(void) {
	proto_t out;
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_lockd_pid(), LOCK_CMD_NEW, NULL, &out);
	if(res == 0)
		res = proto_read_int(&out);
	else 
		res = 0;
	PF->clear(&out);
	return (uint32_t)res;	
}

int lock_free(uint32_t lock) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, lock);
	int res = ipc_call(get_lockd_pid(), LOCK_CMD_FREE, &in, NULL);
	PF->clear(&in);
	if(res != 0)
		res = -1;
	return res;
}

static int lock_lock_raw(int lockd_pid, uint32_t lock) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->addi(&in, lock);
	int res = ipc_call(lockd_pid, LOCK_CMD_LOCK, &in, &out);
	PF->clear(&in);
	if(res == 0)
		res = proto_read_int(&out);
	else
		res = -1;
	PF->clear(&out);
	return res;
}

int lock_lock(uint32_t lock) {
	int lockd_pid = get_lockd_pid();
	while(1) {
		if(lock_lock_raw(lockd_pid, lock) == 0)
			break;
		sleep(0);
	}
	return 0;
}

int lock_unlock(uint32_t lock) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->addi(&in, lock);
	int res = ipc_call(get_lockd_pid(), LOCK_CMD_UNLOCK, &in, &out);
	PF->clear(&in);
	if(res == 0)
		res = proto_read_int(&out);
	else
		res = -1;
	PF->clear(&out);
	return res;
}
