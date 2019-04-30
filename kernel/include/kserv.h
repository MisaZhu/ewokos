#ifndef KSERV_H
#define KSERV_H

#include <proc.h>

#define KSERV_NAME_MAX 16

typedef struct {
	char name[KSERV_NAME_MAX+1];
	int32_t pid;
} kserv_t;

int32_t kserv_reg(const char* name); 

int32_t kserv_unreg(process_t* proc); 

int32_t kserv_get(const char* name);

#endif
