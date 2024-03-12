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
}net_task_t;

#endif