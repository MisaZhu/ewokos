#ifndef KERNEL_SERV_H
#define KERNEL_SERV_H

#include <types.h>
#include <proc.h>
#include <lib/kservtypes.h>

bool kservReg(KSNameT name);
void kservUnreg(ProcessT* proc);
int kservWrite(KSNameT name, char* p, uint32_t size);
int kservRead(KSNameT name, char* p, uint32_t size);


#endif
