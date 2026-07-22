#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/core.h>

char* getcwd(char* buf, size_t size) {
	const char* cwd;

	if(buf == NULL || size == 0) {
		errno = EINVAL;
		return NULL;
	}

	memset(buf, 0, size);
	proto_t out;
	PF->init(&out);
	if(ipc_call(get_cored_pid(), CORE_CMD_GET_CWD, NULL, &out) != 0) {
		PF->clear(&out);
		if(errno == ENONE)
			errno = EIO;
		return NULL;
	}

	if(proto_read_int(&out) != 0) {
		PF->clear(&out);
		if(errno == ENONE)
			errno = EIO;
		return NULL;
	}

	cwd = proto_read_str(&out);
	if(cwd == NULL) {
		PF->clear(&out);
		errno = EIO;
		return NULL;
	}

	if(strlen(cwd) >= size) {
		PF->clear(&out);
		errno = ERANGE;
		return NULL;
	}

	strncpy(buf, cwd, size-1);
	PF->clear(&out);
	return buf;
}
