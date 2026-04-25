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

net_task_t *task_list = NULL;

// Mutex to protect task list and state
static pthread_mutex_t task_list_lock;
static int task_list_lock_initialized = 0;

static void task_list_add(net_task_t * task){
    if(task_list == NULL){
        task_list = task;
        task->next = NULL;
    }else{
        net_task_t *t = task_list;

        while(t->next != NULL){
            t = t->next;
        }
        t->next = task;
        task->next = NULL;
    }
}

void start_task(void){
    if(!task_list_lock_initialized){
        pthread_mutex_init(&task_list_lock, NULL);
        task_list_lock_initialized = 1;
    }
    
    ipc_disable();
    net_task_t *task = task_list;
    while(task!=NULL){
       if(!task->thread_started){
           pthread_create(&task->tid, NULL, task_thread, task);
           task->thread_started = 1;
       }
       task = task->next;
    }
    task_list = NULL;
    ipc_enable();
}

net_task_t *create_task(int fd, int from_pid, int node){
    net_task_t *task = malloc(sizeof(net_task_t));
    memset(task, 0 , sizeof(net_task_t));
    task->fd = fd;
    task->from_pid = 0;
    task->node = node;
    task->running = false;
    task->read_task = NULL;
    task->state = NET_TASK_IDLE;
    task->sock = -1;
    task->thread_started = 0;
    task_list_add(task);
    return task;
}

void release_task(net_task_t *task){
    pthread_mutex_lock(&task_list_lock);
    
    // Set state first, then wake up to ensure thread sees state change
    task->running = false;
    
    // Save values needed outside of lock
    net_task_t* read_task = task->read_task;
    int from_pid = task->from_pid;
    pthread_t main_tid = task->tid;
    int main_thread_started = task->thread_started;
    bool is_read_task = task->is_read_task;
    int sock = task->sock;
    pthread_t read_tid = 0;
    int read_thread_started = 0;
    if(read_task != NULL){
        read_task->running = false;
        read_tid = read_task->tid;
        read_thread_started = read_task->thread_started;
    }
    
    pthread_mutex_unlock(&task_list_lock);
    
    // Wake up outside lock
    if(read_task != NULL){
        klog("read_task: %d, %d\n", read_task->tid, read_thread_started);
        proc_wakeup((uint32_t)read_task);
        proc_wakeup_pid(read_task->from_pid, RW_BLOCK_EVT);
        // If read_task thread is started, wait for it to exit
        if(read_thread_started){
            klog("pthread_join: %d\n", read_tid);
            pthread_join(read_tid, NULL);
            klog("pthread_join: %d done\n", read_tid);
            free(read_task); // We free read_task here
        }
    }
    
    // Wake up the task thread itself
    proc_wakeup((uint32_t)task);
    
    // Also wake up waiting client if task was processing
    if(from_pid > 0) {
        proc_wakeup_pid(from_pid, RW_BLOCK_EVT);
    }

    // Wait for task thread to exit
    if(main_thread_started){
        pthread_join(main_tid, NULL);
    }

    // Now close the socket if this is not a read task
    if(!is_read_task) {
        sock_close(sock);
    }
    
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
        // Unlock first then wake up to avoid deadlock
        proc_wakeup((uint32_t)task);
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
        if(from_pid != task->from_pid || SOCK_RECV != task->cmd) {
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
        // Unlock first then wake up to avoid deadlock
        proc_wakeup((uint32_t)task);
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
        // Unlock first then wake up to avoid deadlock
        proc_wakeup((uint32_t)task);
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
		default:
			break;
	}
    return 0;
}

static void* task_thread(void* arg){
    net_task_t *task = (net_task_t *)arg;

    pthread_mutex_lock(&task_list_lock);
    task->running = true;
    task->state = NET_TASK_IDLE;
    pthread_mutex_unlock(&task_list_lock);

    if(task->from_pid)
        proc_wakeup_pid(task->from_pid, RW_BLOCK_EVT); 
        
    while(1){
        pthread_mutex_lock(&task_list_lock);
        
        // Check if we should exit
        if(!task->running) {
            pthread_mutex_unlock(&task_list_lock);
            break;
        }
        
        // Check state and process
        if(task->state == NET_TASK_START) {
            task->state = NET_TASK_PROCESS;
            pthread_mutex_unlock(&task_list_lock);
            
            do_network_fcntl(task);	
            
            pthread_mutex_lock(&task_list_lock);
            task->state = NET_TASK_FINISH;
            int from_pid = task->from_pid;
            pthread_mutex_unlock(&task_list_lock);
            
            // Wake up waiting client
            if(from_pid > 0) {
                proc_wakeup_pid(from_pid, RW_BLOCK_EVT);
            }
        } else {
            // No task, block until woken up
            pthread_mutex_unlock(&task_list_lock);
            proc_block_by(getpid(), (uint32_t)task);
        }
    }

    // Clean up in and out
    PF->clear(&task->in);
    PF->clear(&task->out);

    // Don't free here, let main thread do the freeing
    return NULL;
}
