#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <time.h>

int main(int argc, char* argv[]) {
	char* tz = getenv("TZ");
	if(tz == NULL)
		tz = "UTC";

	struct tm time_info = {0};
	proto_t out;
	PF->init(&out);
	if(dev_cntl("/dev/localtime", 0, NULL, &out) == 0) {
		int res = proto_read_int(&out);
		char* tz = getenv("TZ");
		if(tz == NULL)
			tz = "UTC";
		if(res == 0) {
			proto_read_to(&out, &time_info, sizeof(time_info));
		}
		PF->clear(&out);
	}

	printf("%04d-%02d-%02d %02d:%02d:%02d %s\n",
			time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec,
			tz);
	return 0;
}
