#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>

#include "task.h"
#include "netd.h"
#include "stack/util.h"

extern int sock_readable(int sock);
extern int sock_writable(int sock);
extern ssize_t sock_send(int id, const void *buf, size_t n);
extern int sock_data_readable(int sock);
extern int sock_tcp_scan_info(int id, int *desc, int *state, int *remain);
extern int sock_get_desc(int id);
extern int sock_get_type(int id);
extern int sock_id_from_tcp_desc(int tcp_desc);
extern int sock_id_from_udp_desc(int udp_desc);
extern void sock_init_maps(void);
extern int sched_ctx_init(struct sched_ctx *ctx);
extern int sched_ctx_destroy(struct sched_ctx *ctx);
extern int sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timeval *abstime);
extern int sched_wakeup(struct sched_ctx *ctx);

#define TASK_POLL_INTERVAL_US 1000 /* 1ms */

static void* task_thread(void* arg);
static void task_list_remove(net_task_t * task);
 
pthread_mutex_t task_list_lock;
net_task_t *task_list = NULL;

#ifndef SOCKS_MAX
#define SOCKS_MAX 128
#endif
static net_task_t* sock_to_task[SOCKS_MAX];
static uint32_t task_total_created = 0;
static uint32_t task_total_freed = 0;
static uint32_t task_active_count = 0;

/*
 * Deferred VFS wakeup delivery.
 *
 * vfs_wakeup() is a blocking reverse IPC into vfsd. The stack-side wakeup
 * paths (tcp_segment_arrives()/tcp_timer()/tcp_pcb_release() and the UDP
 * input path) run WHILE HOLDING the global stack mutex. If that IPC has to
 * wait (vfsd busy, or vfsd itself blocked in an ipc_call into netd, e.g. a
 * clear_zombie FS_CMD_CLOSE), the stack mutex stays held for the whole wait.
 * pthread_mutex_lock() is a try+yield spin (semaphore_enter), so every other
 * netd thread that touches the stack then burns CPU in "rdy": the dispatch
 * thread wedges inside check_poll_events()->sock_writable(), netd stops
 * answering IPC, and vfsd's call into netd never completes either -- a
 * netd<->vfsd livelock with several threads spinning (the sshd-connection
 * spin). Queue the wakeup here instead and let a dedicated flusher thread
 * issue the IPC with no netd lock held. Entries coalesce per VFS node, which
 * also rate-limits per-segment wakeup storms into single edges.
 */
#define WAKEUP_QUEUE_MAX 64
typedef struct {
    uint32_t node;
    uint32_t events;
} pending_wakeup_t;
static pending_wakeup_t wakeup_queue[WAKEUP_QUEUE_MAX];
static uint32_t wakeup_queue_num = 0;
static pthread_mutex_t wakeup_queue_lock;
static struct sched_ctx wakeup_queue_ctx;
static int wakeup_thread_ok = 0;

static int task_cmd_runs_inline(int cmd) {
    switch (cmd) {
        case SOCK_OPEN:
        case SOCK_BIND:
        case SOCK_LISTEN:
        case SOCK_ACCEPT:
        case SOCK_LINK:
        case SOCK_SETOPT:
        case SOCK_GETOPT:
            return 1;
        default:
            return 0;
    }
}

static int task_start_worker_locked(net_task_t *task) {
    int saved_errno;

    if (task == NULL)
        return -1;
    if (task->thread_started)
        return 0;

    if (pthread_create(&task->tid, NULL, task_thread, task) != 0) {
        saved_errno = errno;
        if (saved_errno == 0)
            saved_errno = EAGAIN;
        errno = saved_errno;
        klog("netd: task worker pthread_create failed fd=%d from_pid=%d node=%d err=%d\n",
                task->fd, task->from_pid, task->node, saved_errno);
        return -1;
    }
    pthread_detach(task->tid);
    task->thread_started = 1;
    return 0;
}

static void task_free_unstarted(net_task_t *task) {
    int fin_sock;

    if (task == NULL)
        return;

    pthread_mutex_lock(&task->lock);
    task->running = false;
    fin_sock = task->sock;
    task->sock = -1;
    pthread_mutex_unlock(&task->lock);

    pthread_mutex_lock(&task_list_lock);
    if (fin_sock >= 0 && fin_sock < SOCKS_MAX && sock_to_task[fin_sock] == task)
        sock_to_task[fin_sock] = NULL;
    pthread_mutex_unlock(&task_list_lock);

    task_list_remove(task);
    if (fin_sock >= 0)
        sock_close(fin_sock);

    PF->clear(&task->in);
    PF->clear(&task->out);
    PF->clear(&task->read_in);
    PF->clear(&task->read_out);
    PF->clear(&task->write_in);
    PF->clear(&task->write_out);
    sched_ctx_destroy(&task->wait_ctx);

    pthread_mutex_lock(&task_list_lock);
    if (task_active_count > 0)
        task_active_count--;
    task_total_freed++;
    pthread_mutex_unlock(&task_list_lock);
    pthread_mutex_destroy(&task->lock);
    free(task);
}

static void task_queue_vfs_wakeup(uint32_t node, uint32_t events) {
    if (node == 0 || events == 0)
        return;
    if (!wakeup_thread_ok) {
        /* Degraded mode: flusher thread failed to start. */
        vfs_wakeup(node, (int)events);
        return;
    }
    pthread_mutex_lock(&wakeup_queue_lock);
    uint32_t i;
    for (i = 0; i < wakeup_queue_num; i++) {
        if (wakeup_queue[i].node == node) {
            wakeup_queue[i].events |= events;
            break;
        }
    }
    if (i == wakeup_queue_num) {
        if (wakeup_queue_num < WAKEUP_QUEUE_MAX) {
            wakeup_queue[i].node = node;
            wakeup_queue[i].events = events;
            wakeup_queue_num++;
        } else {
            /* Unreachable in practice: entries coalesce per node and netd
             * serves a single device node. Merge rather than drop. */
            wakeup_queue[0].events |= events;
        }
    }
    sched_wakeup(&wakeup_queue_ctx);
    pthread_mutex_unlock(&wakeup_queue_lock);
}

