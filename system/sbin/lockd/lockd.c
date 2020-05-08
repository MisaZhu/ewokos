#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/proto.h>
#include <sys/proc.h>
#include <sys/lockc.h>
#include <sys/syscall.h>
#include <sys/kserv.h>

static void do_lockd_new(proto_t* out) {
	int32_t *lock =  (int32_t*)malloc(sizeof(int32_t));
	*lock = 0;
	PF->addi(out, (uint32_t)lock);
}

static void do_lockd_free(proto_t* in) {
	int32_t* lock = (int32_t*)proto_read_int(in);
	if(lock != NULL)
		free(lock);
}

static void do_lockd_lock(proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	int32_t* lock = (int32_t*)proto_read_int(in);
	if(lock == NULL || *lock != 0) { //locked already , retry
		return;	
	}

	*lock = 1;
	PF->clear(out)->addi(out, 0);
}

static void do_lockd_unlock(proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	int32_t* lock = (int32_t*)proto_read_int(in);
	if(lock == NULL) { 
		return;	
	}

	*lock = 0;
	PF->clear(out)->addi(out, 0);
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)pid;
	(void)p;
	(void)out;

	switch(cmd) {
	case LOCK_CMD_NEW: 
		do_lockd_new(out);
		return;
	case LOCK_CMD_FREE: 
		do_lockd_free(in);
		return;
	case LOCK_CMD_LOCK: 
		do_lockd_lock(in, out);
		return;
	case LOCK_CMD_UNLOCK: 
		do_lockd_unlock(in, out);
		return;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(kserv_reg(KSERV_LOCK) != 0) {
		kprintf(false, "reg lock kserv error!\n");
		return -1;
	}
	
	kserv_run(handle_ipc, NULL, false);

	while(1) {
		sleep(1);
	}
	return 0;
}
