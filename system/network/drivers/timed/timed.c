#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/kernel_tic.h>
#include <ntpc/ntpc.h>

static uint32_t _time_sec_init = 0;
static time_t _time_init = 0;
static uint32_t _next_sync_sec = 0;

#define TIME_SYNC_INTERVAL_SEC 600
#define TIME_SYNC_RETRY_MIN_SEC 5 
#define TIME_SYNC_RETRY_MAX_SEC 600

static uint32_t _sync_retry_sec = TIME_SYNC_RETRY_MIN_SEC;

static int time_dcntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)p;
	if(cmd != 0 || _time_init == 0) {
		PF->addi(ret, -1);
		return -1;
	}

	uint32_t current_time_sec;
	kernel_tic(&current_time_sec, NULL);
	time_t time = _time_init + current_time_sec - _time_sec_init;
	PF->addi(ret, 0)->addi(ret, time);
	return 0;
}

static int time_loop(vdevice_t* dev, void* p) {
	(void)dev;
	uint32_t current_time_sec;
	kernel_tic(&current_time_sec, NULL);

	if(_next_sync_sec != 0 && current_time_sec < _next_sync_sec) {
		usleep(300000);
		return 0;
	}

	if(_time_init == 0 || (current_time_sec - _time_sec_init) > TIME_SYNC_INTERVAL_SEC) {
    	time_t t = ntpc_get_time(DEFAULT_NTP_SERVER, DEFAULT_NTP_PORT);
		if(t > 0) {
			_time_init = t;
			kernel_tic(&_time_sec_init, NULL);
			_next_sync_sec = _time_sec_init + TIME_SYNC_INTERVAL_SEC;
			_sync_retry_sec = TIME_SYNC_RETRY_MIN_SEC;
		}
		else {
			// Back off when NTP is unavailable so netd does not keep timing out UDP requests.
			_next_sync_sec = current_time_sec + _sync_retry_sec;
			if(_sync_retry_sec < TIME_SYNC_RETRY_MAX_SEC) {
				if(_sync_retry_sec > TIME_SYNC_RETRY_MAX_SEC) {
					_sync_retry_sec = TIME_SYNC_RETRY_MAX_SEC;
				}
			}
		}
	}
	usleep(300000);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/time";
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "time");
	dev.dev_cntl = time_dcntl;
	dev.loop_step = time_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
