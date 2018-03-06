#include <kserv/fs.h>
#include <pmessage.h>
#include <malloc.h>
#include <string.h>

int _fsPid = 1;

int fsOpen(const char* name) {
	int fd = -1;

	PackageT* pkg = preq(_fsPid, FS_OPEN, (void*)name, strlen(name)+1);
	if(pkg == NULL)	
		return -1;

	fd = *(int*)getPackageData(pkg);
	free(pkg);
	return fd;
}

void fsInit(int ksPid) {
	_fsPid = ksPid;
}

int fsInited() {
	return _fsPid;
}
