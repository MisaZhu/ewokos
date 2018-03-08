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

int fsClose(int fd) {
	if(fd < 0)
		return -1;

	if(psend(-1, _fsPid, FS_CLOSE, (void*)&fd, 4) < 0)
		return -1;
	return 0;
}

int fsRead(int fd, char* buf, uint32_t size) {
	if(fd < 0)
		return -1;
	
	char req[8];
	memcpy(req, &fd, 4);
	memcpy(req+4, &size, 4);

	PackageT* pkg = preq(_fsPid, FS_READ, req, 8);
	if(pkg == NULL)
		return -1;

	int sz = pkg->size;
	if(sz == 0) {
		free(pkg);
		return 0;
	}
	
	memcpy(buf, getPackageData(pkg), sz);
	free(pkg);
	return sz;
}

void fsInit(int ksPid) {
	_fsPid = ksPid;
}

int fsInited() {
	return _fsPid;
}
