#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define MUTEX_INITIALIZER  0
typedef uint32_t mutex_t;



#define memory_alloc(x)		calloc(1, x)
#define memory_free(x)		free(x)

#define flockfile(x)	do{}while(0)
#define funlockfile(x)	do{}while(0)


# define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))

#define timersub(s,t,a) (void) ( (a)->tv_sec = (s)->tv_sec - (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec - (t)->tv_usec) < 0 && \
	((a)->tv_usec += 1000000, (a)->tv_sec--) )

#define timerclear(t) ((t)->tv_sec = (t)->tv_usec = 0)

struct sched_ctx {
	int cond;
    int interrupted;
    int wc; /* wait count */
};

#define gettimeofday gettimeofday_plat

void raise_softirq(uint32_t sig);

#define EINTR -1

#define SIGIRQ    0x0
#define SIGNET    0x1
#define SIGINT    0x2
#define SIGSHARE  0x3
#define SIGALRM   0x4
#define SIGMAX    0x5


extern int dflag[16];
extern int dcnt;
#define TRACE()     do{dflag[dcnt%(sizeof(dflag)/sizeof(int))] = __LINE__; dcnt++;}while(0)


#if 0
// #define mutex_lock(x)	      do{TRACE();pthread_mutex_lock(x);}while(0)	
// #define mutex_unlock(x)     do{TRACE();pthread_mutex_unlock(x);}while(0)	
#define mutex_lock(x)	      do{klog("%s %d %08x lock\n", __func__, __LINE__, x);pthread_mutex_lock(x);klog("%s %d %08x enter\n", __func__, __LINE__, x);}while(0)	
#define mutex_unlock(x)     do{klog("%s %d %08x unlock\n", __func__, __LINE__, x);pthread_mutex_unlock(x);}while(0)	
#else
#define mutex_lock(x)	pthread_mutex_lock(x)
#define mutex_unlock(x) pthread_mutex_unlock(x)
#endif
#endif
