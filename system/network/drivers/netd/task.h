#ifndef __NET_TASK_H__
#define __NET_TASK_H__

#include <stdint.h>
#include <ewoksys/ipc.h>
#include <pthread.h>
#include "platform.h"

enum{
    NET_TASK_IDLE,
    NET_TASK_START,
    NET_TASK_PROCESS,
    NET_TASK_FINISH
};

#define TASK_READ_BUF_SIZE 1024*16
typedef struct net_task{
    int fd;
	int from_pid;
	int node;
    int cmd;
    int read_from_pid;
    pthread_t tid;
    struct sched_ctx wait_ctx;
    char read_buf[TASK_READ_BUF_SIZE];
	proto_t in;
	proto_t out;
	proto_t read_in;
	proto_t read_out;
	void *p;
	void *read_p;
    bool running;
    int state;
    int read_state;
    int sock;
    int refs;
    bool pending_main_rd;
    int thread_started;
    /*
     * FS_CMD_CLOSE arrives on the dispatch thread; the VFS_EVT_CLOSE wakeup
     * (a reverse IPC to vfsd) must NOT be issued there (vfsd is synchronously
     * waiting on netd for that very close). Defer it to the worker self-reap.
     */
    uint32_t pending_close_wakeup;

    struct net_task* next;
    struct net_task* prev;
}net_task_t;

net_task_t *create_task(int fd, int from_pid, int node);
void start_task(void);
void release_task(net_task_t *task);
int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p);
int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p);
int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p);
int task_check_read_events(void);
int task_has_read_watchers(void);
int task_wakeup_tcp_readers(int tcp_desc);
int task_wakeup_tcp_writers(int tcp_desc);
int task_wakeup_udp_readers(int udp_desc);
int task_wakeup_raw_readers(int sock_id);

extern pthread_mutex_t task_list_lock;

#endif
