#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif


int set_global(const char* key, proto_t* in);
int get_global(const char* key, proto_t* out);

int set_global_str(const char* key, const char* s);
const char* get_global_str(const char* key);

#ifdef __cplusplus
}
#endif

#endif
