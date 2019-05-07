#ifndef KSERV_H
#define KSERV_H

#include <proc.h>

#define KSERV_NAME_MAX 16

enum {
	KSERV_NONE = 0,
	KSERV_BUSY,
	KSERV_READY
};

typedef struct {
	char name[KSERV_NAME_MAX+1];
	int32_t pid;
	uint32_t state;
} kserv_t;

int32_t kserv_reg(const char* name); 

int32_t kserv_ready(void);

int32_t kserv_unreg(process_t* proc); 

//if service not exist or not ready , will return -1
int32_t kserv_get_by_name(const char* name);
int32_t kserv_get_by_pid(int32_t pid);

#endif
