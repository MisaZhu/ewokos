#ifndef KMALLOC_H
#define KMALLOC_H

#include <types.h>

void* kmalloc(uint32_t size);
void kmfree(void* p);
void kmInit();

#endif
