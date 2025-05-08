#ifndef SVC_H
#define SVC_H

#include <kernel/context.h>
#include <syscalls.h>
#include <ewokos_config.h>

extern void svc_handler(int32_t code, ewok_addr_t arg0, ewok_addr_t arg1, ewok_addr_t arg2, context_t* ctx);

#endif
