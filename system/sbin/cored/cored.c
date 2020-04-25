#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <sys/kserv.h>
#include <sys/core.h>

static int _kservs[KSERV_MAX]; //pids of kservers

static void do_reg(int pid, proto_t* in, proto_t* out) {
	int ks_id = proto_read_int(in);
	if(ks_id < 0 || ks_id >= KSERV_MAX || _kservs[ks_id] >= 0) {
		proto_add_int(out, -1);
		return;
	}
	_kservs[ks_id] = pid;
	proto_add_int(out, 0);
}

static void do_get(proto_t* in, proto_t* out) {
	int ks_id = proto_read_int(in);
	if(ks_id < 0 || ks_id >= KSERV_MAX || _kservs[ks_id] < 0) {
		proto_add_int(out, -1);
		return;
	}
	proto_add_int(out, _kservs[ks_id]);
}

static void do_unreg(int pid, proto_t* in, proto_t* out) {
	int ks_id = proto_read_int(in);
	if(ks_id < 0 || ks_id >= KSERV_MAX ||
			_kservs[ks_id] < 0 ||
			_kservs[ks_id] != pid) {
		proto_add_int(out, -1);
		return;
	}
	proto_add_int(out, 0);
}

static void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;

	if(cmd == CORE_CMD_KSERV_REG) { //regiester kserver pid
		do_reg(pid, in, out);
	}
	else if(cmd == CORE_CMD_KSERV_UNREG) { //unregiester kserver pid
		do_unreg(pid, in, out);
	}
	else if(cmd == CORE_CMD_KSERV_GET) { //get kserver pid
		do_get(in, out);
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	for(int i=0; i<KSERV_MAX; i++) {
		_kservs[i] = -1;
	}
	
	kserv_run(handle, NULL, false);

	while(true) {
		sleep(1);
	}
	return 0;
}
