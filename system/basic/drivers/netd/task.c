

#include "task.h"
#include "netd.h"
#include <ewoksys/vfs.h>

static void* task_thread(void* arg);

net_task_t *create_task(int fd, int from_pid, int node){
    net_task_t *task = malloc(sizeof(net_task_t));
    memset(task, 0 , sizeof(net_task_t));
    task->fd = fd;
    task->from_pid = from_pid;
    task->node = node;
    task->running = true;
    task->sock = -1;
    task->iobuf = malloc(4096);
    task->inbuf = malloc(4096);
    task->outbuf = malloc(4096);
    pthread_create(&task->tid, NULL, task_thread, task);
    return task;
}

void release_task(net_task_t *task){
    task->running = false;
    proc_wakeup(task);
}

int  task_cntl(net_task_t* task, int from_pid, int cmd, proto_t *in,  proto_t *out, void *p){
    if(task->state == NET_TASK_FINISH){
        if(cmd != task->cmd || from_pid != task->from_pid){
            klog("ipc error\n");
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
        PF->init_data(&task->in, task->inbuf, 4096);
        task->in.size = 0;
        PF->init_data(&task->out, task->outbuf, 4096);
        task->out.size = 0;
        PF->copy(&task->in, in->data, in->size);
        task->state = NET_TASK_START;
        proc_wakeup(task);
    }
    return VFS_ERR_RETRY;
}

int  task_read(net_task_t* task, int from_pid, char* buf,  int size, void *p){
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_RECV != task->cmd || from_pid != task->from_pid){
            klog("ipc error\n");
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
        PF->init_data(&task->in, task->inbuf, 4096);
        task->in.size = 0;
        PF->init_data(&task->out, task->outbuf, 4096);
        task->out.size = 0;
        PF->addi(&task->in, size);
        task->state = NET_TASK_START;
        proc_wakeup(task);
    }
    return VFS_ERR_RETRY;
}

int  task_write(net_task_t* task, int from_pid,  char* buf,  int size, void *p){
    
    if(task->state == NET_TASK_FINISH){
        if(SOCK_SEND != task->cmd || from_pid != task->from_pid){
            klog("ipc error\n");
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
        PF->init_data(&task->in, task->inbuf, 4096);
        task->in.size = 0;
        PF->init_data(&task->out, task->outbuf, 4096);
        task->out.size = 0;
        PF->add(&task->in, buf, size);
        task->state = NET_TASK_START;
        proc_wakeup(task);
    }
    return VFS_ERR_RETRY;
}


int do_network_fcntl(net_task_t *task){
	int domain, sock,type,protocol;
	char *data;
	int size, addrlen = sizeof(struct sockaddr);
	struct sockaddr *paddr;
	struct sockaddr addr;
    char* buf;
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
            ret = sock_recvfrom(sock, task->iobuf, size, &addr, &addrlen);
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->iobuf, size);
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
            ret = sock_recv(sock, task->iobuf, size);
            PF->addi(&task->out, ret);
            if(ret > 0){
                PF->add(&task->out, task->iobuf, ret);
            }
            free(buf);
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
		default:
			break;
	}
    return 0;
}


static void* task_thread(void* arg){
    net_task_t *task = (net_task_t *)arg;
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
        proc_block_by(getpid(), task);
    }
    PF->clear(&task->in);
    PF->clear(&task->out);
    free(task->inbuf);
    free(task->outbuf);
    free(task->iobuf);
    sock_close(task->sock);
    klog("close task %08x\n", task,  task->sock);
    free(task);
}
