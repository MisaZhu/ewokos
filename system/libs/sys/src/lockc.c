#include <sys/core.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <unistd.h>

uint32_t lock_new(void) {
	proto_t out;
	PF->init(&out, NULL, 0);
	int res = ipc_call(CORED_PID, CORE_CMD_LOCK_NEW, NULL, &out);
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
	int res = ipc_call(CORED_PID, CORE_CMD_LOCK_FREE, &in, NULL);
	PF->clear(&in);
	if(res != 0)
		res = -1;
	return res;
}

static int lock_lock_raw(uint32_t lock) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->addi(&in, lock);
	int res = ipc_call(CORED_PID, CORE_CMD_LOCK, &in, &out);
	PF->clear(&in);
	if(res == 0)
		res = proto_read_int(&out);
	else
		res = -1;
	PF->clear(&out);
	return res;
}

int lock_lock(uint32_t lock) {
	while(1) {
		if(lock_lock_raw(lock) == 0)
			break;
		sleep(0);
	}
	return 0;
}

int lock_unlock(uint32_t lock) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->addi(&in, lock);
	int res = ipc_call(CORED_PID, CORE_CMD_UNLOCK, &in, &out);
	PF->clear(&in);
	if(res == 0)
		res = proto_read_int(&out);
	else
		res = -1;
	PF->clear(&out);
	return res;
}
