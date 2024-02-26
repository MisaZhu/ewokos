#ifndef __LOG_H__
#define __LOG_H__

extern int _spinlock; 
extern int _locktimeout;

#define brcm_klog(...) do{  \
        _locktimeout = 10;  \
        while(_spinlock && _locktimeout--){usleep(1);};   \
        _spinlock = 1;               \
        klog(__VA_ARGS__);           \
        _spinlock = 0;               \
    }while(0)                        \

#endif