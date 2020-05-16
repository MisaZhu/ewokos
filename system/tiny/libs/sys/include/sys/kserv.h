#ifndef KSERV_H
#define KSERV_H

#include <sys/proto.h>
#include <stdbool.h>

#define KSERV_VFS      "kserv.vfs"
#define KSERV_PS2_KEYB "kserv.ps2keyb"
#define KSERV_LOCK     "kserv.lock"
#define KSERV_PROC     "kserv.proc"

int kserv_reg(const char* kserv_id);
int kserv_unreg(const char* kserv_id);
int kserv_get(const char* kserv_id);

typedef void (*kserv_handle_t)(int from_pid, int cmd, proto_t* in, proto_t* out, void* p);
void kserv_run(kserv_handle_t handle, void* p, bool prefork);

#endif