static void* task_wakeup_thread(void* arg) {
    (void)arg;
    pending_wakeup_t batch[WAKEUP_QUEUE_MAX];
    while (1) {
        uint32_t num;
        pthread_mutex_lock(&wakeup_queue_lock);
        while (wakeup_queue_num == 0) {
            sched_sleep(&wakeup_queue_ctx, (mutex_t*)&wakeup_queue_lock, NULL);
        }
        num = wakeup_queue_num;
        memcpy(batch, wakeup_queue, num * sizeof(pending_wakeup_t));
        wakeup_queue_num = 0;
        pthread_mutex_unlock(&wakeup_queue_lock);
        for (uint32_t i = 0; i < num; i++) {
            vfs_wakeup(batch[i].node, (int)batch[i].events);
        }
    }
    return NULL;
}

static int task_owner_pid(int from_pid) {
    int owner_pid = proc_getpid(from_pid);

    if(owner_pid > 0)
        return owner_pid;
    return from_pid;
}

static void task_list_add(net_task_t * task){
    pthread_mutex_lock(&task_list_lock);
    task->prev = NULL;
    if(task_list == NULL){
        task_list = task;
        task->next = NULL;
    }else{
        net_task_t *t = task_list;

        while(t->next != NULL){
            t = t->next;
        }
        t->next = task;
        task->prev = t;
        task->next = NULL;
    }
    pthread_mutex_unlock(&task_list_lock);
}

static void task_list_remove(net_task_t * task){
    if(task == NULL)
        return;

    pthread_mutex_lock(&task_list_lock);
    if(task == task_list)
        task_list = task->next;

    if(task->prev)
        task->prev->next = task->next;
    if(task->next)
        task->next->prev = task->prev;
    task->next = NULL;
    task->prev = NULL;
    pthread_mutex_unlock(&task_list_lock);
}

void start_task(void){
    /*
     * Workers are created eagerly in create_task() and tasks are destroyed
     * directly from the close path, so the old full-list maintenance scan is
     * no longer needed on every intr_loop() tick.
     *
     * Initialize the O(1) per-connection wakeup maps: sock_to_task[] here and
     * the desc-to-sock reverse maps inside the stack.
     */
    memset(sock_to_task, 0, sizeof(sock_to_task));
    sock_init_maps();

    memset(wakeup_queue, 0, sizeof(wakeup_queue));
    wakeup_queue_num = 0;
    /*
     * Explicit init before the flusher starts: the lazy semaphore_alloc in
     * pthread_mutex_lock() races when two threads hit the first lock at once.
     */
    pthread_mutex_init(&wakeup_queue_lock, NULL);
    sched_ctx_init(&wakeup_queue_ctx);
    pthread_t wakeup_tid;
    if (pthread_create(&wakeup_tid, NULL, task_wakeup_thread, NULL) == 0) {
        pthread_detach(wakeup_tid);
        wakeup_thread_ok = 1;
    } else {
        klog("netd: wakeup flusher thread create failed, using direct wakeups\n");
    }
}

net_task_t *create_task(int fd, int from_pid, int node){
    net_task_t *task = malloc(sizeof(net_task_t));
    if(task == NULL) {
        errno = ENOMEM;
        klog("netd: create_task malloc failed fd=%d from_pid=%d node=%d active=%u created=%u freed=%u\n",
                fd, from_pid, node, task_active_count, task_total_created,
                task_total_freed);
        return NULL;
    }
    memset(task, 0 , sizeof(net_task_t));
    pthread_mutex_init(&task->lock, NULL);
    sched_ctx_init(&task->wait_ctx);
    task->fd = fd;
    task->node = node;
    task->from_pid = task_owner_pid(from_pid);
    task->state = NET_TASK_IDLE;
    task->read_from_pid = task->from_pid;
    task->read_state = NET_TASK_IDLE;
    task->write_from_pid = task->from_pid;
    task->write_state = NET_TASK_IDLE;
    task->sock = -1;
    task->refs = 1;
    task->running = true;
    task->write_ready = true;
    task->thread_started = 0;
    task_list_add(task);
    pthread_mutex_lock(&task_list_lock);
    task_total_created++;
    task_active_count++;
    pthread_mutex_unlock(&task_list_lock);
    return task;
}

void release_task(net_task_t *task){
    if(task == NULL)
        return;

    if(!task->thread_started) {
        task_free_unstarted(task);
        return;
    }

    /*
     * Teardown MUST NOT block the shared IPC dispatch context. The old path ran
     * sock_close() (up to ~300ms of TCP graceful-close retries) and
     * pthread_join() right here, inside network_close(). Because the kernel
     * gives each server process a single active IPC slot, that froze every
     * other connection's read/write/poll AND every accept() for the whole
     * duration.
     *
     * Instead just flag the connection's own worker and wake it. The worker
     * performs sock_close() in its own thread, removes itself from task_list,
     * and frees itself (it is detached), so dispatch stays fair and accept()
     * keeps responding immediately.
     */
    pthread_mutex_lock(&task->lock);
    task->running = false;
    task->pending_close_wakeup = (task->from_pid > 0) ? 1 : 0;
    sched_wakeup(&task->wait_ctx);
    pthread_mutex_unlock(&task->lock);

    /*
     * The close-wakeup (vfs_wakeup, a reverse IPC to vfsd) is issued by the
     * worker's self-reap path, NOT here. release_task() runs on the netd IPC
     * dispatch thread while vfsd is synchronously waiting on the current
     * FS_CMD_CLOSE; calling back into vfsd from here would deadlock
     * netd<->vfsd and pin task_list_lock forever under concurrent load.
     */
}

