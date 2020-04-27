#ifndef GLOBAL_H
#define GLOBAL_H

#include <proto.h>

int32_t global_set(const char* key, proto_t* val);
int32_t global_set_int(const char* key, int32_t val);
int32_t global_set_str(const char* key, const char* val);

#endif
