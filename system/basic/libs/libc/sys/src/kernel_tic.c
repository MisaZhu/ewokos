#include <sys/syscall.h>
#include <sys/kernel_tic.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int32_t kernel_tic(uint32_t* sec, uint64_t* usec) {
	uint32_t hi, low;
	int32_t ret = syscall3(SYS_GET_KERNEL_TIC, (int32_t)sec, (int32_t)&hi, (int32_t)&low);
	if(usec != NULL)
		*usec = ((uint64_t)hi) << 32 | low;
	return ret;
}

#ifdef __cplusplus
}
#endif
