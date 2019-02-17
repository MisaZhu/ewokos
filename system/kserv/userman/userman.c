#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv/kserv.h>
#include <kserv/userman.h>
#include <stdio.h>

static int login(const char* user, const char* passwd) {
	//TODO: login by username and password
	if(strcmp(user, passwd) != 0)
		return -1;

	if(strcmp(user, "root") == 0)
		return 0;
	return 1;
}

static void doAuth(PackageT* pkg) {
	const char* loginData = (const char*)getPackageData(pkg);
	int uid = -1;

	const char* user = loginData;
	const char* passwd = loginData + LOGIN_DATA_LEN;

	uid = login(user, passwd);
	syscall2(SYSCALL_SET_UID, pkg->pid, uid);
	psend(pkg->id, pkg->type, &uid, 4);
}

static void handle(PackageT* pkg, void* p) {
	(void)p;

	switch(pkg->type) {
	case USERMAN_AUTH:
		doAuth(pkg);
	}
}

void _start() {
	if(syscall1(SYSCALL_KSERV_GET, (int)KSERV_USERMAN_NAME) >= 0) {
    printf("Panic: 'userman' process has been running already!\n");
		exit(0);
	}

	if(!kservRun(KSERV_USERMAN_NAME, handle, NULL))
		exit(0);
}
