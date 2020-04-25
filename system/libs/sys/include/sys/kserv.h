#ifndef KSERV_H
#define KSERV_H

#include <sys/proto.h>
#include <stdbool.h>

enum {
	KSERV_VFS = 0,
	KSERV_MAX
};

enum {
	KSERV_CMD_REG = 0,
	KSERV_CMD_UNREG,
	KSERV_CMD_GET
};

int kserv_reg(int kserv_id);
int kserv_unreg(int kserv_id);
int kserv_get(int kserv_id);

typedef void (*kserv_handle_t)(int from_pid, int cmd, proto_t* in, proto_t* out, void* p);

void kserv_run(kserv_handle_t handle, void* p, bool prefork);

#endif
