#ifndef KSERV_H
#define KSERV_H

#include "package.h"

typedef void (*KServFuncT) (PackageT* pkg, void *p);

bool kservRun(const char* regName, KServFuncT servFunc, void* p);

#endif
