#ifndef SVC_H
#define SVC_H

#include <kernel/context.h>
#include <syscalls.h>
#include <ewokos_config.h>

extern void svc_handler(int32_t code, ewokos_addr_t arg0, ewokos_addr_t arg1, ewokos_addr_t arg2, context_t* ctx);

#endif
