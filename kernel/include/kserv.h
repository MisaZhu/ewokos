#ifndef KSERV_H
#define KSERV_H

#define KSERV_NAME_MAX 16

typedef struct {
	char name[KSERV_NAME_MAX+1];
	int pid;
} kserv_t;

int kserv_reg(const char* name); 

int kserv_get(const char* name);

#endif