static uint32_t task_arm_wait_event(int cmd) __attribute__((unused));
static uint32_t task_arm_wait_event(int cmd) {
    switch (cmd) {
        case SOCK_CONNECT:
        case SOCK_SENDTO:
            return VFS_EVT_WR;
        case SOCK_ACCEPT:
        case SOCK_RECVFROM:
            return VFS_EVT_RD;
        default:
            return 0;
    }
}

int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p){
    from_pid = task_owner_pid(from_pid);
    pthread_mutex_lock(&task->lock);
    
    if(task->state == NET_TASK_FINISH){
        if(cmd != task->cmd){
            pthread_mutex_unlock(&task->lock);
            return VFS_ERR_RETRY;
        }
        if(from_pid == task->from_pid) {
            PF->copy(out, task->out.data, task->out.size);
            PF->clear(&task->out);
            PF->clear(&task->in);
            task->state = NET_TASK_IDLE;
            pthread_mutex_unlock(&task->lock);
            return 0;
        }
        /*
         * The same socket can be inherited across fork()/dup(). If a new pid
         * touches the socket after the previous owner completed an async
         * request, drop the stale completion and arm the new request in this
         * call. Returning RETRY here can strand the caller in VFS_EVT_WR sleep
         * with no freshly armed operation to wake it.
         */
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
    }

    if(task->state == NET_TASK_IDLE){
        if(task_cmd_runs_inline(cmd)) {
            task->cmd = cmd;
            task->p = p;
            task->from_pid = from_pid;
            PF->clear(&task->in);
            PF->clear(&task->out);
            PF->copy(&task->in, in->data, in->size);
            task->state = NET_TASK_PROCESS;
            pthread_mutex_unlock(&task->lock);

            if(do_network_fcntl(task) <= 0) {
                pthread_mutex_lock(&task->lock);
                PF->clear(&task->in);
                PF->clear(&task->out);
                task->state = NET_TASK_IDLE;
                pthread_mutex_unlock(&task->lock);
                return VFS_ERR_RETRY;
            }

            pthread_mutex_lock(&task->lock);
            PF->copy(out, task->out.data, task->out.size);
            PF->clear(&task->out);
            PF->clear(&task->in);
            task->state = NET_TASK_IDLE;
            pthread_mutex_unlock(&task->lock);
            return 0;
        }
        if(!task->thread_started && task_start_worker_locked(task) != 0) {
            pthread_mutex_unlock(&task->lock);
            return -1;
        }
        task->cmd = cmd;	
        task->p = p;
        task->from_pid = from_pid;
        /*
         * task->in/out are persistent proto_t objects. PF->init() only resets
         * the view and does not release heap buffers grown by previous large I/O.
         */
        PF->clear(&task->in);
        PF->clear(&task->out);
        PF->copy(&task->in, in->data, in->size);
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task->lock);
        // Signal task thread to wake up
    } else {
        pthread_mutex_unlock(&task->lock);
    }
    return VFS_ERR_RETRY;
}

int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p){
    from_pid = task_owner_pid(from_pid);
    pthread_mutex_lock(&task->lock);

    if(task->read_cache_ready) {
        int remain = task->read_cache_len - task->read_cache_off;
        int cache_errno = task->read_cache_errno;
        int len = size < remain ? size : remain;
        if(len > 0) {
            memcpy(buf, task->read_buf + task->read_cache_off, len);
            task->read_cache_off += len;
        }
        if(task->read_cache_off >= task->read_cache_len) {
            task->read_cache_ready = false;
            task->read_cache_len = 0;
            task->read_cache_off = 0;
            task->read_cache_errno = 0;
        }
        pthread_mutex_unlock(&task->lock);
        if(len == 0 && cache_errno != 0) {
            errno = cache_errno;
            return -1;
        }
        return len;
    }
    
    if(task->read_state == NET_TASK_FINISH){
        if(from_pid == task->read_from_pid) {
            int len = proto_read_int(&task->read_out);
            int sock_errno = 0;
            if(len > 0){
                proto_read_to(&task->read_out, buf, len);
            }
            if(len <= 0 && task->read_out.size > task->read_out.offset) {
                sock_errno = proto_read_int(&task->read_out);
            }
            PF->clear(&task->read_out);
            PF->clear(&task->read_in);
            task->read_state = NET_TASK_IDLE;
            pthread_mutex_unlock(&task->lock);
            if(len < 0 && (sock_errno == 0 || sock_errno == EAGAIN || sock_errno == EINTR)) {
                return VFS_ERR_RETRY;
            }
            return len;
        }
        /*
         * A fork-inherited stdin may be touched by a child process before the
         * parent shell issues the next read. Clear the stale completion and
         * arm the parent read immediately so the caller does not sleep on an
         * already-consumed event.
         */
        PF->clear(&task->read_out);
        PF->clear(&task->read_in);
        task->read_state = NET_TASK_IDLE;
    }

    if(task->read_state == NET_TASK_IDLE){
        if(!task->thread_started && task_start_worker_locked(task) != 0) {
            pthread_mutex_unlock(&task->lock);
            return -1;
        }
        task->read_p = p;
        task->read_from_pid = from_pid;
        PF->clear(&task->read_in);
        PF->clear(&task->read_out);
        PF->addi(&task->read_in, size);
        task->read_state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task->lock);
    } else {
        pthread_mutex_unlock(&task->lock);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
    from_pid = task_owner_pid(from_pid);
    pthread_mutex_lock(&task->lock);

    /* Legacy FINISH state no longer produced by the worker; discard it so a
     * stale slot can never wedge the write path. */
    if(task->write_state == NET_TASK_FINISH){
        PF->clear(&task->write_out);
        PF->clear(&task->write_in);
        task->write_state = NET_TASK_IDLE;
    }

    if(task->write_state == NET_TASK_IDLE){
        /*
         * Async-accepted write: copy the payload into write_in and return the
         * byte count NOW, on the dispatch thread. The worker drains it into
         * the TCP stack in the background (rearmed by the ACK-driven
         * task_wakeup_tcp_writers() whenever the send window closes).
         *
         * This collapses the old per-write round trip (RETRY -> client
         * vfs_block -> worker sock_send -> deferred vfs_wakeup -> client
         * retries the write IPC, ~ms each, capping bulk TX near 500KB/s) to a
         * single IPC in the common case. The dispatch thread MUST NOT enter
         * the stack itself: sock_send() issues blocking eth-driver ipc_calls,
         * which are forbidden in the IPC handler context.
         *
         * Backpressure: while a previous write is still draining the slot is
         * busy and the client gets VFS_ERR_RETRY below, blocking on
         * VFS_EVT_WR until the completion wakeup frees the slot.
         */
        if(task->write_err != 0) {
            /* A previous async-accepted write failed hard after its byte
             * count was already returned; surface the error here. */
            int werr = task->write_err;
            task->write_err = 0;
            pthread_mutex_unlock(&task->lock);
            errno = werr;
            return -1;
        }
        if(size == 0) {
            pthread_mutex_unlock(&task->lock);
            return 0;
        }
        if(buf == NULL || size < 0) {
            pthread_mutex_unlock(&task->lock);
            errno = EINVAL;
            return -1;
        }
        if(!task->thread_started && task_start_worker_locked(task) != 0) {
            pthread_mutex_unlock(&task->lock);
            return -1;
        }
        task->write_p = p;
        task->write_from_pid = from_pid;
        task->write_ready = false;
        task->write_off = 0;
        PF->clear(&task->write_in);
        PF->clear(&task->write_out);
        PF->add(&task->write_in, buf, size);
        task->write_state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task->lock);
        return size;
    }

    pthread_mutex_unlock(&task->lock);
    return VFS_ERR_RETRY;
}


