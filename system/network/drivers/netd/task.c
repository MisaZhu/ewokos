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
 
pthread_mutex_t task_list_lock;
net_task_t *task_list = NULL;

#ifndef SOCKS_MAX
#define SOCKS_MAX 128
#endif
static net_task_t* sock_to_task[SOCKS_MAX];

static int task_owner_pid(int from_pid) {
    int owner_pid = proc_getpid(from_pid);

    if(owner_pid > 0)
        return owner_pid;
    return from_pid;
}

static void task_list_add(net_task_t * task){
    pthread_mutex_lock(&task_list_lock);
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
}

net_task_t *create_task(int fd, int from_pid, int node){
    net_task_t *task = malloc(sizeof(net_task_t));
    if(task == NULL) {
        return NULL;
    }
    memset(task, 0 , sizeof(net_task_t));
    sched_ctx_init(&task->wait_ctx);
    task->fd = fd;
    task->node = node;
    task->from_pid = task_owner_pid(from_pid);
    task->state = NET_TASK_IDLE;
    task->read_from_pid = task->from_pid;
    task->read_state = NET_TASK_IDLE;
    task->sock = -1;
    task->refs = 1;
    task->running = true;
    task->thread_started = 1;
    task_list_add(task);
    if(pthread_create(&task->tid, NULL, task_thread, task) != 0) {
        task_list_remove(task);
        task->thread_started = 0;
        task->running = false;
        sched_ctx_destroy(&task->wait_ctx);
        free(task);
        return NULL;
    }
    /*
     * The worker self-reaps: teardown (sock_close + free) happens inside the
     * connection's own thread, never in the shared IPC dispatch context. Detach
     * so no one has to join it (join in the gate would freeze every other
     * connection and accept() behind the single per-server IPC slot).
     */
    pthread_detach(task->tid);
    return task;
}

void release_task(net_task_t *task){
    if(task == NULL)
        return;

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
    pthread_mutex_lock(&task_list_lock);
    task->running = false;
    int from_pid = task->from_pid;
    uint32_t node = task->node;
    sched_wakeup(&task->wait_ctx);
    pthread_mutex_unlock(&task_list_lock);

    /* Wake any client still blocked on this node so its close() returns. */
    if(from_pid > 0) {
        vfs_wakeup(node, VFS_EVT_CLOSE);
    }
}

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
    pthread_mutex_lock(&task_list_lock);
    
    if(task->state == NET_TASK_FINISH){
        if(cmd != task->cmd){
            pthread_mutex_unlock(&task_list_lock);
            return VFS_ERR_RETRY;
        }
        if(from_pid == task->from_pid) {
            PF->copy(out, task->out.data, task->out.size);
            PF->clear(&task->out);
            PF->clear(&task->in);
            task->state = NET_TASK_IDLE;
            pthread_mutex_unlock(&task_list_lock);
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
        uint32_t wait_event = task_arm_wait_event(cmd);
        if(wait_event != 0 && task->node > 0) {
            /*
             * Clear stale sticky readiness before arming a new async request.
             * Otherwise callers that block on RD/WR can immediately consume an
             * old edge and spin while the new operation is still in progress.
             */
            vfs_clear_poll_events(task->node, wait_event);
        }
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task_list_lock);
        // Signal task thread to wake up
    } else {
        pthread_mutex_unlock(&task_list_lock);
    }
    return VFS_ERR_RETRY;
}

