/*kserv is the first process after kernel booted,
for manageing kernel service processes.
*/

#include <stdio.h>
#include <fork.h>
#include <types.h>
#include <pmessage.h>
#include <kserv/kserv.h>
#include <string.h>

#define KSERV_MAX 32
#define KSERV_NAME_MAX 16

typedef struct {
	char name[KSERV_NAME_MAX+1];
	int pid;
} KServT;

static KServT _kernelServices[KSERV_MAX];

static void regKServ(const char* name, int pid) {
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].name[0] == 0) 
			break;

		if(strcmp(_kernelServices[i].name, name) == 0) {
			_kernelServices[i].pid = pid;
			printf("kserv[%d] has been registered already!\n", i);
			return;
		}
	}
	
	if(i >= KSERV_MAX) {
		printf("no more kserv can be registered!\n");
		return;
	}

	strncpy(_kernelServices[i].name, name, KSERV_NAME_MAX);
	_kernelServices[i].pid = pid;
}

static int getKServ(const char* name) {
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].name[0] == 0) 
			break;

		if(strcmp(_kernelServices[i].name, name) == 0) {
			return _kernelServices[i].pid;
		}
	}
	return -1;
}

static void handle(PackageT* pkg) {
	if(pkg == NULL || pkg->size == 0)
		return;
	char* name = (char*)getPackageData(pkg);

	if(pkg->type == KS_REG) { /*register a kernel process*/
		regKServ(name, pkg->pid);	
	}
	else if(pkg->type == KS_GET) { /*get the pid of specific kernel process*/
		int pid = getKServ(name);
		psend(pkg->id, pkg->pid, pkg->type, &pid, 4);
	}
}

static void init() {
	for(int i=0; i<KSERV_MAX; ++i) {
		_kernelServices[i].name[0] = 0; 
		_kernelServices[i].pid = -1; 
	}
}

void _start() 
{
	init();

	printf("start file system ...\n");
	int pid = fork();
	if(pid == 0) { //file system process
		exec("vfs");
	}

	printf("start shell...\n");
	pid = fork();
	if(pid == 0) {
		while(1) {
			pid = fork();
			if(pid == 0) { //shell process
				exec("shell");
			}
			wait(pid);
		}
	}

	while(1) {
		PackageT* pkg = precv(-1);
		if(pkg != NULL)
			handle(pkg);
		else
			yield();
	}
	exit(0);
}
