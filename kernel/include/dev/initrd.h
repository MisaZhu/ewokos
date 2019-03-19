#ifndef INITRD_H
#define INITRD_H

#include <types.h>

int32_t loadInitRD();
void closeInitRD();
const char* readInitRD(const char* name, int32_t *size);
void* cloneInitRD();

#endif