int do_network_fcntl(net_task_t *task){
	int domain, sock,type,protocol, level, optname;
	char *data, *optval;
	int32_t size, addrlen = sizeof(struct sockaddr), optlen;
	struct sockaddr *paddr;
	struct sockaddr addr;
	int ret = -1;
	int sock_errno = 0;
	sock = task->sock;

	switch(task->cmd){
		case SOCK_OPEN:
			domain = proto_read_int(&task->in);
			type = proto_read_int(&task->in);
			protocol = proto_read_int(&task->in);
			sock = sock_open(domain, type, protocol);
			PF->addi(&task->out, sock);
			pthread_mutex_lock(&task_list_lock);
			pthread_mutex_lock(&task->lock);
			task->sock = sock;
            task->write_ready = true;
			if(sock >= 0 && sock < SOCKS_MAX) {
				sock_to_task[sock] = task;
			}
			pthread_mutex_unlock(&task->lock);
			pthread_mutex_unlock(&task_list_lock);
			break;
		case SOCK_BIND:
			paddr = proto_read(&task->in, &addrlen);
			if(paddr == NULL) {
				ret = -1;
			} else {
				ret = sock_bind(sock, paddr, addrlen);
			}
			PF->addi(&task->out, ret);
			break;
		case SOCK_SENDTO:
			data = proto_read(&task->in, &size);
			paddr = proto_read(&task->in, &addrlen);
			if(data == NULL || paddr == NULL) {
				ret = -1;
			} else {
				errno = 0;
				ret = sock_sendto(sock, data, size, paddr, addrlen);
				sock_errno = errno;
				if(ret < 0 && sock_errno == 0)
					sock_errno = (ret == -17) ? EBADF : EIO;
			}
			PF->addi(&task->out, ret);
			PF->addi(&task->out, ret < 0 ? sock_errno : 0);
			break;
		case SOCK_RECVFROM:
			/*
			 * Keep the per-socket worker event-driven: it must never
			 * sched_sleep() inside the stack. If nothing is readable yet,
			 * leave the request armed in PROCESS and let
			 * task_wakeup_*_readers() restart it once data/EOF arrives,
			 * mirroring the non-blocking do_network_read() path. Blocking
			 * here parks the connection worker deep in the stack ("blk"),
			 * which is only releasable via internal stack wakeups.
			 */
			if(sock >= 0 && !sock_readable(sock)) {
				return 0;
			}
			size = proto_read_int(&task->in);
            size = size < TASK_READ_BUF_SIZE ? size:TASK_READ_BUF_SIZE;
            errno = 0;
            ret = sock_recvfrom(sock, task->read_buf, size, &addr, &addrlen);
            sock_errno = errno;
            if(ret < 0 && sock_errno == 0)
                sock_errno = EAGAIN;
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->addi(&task->out, addrlen);
                PF->add(&task->out, task->read_buf, ret);
                PF->add(&task->out, &addr, addrlen);	
            }
            PF->addi(&task->out, ret < 0 ? sock_errno : 0);
			break;
		case SOCK_SEND:
			data = proto_read(&task->in, &size);
            if(data && size){
				errno = 0;
			    ret = sock_send(sock, data, size);
				sock_errno = errno;
				if(ret < 0 && sock_errno == 0)
					sock_errno = EAGAIN;
            } else {
				/* Nothing to send: report 0 bytes written, not a phantom -1
				 * error.  A spurious -1/errno=0 here would get treated as
				 * EAGAIN by task_write() and re-armed forever, blocking the
				 * client on VFS_EVT_WR with no wakeup source. */
				ret = 0;
				sock_errno = 0;
			}
			PF->addi(&task->out, ret);
			PF->addi(&task->out, ret < 0 ? sock_errno : 0);
			break;
		case SOCK_RECV:
			/* Non-blocking mirror of do_network_read(): re-arm instead of
			 * sched_sleep()-ing the worker inside tcp_receive(). */
			if(sock >= 0 && !sock_readable(sock)) {
				return 0;
			}
			size = proto_read_int(&task->in);
            size = size < TASK_READ_BUF_SIZE ? size:TASK_READ_BUF_SIZE;
            errno = 0;
            ret = sock_recv(sock, task->read_buf, size);
            sock_errno = errno;
            if(ret < 0 && sock_errno == 0)
                sock_errno = EAGAIN;
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->read_buf, ret);
            }
            PF->addi(&task->out, ret < 0 ? sock_errno : 0);
			break;
		case SOCK_LISTEN:
			size = proto_read_int(&task->in);
			ret = sock_listen(sock, size);
			PF->addi(&task->out, ret);
			break;	
		case SOCK_ACCEPT:
            if(sock >= 0 && !sock_readable(sock)) {
                return 0;
            }
            errno = 0;
			ret = sock_accept(sock, &addr, &addrlen);
            sock_errno = errno;
            if(ret < 0 && sock_errno == 0)
                sock_errno = EAGAIN;
            if(ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
                return 0;
            }
			PF->addi(&task->out, ret);
			if(ret > 0){
				PF->add(&task->out, &addr, addrlen);	
			}
			break;	
		case SOCK_CLOSE:
			ret = sock_close(sock);
			PF->addi(&task->out, ret);
			break;
		case SOCK_LINK:
			sock = proto_read_int(&task->in);	
			pthread_mutex_lock(&task_list_lock);
			pthread_mutex_lock(&task->lock);
			if(task->sock >= 0 && task->sock < SOCKS_MAX &&
			   sock_to_task[task->sock] == task) {
				sock_to_task[task->sock] = NULL;
			}
			task->sock = sock;
            task->write_ready = true;
			if(sock >= 0 && sock < SOCKS_MAX) {
				sock_to_task[sock] = task;
			}
			pthread_mutex_unlock(&task->lock);
			pthread_mutex_unlock(&task_list_lock);
			PF->addi(&task->out, 0);
			break;
		case SOCK_CONNECT:
			{
				uint32_t saved_offset = task->in.offset;
				paddr = proto_read(&task->in, &addrlen);
				if(paddr == NULL) {
					ret = -1;
				} else {
					errno = 0;
					ret = sock_connect(sock, paddr, addrlen);
					sock_errno = errno;
					if(ret < 0 && sock_errno == 0)
						sock_errno = EAGAIN;
					if(ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
						task->in.offset = saved_offset;
						return 0;
					}
				}
				PF->addi(&task->out, ret);
				PF->addi(&task->out, ret < 0 ? sock_errno : 0);
				break;
			}
		case SOCK_SETOPT:
		level = proto_read_int(&task->in);
		optname = proto_read_int(&task->in);
		optval = proto_read(&task->in, &optlen);
		if(optval == NULL) {
			ret = -1;
		} else {
			ret = sock_setsockopt(sock, level, optname, optval, optlen);
		}
		PF->addi(&task->out, ret);
		break;
	case SOCK_GETOPT:
		level = proto_read_int(&task->in);
		optname = proto_read_int(&task->in);
		optlen = proto_read_int(&task->in);
		// First read the optlen, then process
		ret = sock_getsockopt(sock, level, optname, task->read_buf, &optlen);
		PF->addi(&task->out, ret);
		if(ret == 0) {
			PF->addi(&task->out, optlen);
			PF->add(&task->out, task->read_buf, optlen);
		}
		break;
		default:
			break;
	}
    return 1;
}

