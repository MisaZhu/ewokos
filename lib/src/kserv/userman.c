#include <kserv/userman.h>
#include <pmessage.h>
#include <string.h>
#include <syscall.h>

static int _usermanPid = -1;

#define CHECK_KSERV_USERMAN \
	if(_usermanPid < 0) \
		_usermanPid = syscall1(SYSCALL_KSERV_GET, (int)KSERV_USERMAN_NAME); \
	if(_usermanPid < 0) \
		return -1; 


int usermanAuth(const char* name, const char* passwd) {
	CHECK_KSERV_USERMAN
	int uid = -1;
	char loginData[LOGIN_DATA_LEN*2];
	strncpy(loginData, name, LOGIN_DATA_LEN);
	strncpy(loginData+LOGIN_DATA_LEN, passwd, LOGIN_DATA_LEN);

	PackageT* pkg = preq(_usermanPid, USERMAN_AUTH, (void*)loginData, LOGIN_DATA_LEN*2);
	if(pkg == NULL)	
		return -1;

	uid = *(int*)getPackageData(pkg);
	free(pkg);
	return uid;
}

