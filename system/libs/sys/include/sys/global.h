#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/proto.h>

int set_global(const char* key, proto_t* in);
int get_global(const char* key, proto_t* out);

#endif
