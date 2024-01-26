#include <stdlib.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/core.h>
#include <string.h>
#include <stdio.h>

const char* getenv(const char* name) {
	static char ret[1024];
	ret[0] = 0;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, name);

	int res = ipc_call(get_cored_pid(), CORE_CMD_GET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) == 0) {
			sstrncpy(ret, proto_read_str(&out), 1023);
		}
	}
	PF->clear(&out);
	return ret;
}

