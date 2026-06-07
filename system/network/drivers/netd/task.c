#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>

#include "task.h"
#include "netd.h"

extern int sock_readable(int sock);
extern int sock_get_desc(int id);
extern int sock_get_type(int id);
extern int sched_ctx_init(struct sched_ctx *ctx);
extern int sched_ctx_destroy(struct sched_ctx *ctx);
extern int sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timeval *abstime);
extern int sched_wakeup(struct sched_ctx *ctx);

#define TASK_POLL_INTERVAL_US 1000 /* 1ms */

static void* task_thread(void* arg);
 
pthread_mutex_t task_list_lock;
net_task_t *task_list = NULL;

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
    ipc_disable();
    net_task_t *task = task_list;
    while(task!=NULL){
        net_task_t* next = task->next;
        if(!task->running) {
            // Remove task from list safely while holding lock
            task_list_remove(task);
            // Unlock before releasing the task
            release_task(task);
        }
        else if(!task->thread_started)  {
            pthread_create(&task->tid, NULL, task_thread, task);
            task->thread_started = 1;
        }
        task = next;
    }
    ipc_enable();
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
    task->state = NET_TASK_IDLE;
    task->sock = -1;
    task->refs = 1;
    task->running = true;
    task_list_add(task);
    return task;
}

void release_task(net_task_t *task){
    if(task == NULL)
        return;
    
    pthread_mutex_lock(&task_list_lock);
    task->running = false;
    sched_wakeup(&task->wait_ctx);
    pthread_mutex_unlock(&task_list_lock);
    // Set running = false to signal thread to exit
    // Save values needed outside of lock
    int from_pid = task->from_pid;
    bool is_read_task = task->is_read_task;
    int sock = task->sock;
    int thread_started = task->thread_started;

    // Also wake up waiting client if task was processing
    if(from_pid > 0) {
        vfs_wakeup(task->node, VFS_EVT_CLOSE);
    }

    /*
     * A blocked socket worker must be forced out of send/recv before join.
     * Joining first can strand the close path forever on a thread still
     * sleeping inside the stack, leaving the guest side in CLOSE_WAIT and
     * poisoning later telnet sessions.
     */
    if(!is_read_task && sock >= 0) {
        sock_close(sock);
        sock = -1;
    }

    // Wait for main task thread to exit before destroying cond and freeing
    if(thread_started){
        pthread_join(task->tid, NULL);
    }
    sched_ctx_destroy(&task->wait_ctx);

    // Now safe to destroy condition variable after thread has exited
    free(task); // We free main task here
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
    if(!task->is_read_task){
        return VFS_ERR_RETRY;
    }

    pthread_mutex_lock(&task_list_lock);
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_RECV != task->cmd) {
            pthread_mutex_unlock(&task_list_lock);
            return VFS_ERR_RETRY;
        }
        if(from_pid == task->from_pid) {
            int len = proto_read_int(&task->out);
            int sock_errno = 0;
            if(len > 0){
                proto_read_to(&task->out, buf, len);
            }
            if(len <= 0 && task->out.size > task->out.offset) {
                sock_errno = proto_read_int(&task->out);
            }
            PF->clear(&task->out);
            PF->clear(&task->in);
            task->state = NET_TASK_IDLE;
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
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
    }

    if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_RECV;	
        task->p = p;
        task->from_pid = from_pid;
        PF->clear(&task->in);
        PF->clear(&task->out);
        PF->addi(&task->in, size);
        task->state = NET_TASK_START;
        sched_wakeup(&task->wait_ctx);
        pthread_mutex_unlock(&task_list_lock);
    } else {
        pthread_mutex_unlock(&task_list_lock);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
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
            pthread_mutex_unlock(&task_list_lock);
            if(ret < 0 && (sock_errno == 0 || sock_errno == EAGAIN || sock_errno == EINTR)) {
                return VFS_ERR_RETRY;
            }
            return ret;
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
			ret = sock_accept(sock, &addr, &addrlen);
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
			PF->addi(&task->out, 0);
			break;
		case SOCK_CONNECT:
			paddr = proto_read(&task->in, &addrlen);
			if(paddr == NULL) {
				ret = -1;
			} else {
				errno = 0;
				ret = sock_connect(sock, paddr, addrlen);
				sock_errno = errno;
				if(ret < 0 && sock_errno == 0)
					sock_errno = EAGAIN;
			}
			PF->addi(&task->out, ret);
			PF->addi(&task->out, ret < 0 ? sock_errno : 0);
			break;
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
    return 0;
}

