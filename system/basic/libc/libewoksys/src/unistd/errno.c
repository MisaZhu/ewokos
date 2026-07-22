#include <errno.h>
#include <sysinfo.h>
#include <ewoksys/thread.h>

static int errno_fallback = ENONE;
static int errno_slots[MAX_PROC_NUM];

int *__errno(void) {
	int tid = thread_get_id();
	if(tid < 0 || tid >= MAX_PROC_NUM)
		return &errno_fallback;
	return &errno_slots[tid];
}
