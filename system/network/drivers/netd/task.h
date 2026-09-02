#ifndef __NET_TASK_H__
#define __NET_TASK_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <ewoksys/ipc.h>
#include <pthread.h>
#include "platform.h"

/*
 * One task per open of /dev/net0 (each open gets a unique anonymous VFS
 * node). Under IPC_MULTI_TASK the kernel worker pool runs the vdevice
 * handlers concurrently, so a task is nothing but the node<->socket binding
 * plus the state those concurrent handlers must share:
 *
 *  - refs:      vfs open/dup reference count (dup/close bookkeeping).
 *  - inflight:  handlers currently operating on this task; release_task()
 *               and SOCK_CLOSE drain it before the socket id is returned to
 *               the stack's free pool (a concurrent handler holds a snapshot
 *               of that id and must not hit a reused connection).
 *  - closing:   set once refs hit 0; rejects new operations while the
 *               teardown drains.
 *  - dead:      release_task() drained its bounded window but an operation
 *               was still inflight (e.g. task_write() stalled in
 *               net_tx_flush() on wl0 TX backpressure): ownership of the
 *               deferred sock_close + free is handed to the LAST
 *               task_end_op(). Freeing eagerly was a use-after-free on a
 *               destroyed mutex inside that op's task_end_op().
 *  - fin_sock:  the detached socket id the deferred teardown must close.
 *  - rcv_*:     SO_RCVTIMEO bookkeeping for blocking recv()/recvfrom()
 *               (see task_timeout_check() in task.c).
 *
 * Lock order: stack mutex -> task_list_lock -> task->lock -> wakeup queue.
 */
typedef struct net_task {
    int fd;
    int from_pid;
    int node;
    int sock;                   /* bound stack socket id, -1 when unbound */
    int refs;
    int inflight;
    int fin_sock;               /* deferred-close socket id (dead handoff) */
    bool closing;
    bool dead;                  /* freed by the last task_end_op() */
    bool is_listener;
    bool rcv_deadline_set;      /* armed SO_RCVTIMEO deadline */
    bool rcv_timeout_pending;   /* expired deadline latched for the retried recv */
    struct timeval rcv_deadline;
    pthread_mutex_t lock;
    struct net_task* next;
    struct net_task* prev;
} net_task_t;

net_task_t *create_task(int fd, int from_pid, int node);
net_task_t *task_find_live_by_node(uint32_t node);
void start_task(void);
void release_task(net_task_t *task);

/* Node-keyed handler entry points (run concurrently on IPC pool workers). */
int      task_cntl(uint32_t node, int from_pid, int cmd, proto_t *in, proto_t *out);
int      task_read(uint32_t node, char* buf, int size);
int      task_write(uint32_t node, const char* buf, int size);
uint32_t task_poll_events(uint32_t node);

/* Periodic SO_RCVTIMEO sweeper, registered as a stack net-timer. */
void task_timeout_check(void);

/* Deferred vfs_wakeup() flusher, called by intr_step() outside all locks. */
void task_flush_wakeups(void);

/* Stack-side wakeup hooks (callers hold the global stack mutex). */
int task_wakeup_tcp_readers(int tcp_desc);
int task_wakeup_tcp_writers(int tcp_desc);
int task_wakeup_udp_readers(int udp_desc);
int task_wakeup_raw_readers(int sock_id);

extern pthread_mutex_t task_list_lock;

#endif
