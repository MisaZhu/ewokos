#ifndef KSERV_H
#define KSERV_H

#include "package.h"

typedef void (*kserv_func_t) (package_t* pkg, void *p);

bool kserv_run(const char* reg_name, kserv_func_t servFunc, void* p);

int kserv_get_pid(const char* reg_name);

void kserv_wait(const char* reg_name);

#endif