static int do_network_read(net_task_t *task){
	int32_t size;
	int ret;
	int sock_errno = 0;
    uint32_t saved_offset;

    if(task->sock < 0) {
        PF->addi(&task->read_out, -1);
        PF->addi(&task->read_out, EBADF);
        return 1;
    }

    /*
     * Keep the single per-socket worker non-blocking on reads. If the stream
     * is not readable yet, leave the request armed in read_state=PROCESS and
     * let task_check_read_events()/task_wakeup_tcp_readers() restart it once
     * data or EOF becomes observable.
     */
    if(!sock_readable(task->sock)) {
        return 0;
    }

    if(task->read_prefetch) {
        errno = 0;
        ret = sock_recv(task->sock, task->read_buf, TASK_READ_BUF_SIZE);
        sock_errno = errno;
        if(ret < 0 && sock_errno == 0)
            sock_errno = EAGAIN;
        if(ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
            return 0;
        }
        task->read_cache_len = ret > 0 ? ret : 0;
        task->read_cache_off = 0;
        task->read_cache_errno = ret < 0 ? sock_errno : 0;
        return 1;
    }

    saved_offset = task->read_in.offset;
	size = proto_read_int(&task->read_in);
    size = size < TASK_READ_BUF_SIZE ? size:TASK_READ_BUF_SIZE;
    errno = 0;
    ret = sock_recv(task->sock, task->read_buf, size);
    sock_errno = errno;
    if(ret < 0 && sock_errno == 0)
        sock_errno = EAGAIN;
    if(ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
        task->read_in.offset = saved_offset;
        return 0;
    }
    PF->addi(&task->read_out, ret);
    if(ret > 0){
        PF->add(&task->read_out, task->read_buf, ret);
    }
    PF->addi(&task->read_out, ret < 0 ? sock_errno : 0);
    return 1;
}

