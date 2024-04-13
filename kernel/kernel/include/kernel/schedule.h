#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <kernel/context.h>
#include <sysinfo.h>

#define SCHD_CORE_LOCK_LIMIT 5000 //5msec
extern int32_t schedule(context_t* ctx);

#endif
