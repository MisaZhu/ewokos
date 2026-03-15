
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>

#include "task.h"
#include "netd.h"

static void* task_thread(void* arg);

net_task_t *task_list = NULL;

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
    ipc_disable();
    net_task_t *task = task_list;
    while(task!=NULL){
       pthread_create(&task->tid, NULL, task_thread, task);
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
    task->read_buf = NULL;
    task_list_add(task);
    //pthread_create(&task->tid, NULL, task_thread, task);
    return task;
}

void release_task(net_task_t *task){
    sock_close(task->sock);
    task->running = false;
    proc_wakeup((uint32_t)task);
}

int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p){
    if(task->state == NET_TASK_FINISH){
        if(cmd != task->cmd || from_pid != task->from_pid){
            //klog("cntl ipc error task_cmd:%d(should:%d) frompid:%d task->frompid:%d\n",
            //        task->cmd, cmd, from_pid, task->from_pid);
            return VFS_ERR_RETRY;
        }
        PF->copy(out, task->out.data, task->out.size);
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE; 
        return 0;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = cmd;	
        task->p = p;
        task->from_pid = from_pid;
        PF->init(&task->in);
        PF->init(&task->out);
        PF->copy(&task->in, in->data, in->size);
        task->state = NET_TASK_START;
        proc_wakeup((uint32_t)task);
    }
    return VFS_ERR_RETRY;
}

int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p){
    if(!task->is_read_task){
        return VFS_ERR_RETRY;
    }

    if(task->state == NET_TASK_FINISH){
        if(from_pid != task->from_pid || SOCK_RECV != task->cmd) {
            //klog("read ipc error task_cmd:%d(should:%d) frompid:%d task->frompid:%d\n",
            //        task->cmd, SOCK_RECV, from_pid, task->from_pid);
            return VFS_ERR_RETRY;
        }
        int len = proto_read_int(&task->out);
        if(len > 0){
            proto_read_to(&task->out, buf, len);
        }
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE; 
        return len;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_RECV;	
        task->p = p;
        task->from_pid = from_pid;
        PF->init(&task->in);
        PF->init(&task->out);
        PF->addi(&task->in, size);
        task->state = NET_TASK_START;
        proc_wakeup((uint32_t)task);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
    if(task->state == NET_TASK_FINISH){
        if(SOCK_SEND != task->cmd || from_pid != task->from_pid){
            //klog("write ipc error task_cmd:%d(should:%d) frompid:%d task->frompid:%d\n",
            //        task->cmd, SOCK_SEND, from_pid, task->from_pid);
            return VFS_ERR_RETRY;
        }
        int ret = proto_read_int(&task->out);
        PF->clear(&task->out);
        PF->clear(&task->in);
        task->state = NET_TASK_IDLE; 
        return ret;
    }else if(task->state == NET_TASK_IDLE){
        task->cmd = SOCK_SEND;	
        task->p = p;
        task->from_pid = from_pid;

        PF->init(&task->in);
        PF->init(&task->out);
        PF->add(&task->in, buf, size);
        task->state = NET_TASK_START;
        proc_wakeup((uint32_t)task);
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
			ret = sock_bind(sock, paddr, addrlen);
			PF->addi(&task->out, ret);
			break;
		case SOCK_SENDTO:
			data = proto_read(&task->in, &size);
			paddr = proto_read(&task->in, &addrlen);
			ret = sock_sendto(sock, data, size, paddr, addrlen);
			PF->addi(&task->out, ret);
			break;
		case SOCK_RECVFROM:
			size = proto_read_int(&task->in);
            if(task->read_buf == NULL){
                task->read_buf = malloc(4096);
            }
            ret = sock_recvfrom(sock, task->read_buf, size, &addr, &addrlen);
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->read_buf, size);
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
            if(task->read_buf == NULL){
                task->read_buf = malloc(4096);
            }
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
		ret = sock_connect(sock, paddr, addrlen);
		PF->addi(&task->out, ret);
		break;
	case SOCK_SETOPT:
		level = proto_read_int(&task->in);
		optname = proto_read_int(&task->in);
		optval = proto_read(&task->in, &optlen);
		ret = sock_setsockopt(sock, level, optname, optval, optlen);
		PF->addi(&task->out, ret);
		break;
	default:
		break;
	}
    return 0;
}

static void* task_thread(void* arg){
    net_task_t *task = (net_task_t *)arg;
    task->running = true;
    task->state = NET_TASK_IDLE;

    if(task->from_pid)
        proc_wakeup_pid(task->from_pid, RW_BLOCK_EVT); 
        
    while(task->running){
		ipc_disable();
        if(task->state == NET_TASK_START)
            task->state = NET_TASK_PROCESS;
		ipc_enable();
		
        if(task->state == NET_TASK_PROCESS){
            do_network_fcntl(task);	
            task->state = NET_TASK_FINISH;
            proc_wakeup_pid(task->from_pid, RW_BLOCK_EVT);
        }

        if(task->running) {
            proc_block_by(getpid(), (uint32_t)task);
        }
    }

    PF->clear(&task->in);
    PF->clear(&task->out);
    if(task->read_buf != NULL)
        free(task->read_buf);

    if(!task->is_read_task) {
        sock_close(task->sock);
        if(task->read_task != NULL) {
            task->read_task->running = false;
            proc_wakeup((uint32_t)task->read_task);
        }
    }
    free(task);
    return NULL;
}
