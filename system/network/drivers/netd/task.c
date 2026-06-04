#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>

#include "task.h"
#include "netd.h"

extern int sock_readable(int sock);

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
    pthread_cond_init(&task->cond, NULL);
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
    pthread_cond_signal(&task->cond);
    pthread_mutex_unlock(&task_list_lock);
    // Set running = false to signal thread to exit
    // Save values needed outside of lock
    int from_pid = task->from_pid;
    bool is_read_task = task->is_read_task;
    int sock = task->sock;
    int thread_started = task->thread_started;

    klog("[netd] release_task begin: task=%p fd=%d node=%d sock=%d is_read=%d running=%d thread_started=%d\n",
        task, task->fd, task->node, sock, is_read_task, task->running, thread_started);

    // Also wake up waiting client if task was processing
    if(from_pid > 0) {
        vfs_wakeup(task->node, VFS_EVT_CLOSE);
    }

    // Wait for main task thread to exit before destroying cond and freeing
    if(thread_started){
        pthread_join(task->tid, NULL);
    }
    pthread_cond_destroy(&task->cond);

    // Now close the socket if this is not a read task
    if(!is_read_task) {
        klog("[netd] release_task sock_close: task=%p sock=%d\n", task, sock);
        sock_close(sock);
    }
    // Now safe to destroy condition variable after thread has exited
    klog("[netd] release_task done: task=%p fd=%d node=%d sock=%d\n",
        task, task->fd, task->node, sock);
    free(task); // We free main task here
}

int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p){
    pthread_mutex_lock(&task_list_lock);
    
    if(task->state == NET_TASK_FINISH){
        if(cmd != task->cmd || from_pid != task->from_pid){
            pthread_mutex_unlock(&task_list_lock);
            return VFS_ERR_RETRY;
        }
        PF->copy(out, task->out.data, task->out.size);
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
        pthread_mutex_unlock(&task_list_lock);
        return 0;
    }else if(task->state == NET_TASK_IDLE){
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
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task_list_lock);
        // Signal task thread to wake up
    } else {
        pthread_mutex_unlock(&task_list_lock);
    }
    return VFS_ERR_RETRY;
}

int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p){
    if(!task->is_read_task){
        klog("[netd] task_read reject non-read-task: task=%p pid=%d node=%d\n", task, from_pid, task->node);
        return VFS_ERR_RETRY;
    }

    pthread_mutex_lock(&task_list_lock);
	klog("[netd] task_read enter: task=%p pid=%d node=%d state=%d sock=%d size=%d\n",
		task, from_pid, task->node, task->state, task->sock, size);
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_RECV != task->cmd) {
            pthread_mutex_unlock(&task_list_lock);
            klog("[netd] task_read finish but cmd mismatch: task=%p cmd=%d\n", task, task->cmd);
            return VFS_ERR_RETRY;
        }
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
            klog("[netd] task_read finish retry: task=%p len=%d errno=%d\n", task, len, sock_errno);
            return VFS_ERR_RETRY;
        }
        klog("[netd] task_read finish return: task=%p len=%d errno=%d\n", task, len, sock_errno);
        return len;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_RECV;	
        task->p = p;
        task->from_pid = from_pid;
        PF->clear(&task->in);
        PF->clear(&task->out);
        PF->addi(&task->in, size);
        task->state = NET_TASK_START;
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task_list_lock);
        klog("[netd] task_read start recv: task=%p pid=%d node=%d size=%d\n",
			task, from_pid, task->node, size);
    } else {
        pthread_mutex_unlock(&task_list_lock);
        klog("[netd] task_read busy retry: task=%p state=%d cmd=%d\n", task, task->state, task->cmd);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
    pthread_mutex_lock(&task_list_lock);
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_SEND != task->cmd || from_pid != task->from_pid){
            pthread_mutex_unlock(&task_list_lock);
            return VFS_ERR_RETRY;
        }
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
            klog("[netd] task_write finish retry: task=%p len=%d errno=%d\n", task, ret, sock_errno);
            return VFS_ERR_RETRY;
        }
        klog("[netd] task_write finish return: task=%p len=%d errno=%d\n", task, ret, sock_errno);
        return ret;
    }else if(task->state == NET_TASK_IDLE){
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
        klog("[netd] task_write start send: task=%p pid=%d node=%u size=%d sock=%d\n",
                task, from_pid, task->node, size, task->sock);
        pthread_cond_signal(&task->cond);
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

    static uint32_t cnt = 0;
    //klog("%d:sock_network_fcntl sock:%d, node:%p, cmd:%d\n", cnt++, sock, task->node, task->cmd);
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
				klog("[netd] do_network_fcntl SOCK_SEND enter: task=%p fd=%d node=%d sock=%d size=%d\n",
					task, task->fd, task->node, sock, size);
			    ret = sock_send(sock, data, size);
				sock_errno = errno;
				klog("[netd] do_network_fcntl SOCK_SEND return: task=%p fd=%d node=%d sock=%d ret=%d errno=%d\n",
					task, task->fd, task->node, sock, ret, sock_errno);
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
            klog("[netd] SOCK_CLOSE: task=%p fd=%d node=%d sock=%d\n",
                task, task->fd, task->node, sock);
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
            pthread_cond_wait(&task->cond, &task_list_lock);
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
