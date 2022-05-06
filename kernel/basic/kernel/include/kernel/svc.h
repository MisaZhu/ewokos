#ifndef SVC_H
#define SVC_H

#include <kernel/context.h>
#include <syscalls.h>

typedef struct {
	int32_t pid;
	int32_t code;
	int32_t arg0;
	int32_t arg1;
	int32_t arg2;
} svc_t;

extern svc_t _last_svc;
extern void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx);

#endif