static int do_network_write(net_task_t *task){
    int32_t size;
    int ret;
    int sock_errno = 0;
    uint32_t saved_offset;
    char *data;

    /*
     * The client was already handed the full byte count when this write was
     * accepted (task_write async model), so this worker MUST drain write_in
     * completely. Partial sends (window closed mid-burst) keep the slot armed
     * (return 0, state stays PROCESS) with write_off recording progress; the
     * ACK-driven task_wakeup_tcp_writers() rearms us. Hard errors are latched
     * into write_err for the client's next write() to report.
     */
    if(task->sock < 0) {
        pthread_mutex_lock(&task->lock);
        if(task->write_err == 0)
            task->write_err = EBADF;
        pthread_mutex_unlock(&task->lock);
        return 1;
    }

    if(!sock_writable(task->sock)) {
        pthread_mutex_lock(&task->lock);
        task->write_ready = false;
        pthread_mutex_unlock(&task->lock);
        return 0;
    }

    saved_offset = task->write_in.offset;
    data = proto_read(&task->write_in, &size);
    if(data == NULL || size < 0 || task->write_off > (uint32_t)size) {
        pthread_mutex_lock(&task->lock);
        if(task->write_err == 0)
            task->write_err = EINVAL;
        pthread_mutex_unlock(&task->lock);
        return 1;
    }

    errno = 0;
    ret = sock_send(task->sock, data + task->write_off, size - task->write_off);
    sock_errno = errno;
    if(ret < 0 && sock_errno == 0)
        sock_errno = EAGAIN;
    if(ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
        task->write_in.offset = saved_offset;
        pthread_mutex_lock(&task->lock);
        task->write_ready = false;
        pthread_mutex_unlock(&task->lock);
        return 0;
    }
    if(ret < 0) {
        pthread_mutex_lock(&task->lock);
        if(task->write_err == 0)
            task->write_err = sock_errno;
        task->write_ready = false;
        pthread_mutex_unlock(&task->lock);
        return 1;
    }

    task->write_off += (uint32_t)ret;
    if(task->write_off < (uint32_t)size) {
        /* Window closed with a short send: stay armed for the ACK rearm. */
        task->write_in.offset = saved_offset;
        pthread_mutex_lock(&task->lock);
        task->write_ready = false;
        pthread_mutex_unlock(&task->lock);
        return 0;
    }

    pthread_mutex_lock(&task->lock);
    task->write_ready = true;
    pthread_mutex_unlock(&task->lock);
    return 1;
}

int task_check_read_events(void) {
    return 0;
}

int task_has_read_watchers(void) {
    return 0;
}

static int task_wakeup_socket_readers(int sock_type, int sock_desc, int match_sock_id) {
    int sock_id = match_sock_id;

    /* Resolve sock_id from type+desc if not directly provided */
    if (sock_id < 0) {
        if (sock_type == SOCK_STREAM)
            sock_id = sock_id_from_tcp_desc(sock_desc);
        else if (sock_type == SOCK_DGRAM)
            sock_id = sock_id_from_udp_desc(sock_desc);
    }

    if (sock_id < 0 || sock_id >= SOCKS_MAX)
        return 0;

    uint32_t wake_node = 0;
    int worker_ready = 0;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = sock_to_task[sock_id];
    if (task == NULL) {
        pthread_mutex_unlock(&task_list_lock);
        return 0;
    }
    pthread_mutex_lock(&task->lock);
    pthread_mutex_unlock(&task_list_lock);
    if (!task->running || task->sock != sock_id || task->node <= 0) {
        pthread_mutex_unlock(&task->lock);
        return 0;
    }

    task->pending_main_rd = true;

    if (task->state == NET_TASK_PROCESS &&
        (task->cmd == SOCK_ACCEPT || task->cmd == SOCK_RECV || task->cmd == SOCK_RECVFROM)) {
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        worker_ready = 1;
    } else if (task->read_state == NET_TASK_PROCESS) {
        task->read_state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        worker_ready = 1;
    } else if (task->read_state == NET_TASK_IDLE) {
        if (task->thread_started && !task->read_cache_ready) {
            task->read_prefetch = true;
            task->read_state = NET_TASK_START;
            sched_wakeup(&task->wait_ctx);
            worker_ready = 1;
        } else {
            wake_node = task->node;
        }
    }
    pthread_mutex_unlock(&task->lock);

    if (wake_node > 0) {
        /* Deferred: callers hold the stack mutex; a direct vfs_wakeup here
         * can livelock netd<->vfsd (see task_queue_vfs_wakeup). */
        task_queue_vfs_wakeup(wake_node, VFS_EVT_RD);
    }
    return wake_node ? 1 : worker_ready;
}

int task_wakeup_tcp_readers(int tcp_desc) {
    if (tcp_desc < 0) {
        return 0;
    }
    return task_wakeup_socket_readers(SOCK_STREAM, tcp_desc, -1);
}

int task_wakeup_udp_readers(int udp_desc) {
    if (udp_desc < 0) {
        return 0;
    }
    return task_wakeup_socket_readers(SOCK_DGRAM, udp_desc, -1);
}

int task_wakeup_raw_readers(int sock_id) {
    if (sock_id < 0) {
        return 0;
    }
    return task_wakeup_socket_readers(-1, -1, sock_id);
}

