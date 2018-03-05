#include <kserv/kserv.h>
#include <pmessage.h>
#include <string.h>

int getKServPid(const char* name) {
	int ret;

	PackageT* pkg = preq(0, KS_GET, (void*)name, strlen(name)+1);
	ret = *(int*)getPackageData(pkg);
	freePackage(pkg);

	return ret;
}
