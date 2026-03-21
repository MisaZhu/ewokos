#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char* argv[]) {
	char* tz = getenv("TZ");
	if(tz == NULL)
		tz = "UTC";

	time_t now = time(NULL);
	struct tm* time_info = localtime(&now);

	printf("%04d-%02d-%02d %02d:%02d:%02d %s\n",
			time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
			time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
			tz);
	return 0;
}
