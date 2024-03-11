#ifndef __LOG_H__
#define __LOG_H__

#include <pthread.h>

#define brcm_klog(...) do{  \
        klog(__VA_ARGS__);                  \
    }while(0)                               

#endif