int task_check_read_events(void) {
    int ready = 0;
    enum { MAX_WAKE_CANDIDATES = 64 };
    struct {
        int sock;
        uint32_t node;
    } wake_list[MAX_WAKE_CANDIDATES];
    int wake_count = 0;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = task_list;
    while(task != NULL) {
        if(task->read_task != NULL &&
           task->read_task->is_read_task &&
           task->read_task->state == NET_TASK_IDLE &&
           task->read_task->sock >= 0 &&
           task->read_task->node > 0) {
            if(wake_count < MAX_WAKE_CANDIDATES) {
                wake_list[wake_count].sock = task->read_task->sock;
                wake_list[wake_count].node = task->read_task->node;
                wake_count++;
            }
        }
        task = task->next;
    }
    pthread_mutex_unlock(&task_list_lock);

    for(int i = 0; i < wake_count; i++) {
        if(sock_readable(wake_list[i].sock)) {
            /*
             * Do not prefetch into read_task->out. Interactive users like shell
             * often read one byte at a time, and a background SOCK_RECV would
             * consume the remaining bytes before user space asks for them.
             */
            vfs_wakeup(wake_list[i].node, VFS_EVT_RD);
            ready = 1;
        }
    }
    return ready;
}

int task_has_read_watchers(void) {
    int has_watchers = 0;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = task_list;
    while(task != NULL) {
        if(task->read_task != NULL &&
           task->read_task->running &&
           task->read_task->state == NET_TASK_IDLE &&
           task->read_task->sock >= 0 &&
           task->read_task->node > 0) {
            has_watchers = 1;
            break;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&task_list_lock);
    return has_watchers;
}

int task_wakeup_tcp_readers(int tcp_desc) {
    enum { MAX_WAKE_CANDIDATES = 64 };
    uint32_t wake_nodes[MAX_WAKE_CANDIDATES];
    int wake_count = 0;

    if (tcp_desc < 0) {
        return 0;
    }

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = task_list;
    while (task != NULL) {
        int task_sock = task->sock;
        uint32_t wake_node = 0;

        if (task_sock >= 0 &&
            sock_get_type(task_sock) == SOCK_STREAM &&
            sock_get_desc(task_sock) == tcp_desc) {
            if (task->node > 0) {
                /*
                 * TCP data may arrive while a previous async read is still in
                 * PROCESS/FINISH, before user space issues the next poll().
                 * Preserve a sticky RD edge on the socket node itself so the
                 * next poll()/read pair still observes readable data.
                 */
                task->pending_main_rd = true;
                wake_node = task->node;
            }
        }

        if (wake_node > 0 && wake_count < MAX_WAKE_CANDIDATES) {
            wake_nodes[wake_count++] = wake_node;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&task_list_lock);

    for (int i = 0; i < wake_count; i++) {
        vfs_wakeup(wake_nodes[i], VFS_EVT_RD);
    }
    return wake_count;
}

static uint32_t task_finish_wakeup_event(const net_task_t *task) {
    if(task->is_read_task) {
        return VFS_EVT_RD;
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

    while(1){
        while(task->running && task->state != NET_TASK_START) {
            sched_sleep(&task->wait_ctx, (mutex_t*)&task_list_lock, NULL);
        }

        if(!task->running) {
            pthread_mutex_unlock(&task_list_lock);
            break;
        }

        task->state = NET_TASK_PROCESS;
        pthread_mutex_unlock(&task_list_lock);

        do_network_fcntl(task);

        pthread_mutex_lock(&task_list_lock);
        task->state = NET_TASK_FINISH;
        pthread_mutex_unlock(&task_list_lock);
        vfs_wakeup(task->node, task_finish_wakeup_event(task));
        pthread_mutex_lock(&task_list_lock);
    }

    PF->clear(&task->in);
    PF->clear(&task->out);

    return NULL;
}
