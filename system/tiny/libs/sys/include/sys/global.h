#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/proto.h>

int set_global(const char* key, proto_t* in);
proto_t* get_global(const char* key);

#endif