int task_wakeup_tcp_writers(int tcp_desc) {
    if (tcp_desc < 0)
        return 0;

    int sock_id = sock_id_from_tcp_desc(tcp_desc);
    if (sock_id < 0 || sock_id >= SOCKS_MAX)
        return 0;

    uint32_t wake_node = 0;
    int worker_ready = 0;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = sock_to_task[sock_id];
    if (task == NULL) {
        pthread_mutex_unlock(&task_list_lock);
        return 0;
    }
    pthread_mutex_lock(&task->lock);
    pthread_mutex_unlock(&task_list_lock);
    if (!task->running || task->sock != sock_id || task->node <= 0) {
        pthread_mutex_unlock(&task->lock);
        return 0;
    }

    if (task->write_state == NET_TASK_PROCESS) {
        task->write_ready = true;
        task->write_state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        worker_ready = 1;
    } else if (task->state == NET_TASK_PROCESS && task->cmd == SOCK_CONNECT) {
        task->write_ready = true;
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        worker_ready = 1;
    } else {
        task->write_ready = true;
        wake_node = task->node;
    }
    pthread_mutex_unlock(&task->lock);

    if (wake_node > 0) {
        /* Deferred: callers hold the stack mutex; a direct vfs_wakeup here
         * can livelock netd<->vfsd (see task_queue_vfs_wakeup). */
        task_queue_vfs_wakeup(wake_node, VFS_EVT_WR);
    }
    return wake_node ? 1 : worker_ready;
}

static uint32_t task_finish_wakeup_event(net_task_t *task, bool is_read_op) {
    if(is_read_op) {
        return VFS_EVT_RD;
    }

    if(task->cmd == SOCK_SEND) {
        uint32_t saved_offset = task->out.offset;
        int ret = proto_read_int(&task->out);
        int sock_errno = 0;
        if(ret < 0 && task->out.size > task->out.offset) {
            sock_errno = proto_read_int(&task->out);
        }
        task->out.offset = saved_offset;
        /*
         * A completed async send that still reports EAGAIN/EINTR is NOT a real
         * writable edge. Waking WR here makes userspace poll(POLLOUT) return
         * immediately, retry write(), get EAGAIN again, and spin forever. Real
         * write readiness comes from task_wakeup_tcp_writers() when ACK/window
         * updates reopen the TCP send path.
         */
        if(ret < 0 && (sock_errno == 0 || sock_errno == EAGAIN || sock_errno == EINTR)) {
            return 0;
        }
    }

    switch(task->cmd) {
        case SOCK_RECV:
        case SOCK_RECVFROM:
        case SOCK_ACCEPT:
            return VFS_EVT_RD;
        case SOCK_SEND:
        case SOCK_SENDTO:
        case SOCK_CONNECT:
            return VFS_EVT_WR;
        default:
            return VFS_EVT_WR;
    }
}

