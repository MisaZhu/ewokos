#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <kernel/context.h>

#define SCHD_LOCK_LIMIT 10000 //10msec
extern int32_t schedule(context_t* ctx);

#endif
