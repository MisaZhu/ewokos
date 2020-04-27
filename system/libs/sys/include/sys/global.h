#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/proto.h>

const char* KSERV_VFS_PID = "kserv.vfs.pid";
const char* KSERV_PS2_KEYB_PID = "kserv.ps2keyb.pid";

int set_global(const char* key, proto_t* in);
proto_t* get_global(const char* key);

#endif