static void* task_thread(void* arg){
    net_task_t *task = (net_task_t *)arg;
    pthread_mutex_lock(&task->lock);
    if(task->state != NET_TASK_START) {
        PF->clear(&task->in);
        PF->clear(&task->out);
        task->state = NET_TASK_IDLE;
    }
    if(task->read_state != NET_TASK_START) {
        PF->clear(&task->read_in);
        PF->clear(&task->read_out);
        task->read_state = NET_TASK_IDLE;
    }
    if(task->write_state != NET_TASK_START) {
        PF->clear(&task->write_in);
        PF->clear(&task->write_out);
        task->write_state = NET_TASK_IDLE;
    }

    while(1){
        bool run_main = false;
        bool run_read = false;
        bool run_write = false;
        bool main_completed = false;
        bool read_completed = false;
        bool write_completed = false;

        while(task->running &&
              task->state != NET_TASK_START &&
              task->read_state != NET_TASK_START &&
              task->write_state != NET_TASK_START) {
            sched_sleep(&task->wait_ctx, (mutex_t*)&task->lock, NULL);
        }

        if(!task->running) {
            pthread_mutex_unlock(&task->lock);
            break;
        }

        if(task->state == NET_TASK_START) {
            task->state = NET_TASK_PROCESS;
            run_main = true;
        }
        if(task->read_state == NET_TASK_START) {
            task->read_state = NET_TASK_PROCESS;
            run_read = true;
        }
        if(task->write_state == NET_TASK_START) {
            task->write_state = NET_TASK_PROCESS;
            run_write = true;
        }
        pthread_mutex_unlock(&task->lock);

        /*
         * No pre-op sticky-edge clear is performed. It is unnecessary and
         * racy for sockets: network_check_poll_events() gates WR/RD on the
         * LIVE sock_writable()/sock_readable() state, and vfs_get_poll_events()
         * replaces the sticky RW bits with that live state (auto-dropping stale
         * ones). Clearing here asynchronously from the worker could instead wipe
         * a genuine WR/RD edge that an ACK-driven task_wakeup_tcp_writers() (or
         * a completion wakeup) just posted for a blocked poll() waiter, stranding
         * the client forever (mid-stream cat|dump hang under concurrency).
         */
        if(run_main) {
            main_completed = do_network_fcntl(task) ? true : false;
        }
        if(run_read) {
            read_completed = do_network_read(task) ? true : false;
        }
        if(run_write) {
            write_completed = do_network_write(task) ? true : false;
        }

        if(run_main && main_completed) {
            /*
             * Compute the wake event while still holding task->lock.
             * task_finish_wakeup_event() for SOCK_SEND peeks task->out via
             * proto_read_int(), which transiently advances task->out.offset
             * (then restores it). task_write()'s FINISH collect reads the same
             * task->out under task->lock. On SMP, running the peek AFTER
             * unlocking races that collect: the client can read task->out while
             * offset is mid-peek and pick up the errno field (0) instead of the
             * byte count, yielding a phantom write()==0. telnetd treats a 0-byte
             * write as fatal and tears the relay down -> first-connection stall/
             * mid-stream close. Serialize the peek under the lock to fix it.
             */
            pthread_mutex_lock(&task->lock);
            task->state = NET_TASK_FINISH;
            uint32_t wake_event = task_finish_wakeup_event(task, false);
            /*
             * Lost-wakeup guard for the SEND path.
             *
             * task_finish_wakeup_event() returns 0 for a SEND that completed
             * with EAGAIN (send window was closed) to avoid a poll(POLLOUT)
             * busy spin. But the window may have re-opened WHILE this send was
             * in PROCESS: the ACK-driven task_wakeup_tcp_writers() then raised
             * VFS_EVT_WR, the blocked client consumed that edge, retried, saw
             * state==PROCESS, returned to sleep and cleared the sticky WR bit.
             * If that same ACK also drained the last inflight byte, no further
             * ACK will ever arrive, so the client would sleep on WR forever
             * (telnetd relay stalls -> pipe backs up -> cat|dump hangs).
             *
             * Re-check real writability here and re-fire the WR edge when the
             * window is genuinely open. sock_writable() takes the tcp mutex, so
             * it MUST run after releasing task->lock: the ACK path locks in
             * the opposite order (tcp mutex -> task->lock/task_list_lock) and holding both
             * here in reverse would deadlock on SMP. Gating on sock_writable()
             * means the client's retry actually sends bytes, so it cannot spin.
             */
            int send_recheck_sock = (wake_event == 0 &&
                                     task->cmd == SOCK_SEND &&
                                     task->sock >= 0) ? task->sock : -1;
            pthread_mutex_unlock(&task->lock);
            if(send_recheck_sock >= 0 && sock_writable(send_recheck_sock)) {
                wake_event = VFS_EVT_WR;
            }
            if(wake_event != 0) {
                task_queue_vfs_wakeup(task->node, wake_event);
            }
        }
        if(run_read && read_completed) {
            pthread_mutex_lock(&task->lock);
            uint32_t wake_event;
            if(task->read_prefetch) {
                task->read_prefetch = false;
                task->read_state = NET_TASK_IDLE;
                task->read_cache_ready = true;
                wake_event = VFS_EVT_RD;
            } else {
                task->read_state = NET_TASK_FINISH;
                wake_event = task_finish_wakeup_event(task, true);
            }
            pthread_mutex_unlock(&task->lock);
            if(wake_event != 0) {
                task_queue_vfs_wakeup(task->node, wake_event);
            }
        }
        if(run_write && write_completed) {
            /*
             * Async-accepted write fully drained (or hard-errored into
             * write_err): free the slot and fire WR so a client parked in
             * VFS_ERR_RETRY re-issues its write immediately. The retry is
             * accepted regardless of the TCP window (it only copies into
             * write_in), so this wakeup cannot cause a poll spin.
             */
            pthread_mutex_lock(&task->lock);
            PF->clear(&task->write_in);
            PF->clear(&task->write_out);
            task->write_off = 0;
            task->write_state = NET_TASK_IDLE;
            pthread_mutex_unlock(&task->lock);
            task_queue_vfs_wakeup(task->node, VFS_EVT_WR);
        }

        pthread_mutex_lock(&task->lock);
    }

    /*
     * Self-reap: this runs in the connection's own detached thread after
     * release_task() flagged running=false. Remove from task_list first (under
     * the lock the protocol thread uses to scan for wakeups) so no wakeup can
     * observe the task once we start freeing it. sock_close() may spin in the
     * TCP graceful-close handshake, but that cost is paid here — never in the
     * shared IPC dispatch context that serves accept()/read()/write().
     */
    /*
     * Flush any async-accepted write still pending before closing: the
     * client's write() already returned success for those bytes (e.g. the
     * final SSH CLOSE/EXIT-STATUS packets right before sshd closes the fd);
     * dropping them makes scp report "End of file". Bounded so a dead peer
     * with a closed window cannot stall teardown for more than ~200ms —
     * comparable to the graceful-close spin below.
     */
    if(task->write_state != NET_TASK_IDLE && task->sock >= 0) {
        int flush_tries = 0;
        while(flush_tries < 40) {
            if(do_network_write(task) != 0)
                break; /* fully drained or hard error */
            flush_tries++;
            proc_usleep(5000);
        }
    }

    int fin_sock = task->sock;
    task->sock = -1;
    uint32_t close_node = task->node;
    uint32_t do_close_wakeup = task->pending_close_wakeup;
    task->pending_close_wakeup = 0;
    pthread_mutex_lock(&task_list_lock);
    if(fin_sock >= 0 && fin_sock < SOCKS_MAX && sock_to_task[fin_sock] == task)
        sock_to_task[fin_sock] = NULL;
    pthread_mutex_unlock(&task_list_lock);
    task_list_remove(task);
    if(fin_sock >= 0) {
        sock_close(fin_sock);
    }

    /*
     * Deliver CLOSE through the same deferred reverse-IPC path as RD/WR.
     * Even outside the stack mutex, a synchronous vfs_wakeup() here can still
     * stall behind vfsd->netd traffic and extend the window where clients sit
     * in FS_CMD_POLL timeouts with the socket half-torn-down.
     */
    if(do_close_wakeup && close_node > 0) {
        task_queue_vfs_wakeup(close_node, VFS_EVT_CLOSE);
    }

    PF->clear(&task->in);
    PF->clear(&task->out);
    PF->clear(&task->read_in);
    PF->clear(&task->read_out);
    PF->clear(&task->write_in);
    PF->clear(&task->write_out);

    sched_ctx_destroy(&task->wait_ctx);
    pthread_mutex_lock(&task_list_lock);
    if(task_active_count > 0)
        task_active_count--;
    task_total_freed++;
    pthread_mutex_unlock(&task_list_lock);
    pthread_mutex_destroy(&task->lock);
    free(task);

    return NULL;
}
