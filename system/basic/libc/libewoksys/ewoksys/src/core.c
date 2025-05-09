#include <ewoksys/core.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setenv.h>

#define ENV_UX_ID "UX_ID"

#ifdef __cplusplus
extern "C" {
#endif

int core_get_ux(void) {
	int ret = 0;
	const char* uxid = getenv(ENV_UX_ID);
	if(uxid != NULL) {
		ret = atoi(uxid);
	}
	return ret;
}

int core_set_ux(int ux_index) {
	char uxid[8] = {0};
	snprintf(uxid, 7, "%d", ux_index);

	proto_t in;
	PF->init(&in)->addi(&in, ux_index);
	int res = ipc_call_wait(get_cored_pid(), CORE_CMD_SET_UX, &in);
	PF->clear(&in);

	setenv(ENV_UX_ID, uxid);
	return res;
}

int core_get_active_ux(void) {
	proto_t out;
	PF->init(&out);
	int res = ipc_call(get_cored_pid(), CORE_CMD_GET_UX, NULL, &out);
	if(res == 0)
		res = proto_read_int(&out);
	PF->clear(&out);
	return res;
}

int core_set_active_ux(int ux_index) {
	proto_t in;
	PF->init(&in)->addi(&in, ux_index);
	int res = ipc_call_wait(get_cored_pid(), CORE_CMD_SET_ACTIVE_UX, &in);
	PF->clear(&in);
	return res;
}

int core_next_ux(void) {
	int res = ipc_call_wait(get_cored_pid(), CORE_CMD_NEXT_UX, NULL);
	return res;
}

int core_prev_ux(void) {
	int res = ipc_call_wait(get_cored_pid(), CORE_CMD_PREV_UX, NULL);
	return res;
}

#ifdef __cplusplus
}
#endif