int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p){
    from_pid = task_owner_pid(from_pid);
    pthread_mutex_lock(&task_list_lock);
    
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
            pthread_mutex_unlock(&task_list_lock);
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
        task->read_p = p;
        task->read_from_pid = from_pid;
        PF->clear(&task->read_in);
        PF->clear(&task->read_out);
        /*
         * Drop any stale readable edge before arming a new async read. The
         * socket remains level-trigger readable through network_check_poll_events(),
         * while the in-flight read completes via an explicit wakeup on FINISH.
         */
        vfs_clear_poll_events(task->node, VFS_EVT_RD);
        PF->addi(&task->read_in, size);
        task->read_state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task_list_lock);
    } else {
        pthread_mutex_unlock(&task_list_lock);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
    from_pid = task_owner_pid(from_pid);
    pthread_mutex_lock(&task_list_lock);
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_SEND != task->cmd){
            pthread_mutex_unlock(&task_list_lock);
            return VFS_ERR_RETRY;
        }
        if(from_pid == task->from_pid) {
            int ret = proto_read_int(&task->out);
            int sock_errno = 0;
            if(ret < 0 && task->out.size > task->out.offset) {
                sock_errno = proto_read_int(&task->out);
            }
            PF->clear(&task->out);
            PF->clear(&task->in);
            task->state = NET_TASK_IDLE;
            if(ret < 0 && (sock_errno == 0 || sock_errno == EAGAIN || sock_errno == EINTR)) {
                /*
                 * Fall through to re-arm with the caller's new buf/size.
                 * If we returned VFS_ERR_RETRY here without re-arming, the
                 * task sits IDLE with no pending operation.  On fast links
                 * (especially loopback) the ACK that opened the send window
                 * may have already fired task_wakeup_tcp_writers() → VFS_EVT_WR
                 * BEFORE the client enters its clear/retry/block sequence.
                 * The client clears the stale event, retries (arms here), then
                 * blocks — but with an early return the arm never happens and
                 * the client blocks forever with no wake source.
                 */
            } else {
                pthread_mutex_unlock(&task_list_lock);
                return ret;
            }
        }
        /*
         * Writes on duplicated or fork-inherited socket FDs may arrive from a
         * different pid than the one that completed the previous async send.
         * Re-arm the new write immediately instead of returning a bare RETRY,
         * otherwise the caller can block on WR with no in-flight send to
         * generate the next wakeup edge.
         */
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
    }

    if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_SEND;	
        task->p = p;
        task->from_pid = from_pid;

        PF->clear(&task->in);
        PF->clear(&task->out);
        PF->add(&task->in, buf, size);
        /*
         * Drop any stale writable edge from a previous completed send before
         * arming a new async send. Otherwise userspace may consume the old
         * WR event, retry immediately, and race this new send in PROCESS.
         */
        vfs_clear_poll_events(task->node, VFS_EVT_WR);
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task_list_lock);
        // Signal task thread to wake up
    } else {
        pthread_mutex_unlock(&task_list_lock);
    }
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
			task->sock = sock;
			if(sock >= 0 && sock < SOCKS_MAX) {
				pthread_mutex_lock(&task_list_lock);
				sock_to_task[sock] = task;
				pthread_mutex_unlock(&task_list_lock);
			}
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
					sock_errno = EAGAIN;
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
			task->sock = sock;
			if(sock >= 0 && sock < SOCKS_MAX) {
				pthread_mutex_lock(&task_list_lock);
				sock_to_task[sock] = task;
				pthread_mutex_unlock(&task_list_lock);
			}
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
    if (task == NULL || !task->running || task->sock != sock_id || task->node <= 0) {
        pthread_mutex_unlock(&task_list_lock);
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
        wake_node = task->node;
    }
    pthread_mutex_unlock(&task_list_lock);

    if (wake_node > 0) {
        vfs_wakeup(wake_node, VFS_EVT_RD);
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
    if (task == NULL || !task->running || task->sock != sock_id || task->node <= 0) {
        pthread_mutex_unlock(&task_list_lock);
        return 0;
    }

    if (task->state == NET_TASK_PROCESS && task->cmd == SOCK_CONNECT) {
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        worker_ready = 1;
    } else {
        wake_node = task->node;
    }
    pthread_mutex_unlock(&task_list_lock);

    if (wake_node > 0) {
        vfs_wakeup(wake_node, VFS_EVT_WR);
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
    pthread_mutex_lock(&task_list_lock);
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

    while(1){
        bool run_main = false;
        bool run_read = false;
        bool main_completed = false;
        bool read_completed = false;

        while(task->running &&
              task->state != NET_TASK_START &&
              task->read_state != NET_TASK_START) {
            sched_sleep(&task->wait_ctx, (mutex_t*)&task_list_lock, NULL);
        }

        if(!task->running) {
            pthread_mutex_unlock(&task_list_lock);
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
        pthread_mutex_unlock(&task_list_lock);

        if(run_main) {
            main_completed = do_network_fcntl(task) ? true : false;
        }
        if(run_read) {
            read_completed = do_network_read(task) ? true : false;
        }

        if(run_main && main_completed) {
            pthread_mutex_lock(&task_list_lock);
            task->state = NET_TASK_FINISH;
            pthread_mutex_unlock(&task_list_lock);
            uint32_t wake_event = task_finish_wakeup_event(task, false);
            if(wake_event != 0) {
                vfs_wakeup(task->node, wake_event);
            }
        }
        if(run_read && read_completed) {
            pthread_mutex_lock(&task_list_lock);
            task->read_state = NET_TASK_FINISH;
            pthread_mutex_unlock(&task_list_lock);
            uint32_t wake_event = task_finish_wakeup_event(task, true);
            if(wake_event != 0) {
                vfs_wakeup(task->node, wake_event);
            }
        }

        pthread_mutex_lock(&task_list_lock);
    }

    /*
     * Self-reap: this runs in the connection's own detached thread after
     * release_task() flagged running=false. Remove from task_list first (under
     * the lock the protocol thread uses to scan for wakeups) so no wakeup can
     * observe the task once we start freeing it. sock_close() may spin in the
     * TCP graceful-close handshake, but that cost is paid here — never in the
     * shared IPC dispatch context that serves accept()/read()/write().
     */
    int fin_sock = task->sock;
    task->sock = -1;
    pthread_mutex_lock(&task_list_lock);
    if(fin_sock >= 0 && fin_sock < SOCKS_MAX)
        sock_to_task[fin_sock] = NULL;
    pthread_mutex_unlock(&task_list_lock);
    task_list_remove(task);
    if(fin_sock >= 0) {
        sock_close(fin_sock);
    }

    PF->clear(&task->in);
    PF->clear(&task->out);
    PF->clear(&task->read_in);
    PF->clear(&task->read_out);

    sched_ctx_destroy(&task->wait_ctx);
    free(task);

    return NULL;
}
