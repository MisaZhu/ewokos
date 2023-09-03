#ifndef XEVT_POOL_H 
#define XEVT_POOL_H 

#include <x/xevent.h>

void xevent_push(int pid, xevent_t* evt);
bool xevent_pop(int pid, xevent_t* evt);
void xevent_remove(int pid);
void xevent_pool_init(void);

#endif