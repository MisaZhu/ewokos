#ifndef KSERV_H
#define KSERV_H

#define KSERV_NAME_MAX 16

typedef struct {
	char name[KSERV_NAME_MAX+1];
	int pid;
} KServT;

int kservReg(const char* name); 

int kservGet(const char* name);

#endif
