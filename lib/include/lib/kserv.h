#ifndef KSERV_H
#define KSERV_H

#include <types.h>
#include <lib/kservtypes.h>

int kservReg(int serv);
int kservWrite(int serv, char* p, int size);
int kservRead(int serv, char* p, int size);

#endif
