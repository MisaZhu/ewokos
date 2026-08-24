#ifndef __NET_TASK_H__
#define __NET_TASK_H__

#include <stdint.h>
#include <sys/time.h>
#include <ewoksys/ipc.h>
#include <pthread.h>
#include "platform.h"

enum{
    NET_TASK_IDLE,
    NET_TASK_START,
    NET_TASK_PROCESS,
    NET_TASK_FINISH
};

/* Match the TCP receive buffer (32KB) so one SOCK_RECV worker run can
 * drain a full receive window, and sshd's ~32KB SSH packet body reads
 * complete in a single IPC round trip instead of two. */
#define TASK_READ_BUF_SIZE 1024*32
#define TASK_WRITE_BUF_SIZE 1024*32
typedef struct net_task{
    int fd;
	int from_pid;
	int node;
    int cmd;
    int read_from_pid;
    pthread_t tid;
	pthread_mutex_t lock;
    struct sched_ctx wait_ctx;
    char read_buf[TASK_READ_BUF_SIZE];
    char tx_buf[TASK_WRITE_BUF_SIZE];
	proto_t in;
	proto_t out;
	proto_t read_in;
	proto_t read_out;
	proto_t write_in;
	proto_t write_out;
	void *p;
	void *read_p;
	void *write_p;
    bool running;
    int state;
    int read_state;
    int write_state;
    int sock;
    int refs;
    bool is_listener;
    bool pending_main_rd;
    bool write_ready;
    int thread_started;
    bool read_prefetch;
    bool read_cache_ready;
    int read_cache_len;
    int read_cache_off;
    int read_cache_errno;
    int write_from_pid;
    /*
     * Async-accepted write bookkeeping: write_off tracks how much of the
     * armed write_in payload the worker has already pushed into the TCP
     * stack; write_err latches a hard send error (the client already got the
     * accepted byte count back) to be reported on the next write().
     */
    uint32_t write_off;
    int write_err;
    /*
     * Out-of-band SENDTO slot. The main fcntl slot is single-flight: while a
     * blocking recv()/recvfrom()/accept() sits armed in it (state PROCESS),
     * a sendto() on the same socket used to get a bare VFS_ERR_RETRY with
     * nothing armed that could ever fire the VFS_EVT_WR the caller then
     * sleeps on -- the send->recv->send hang (circular wait: the armed recv
     * wants a reply to the very datagram the blocked send never emitted).
     * SENDTO shares no request state with the armed command, so it runs
     * through this side slot on the same worker whenever the main slot is
     * busy, and its completion fires VFS_EVT_WR independently.
     */
    int sendto_state;
    int sendto_from_pid;
    proto_t sendto_in;
    proto_t sendto_out;
    /*
     * Out-of-band RECVFROM slot: mirror image of the SENDTO side slot. While
     * the main fcntl slot is busy (e.g. an in-flight SENDTO issued from
     * another thread of the same process on the same socket), a recvfrom()
     * used to get a bare VFS_ERR_RETRY with nothing armed: the caller parks
     * on VFS_EVT_RD, the busy SENDTO completes with a WR edge only, and
     * arriving datagrams wake nobody -> the reader thread wedges forever
     * (macemu DNS dies permanently after the first reply). Runs on the same
     * worker with its own SO_RCVTIMEO deadline; completion fires
     * VFS_EVT_RD independently of the main slot.
     */
    int recvfrom_state;
    int recvfrom_from_pid;
    proto_t recvfrom_in;
    proto_t recvfrom_out;
    struct timeval recvfrom_deadline;
    bool recvfrom_deadline_set;
    /*
     * FS_CMD_CLOSE arrives on the dispatch thread; the VFS_EVT_CLOSE wakeup
     * (a reverse IPC to vfsd) must NOT be issued there (vfsd is synchronously
     * waiting on netd for that very close). Defer it to the worker self-reap.
     */
    uint32_t pending_close_wakeup;
    /*
     * SO_RCVTIMEO deadline for the armed recv()/recvfrom() request. The worker
     * is never allowed to block inside the stack, so the timeout the stack
     * implements internally can never fire; task_timeout_check() uses this to
     * complete the request with ETIMEDOUT instead of leaving the client parked
     * in vfs_block() forever.
     */
    struct timeval main_deadline;
    bool main_deadline_set;

    struct net_task* next;
    struct net_task* prev;
}net_task_t;

net_task_t *create_task(int fd, int from_pid, int node);
net_task_t *task_find_live_by_node(uint32_t node);
void start_task(void);
void release_task(net_task_t *task);
int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p);
int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p);
int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p);
int task_check_read_events(void);
void task_timeout_check(void);
int task_has_read_watchers(void);
int task_wakeup_tcp_readers(int tcp_desc);
int task_wakeup_tcp_writers(int tcp_desc);
int task_wakeup_udp_readers(int udp_desc);
int task_wakeup_raw_readers(int sock_id);

extern pthread_mutex_t task_list_lock;

#endif
