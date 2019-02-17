#ifndef PMESSAGE_H
#define PMESSAGE_H

#include <types.h>
#include <package.h>

PackageT* newPackage(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid);

void freePackage(PackageT* pkg);

int popen(int pid);

void pclose(int id);

int pwrite(int id, void* data, uint32_t size);

int pread(int id, void* data, uint32_t size);

int psend(int id, uint32_t type, void* data, uint32_t size);

int pgetPidR(int id);

int pgetPidW(int id);

void* precv(int id, uint32_t *type, uint32_t *size);

PackageT* preq(int pid, uint32_t type, void* data, uint32_t size, bool reply);

PackageT* proll();

#endif
