#ifndef PMESSAGE_H
#define PMESSAGE_H

#include <package.h>

int psend(int id, int pid, uint32_t type, void* data, uint32_t size);

int psendSync(int id, int pid, uint32_t type, void* data, uint32_t size);

PackageT* precv(int id);

PackageT* precvSync(int id);

PackageT* preq(int pid, uint32_t type, void* data, uint32_t size);

#endif
