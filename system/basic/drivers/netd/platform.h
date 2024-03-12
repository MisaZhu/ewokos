#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define MUTEX_INITIALIZER  0
typedef uint32_t mutex_t;

#define memory_alloc(x)		calloc(1, x)
#define memory_free(x)		free(x)

#define mutex_lock(x)	do{klog("%s lock\n", __func__);pthread_mutex_lock(x);}while(0)	
#define mutex_unlock(x) do{klog("%s unlock\n", __func__);pthread_mutex_unlock(x);}while(0)	


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

#endif
