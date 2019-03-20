#ifndef INITRD_H
#define INITRD_H

#include <types.h>

int32_t load_initrd();
void close_initrd();
const char* read_initrd(const char* name, int32_t *size);
void* clone_initrd();

#endif
