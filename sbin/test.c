#include <pmessage.h>
#include <fork.h>
#include <malloc.h>
#include <stdio.h>

void _start()
{
	char *p = "hello";
	int pid = getpid();	
	while(1) {
		psendMessage(pid, p, 5);
		ProcMessageT* msg = preadMessage(-1);
		printf("[%s], size=%d, pid=%d\n", msg->data, msg->size, msg->fromPid);
		free(msg);
	}
	
	exit(0);
}
