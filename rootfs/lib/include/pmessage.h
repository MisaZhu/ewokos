#ifndef PMESSAGE_H
#define PMESSAGE_H

#include <types.h>
#include <package.h>

int popen(int pid, uint32_t bufsize);

void pclose(int id);

int psend(int id, uint32_t type, void* data, uint32_t size);

PackageT* precvPkg(int id);

PackageT* preq(int pid, uint32_t bufSize, uint32_t type, void* data, uint32_t size, bool reply);

PackageT* proll();

#endif
