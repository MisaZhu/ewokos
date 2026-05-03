#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>

#include "task.h"
#include "netd.h"

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
    memset(task, 0 , sizeof(net_task_t));
    task->fd = fd;
    task->node = node;
    task->state = NET_TASK_IDLE;
    task->sock = -1;
    task->running = true;
    task_list_add(task);
    return task;
}

void release_task(net_task_t *task){
    if(task == NULL)
        return;
    
    task->running = false;
    // Set running = false to signal thread to exit
    // Save values needed outside of lock
    net_task_t* read_task = task->read_task;
    int from_pid = task->from_pid;
    bool is_read_task = task->is_read_task;
    int sock = task->sock;
    int thread_started = task->thread_started;

    // Also wake up waiting client if task was processing
    if(from_pid > 0) {
        vfs_wakeup(task->node, VFS_EVT_CLOSE);
    }

    // Wait for main task thread to exit before destroying cond and freeing
    if(thread_started){
        pthread_join(task->tid, NULL);
    }

    // Now close the socket if this is not a read task
    if(!is_read_task) {
        sock_close(sock);
    }
    // Now safe to destroy condition variable after thread has exited
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
        PF->init(&task->in);
        PF->init(&task->out);
        PF->copy(&task->in, in->data, in->size);
        task->state = NET_TASK_START;
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
        int len = proto_read_int(&task->out);
        if(len > 0){
            proto_read_to(&task->out, buf, len);
        }
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
        pthread_mutex_unlock(&task_list_lock);
        return len;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_RECV;	
        task->p = p;
        task->from_pid = from_pid;
        PF->init(&task->in);
        PF->init(&task->out);
        PF->addi(&task->in, size);
        task->state = NET_TASK_START;
        pthread_mutex_unlock(&task_list_lock);
        // Signal task thread to wake up
    } else {
        pthread_mutex_unlock(&task_list_lock);
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
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE;
        pthread_mutex_unlock(&task_list_lock);
        return ret;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_SEND;	
        task->p = p;
        task->from_pid = from_pid;

        PF->init(&task->in);
        PF->init(&task->out);
        PF->add(&task->in, buf, size);
        task->state = NET_TASK_START;
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
				ret = sock_sendto(sock, data, size, paddr, addrlen);
			}
			PF->addi(&task->out, ret);
			break;
		case SOCK_RECVFROM:
			size = proto_read_int(&task->in);
            size = size < TASK_READ_BUF_SIZE ? size:TASK_READ_BUF_SIZE;
            ret = sock_recvfrom(sock, task->read_buf, size, &addr, &addrlen);
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->read_buf, ret);
                PF->add(&task->out, &addr, addrlen);	
            }
			break;
		case SOCK_SEND:
			data = proto_read(&task->in, &size);
            if(data && size){
			    ret = sock_send(sock, data, size);
            } 
			PF->addi(&task->out, ret);
			break;
		case SOCK_RECV:
			size = proto_read_int(&task->in);
            size = size < TASK_READ_BUF_SIZE ? size:TASK_READ_BUF_SIZE;
            ret = sock_recv(sock, task->read_buf, size);
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->read_buf, ret);
            }
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
				ret = sock_connect(sock, paddr, addrlen);
			}
			PF->addi(&task->out, ret);
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

static void* task_thread(void* arg){
    net_task_t *task = (net_task_t *)arg;

    pthread_mutex_lock(&task_list_lock);
    if(task->state != NET_TASK_START) {
        PF->init(&task->in);
        PF->init(&task->out);
        task->state = NET_TASK_IDLE;
    }
    pthread_mutex_unlock(&task_list_lock);

    vfs_wakeup(task->node, VFS_EVT_RW);

    while(1){
        pthread_mutex_lock(&task_list_lock);

        if(!task->running) {
            pthread_mutex_unlock(&task_list_lock);
            break;
        }

        if(task->state == NET_TASK_START) {
            pthread_mutex_unlock(&task_list_lock);
            task->state = NET_TASK_PROCESS;

            do_network_fcntl(task);

            pthread_mutex_lock(&task_list_lock);
            task->state = NET_TASK_FINISH;
            pthread_mutex_unlock(&task_list_lock);
        } else if(task->state == NET_TASK_FINISH) {
            pthread_mutex_unlock(&task_list_lock);
            vfs_wakeup(task->node, VFS_EVT_RW);
            usleep(1000);
        } else {
            pthread_mutex_unlock(&task_list_lock);
            usleep(1000);
        }
    }

    PF->clear(&task->in);
    PF->clear(&task->out);

    return NULL;
}
