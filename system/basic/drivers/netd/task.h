#ifndef __NET_TASK_H__
#define __NET_TASK_H__

#include <ewoksys/ipc.h>
#include <pthread.h>

enum{
    NET_TASK_IDLE,
    NET_TASK_START,
    NET_TASK_PROCESS,
    NET_TASK_FINISH
};

typedef struct net_task{
    int fd;
	int from_pid;
	int node;
    int cmd;
    pthread_t tid;
    char* iobuf;
    char* inbuf;
    char* outbuf;
	proto_t in;
	proto_t out;
	void *p;
    bool running;
    int state;
    int sock;
    char *buf;
    struct net_task *next;
}net_task_t;

net_task_t *create_task(int fd, int from_pid, int node);
void start_task(void);
void release_task(net_task_t *task);
int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p);
int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p);
int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p);

#endif