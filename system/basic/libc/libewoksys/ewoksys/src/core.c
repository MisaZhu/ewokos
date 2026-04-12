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

int core_set_env(const char* name, const char* value) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, name)->adds(&in, value);

	int res = ipc_call(get_cored_pid(), CORE_CMD_SET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) != 0) {
			res = -1;
		}
	}
	else {
		res = -1;
	}
	PF->clear(&out);
	return res;
}

#ifdef __cplusplus
}
#endif

