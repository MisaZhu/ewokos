#include <sys/syscall.h>
#include <sys/kernel_tic.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int32_t kernel_tic32(uint32_t* sec, uint32_t* usec_hi, uint32_t* usec_low) {
	return syscall3(SYS_GET_KERNEL_TIC, (int32_t)sec, (int32_t)usec_hi, (int32_t)usec_low);
}

inline int32_t kernel_tic(uint32_t* sec, uint64_t* usec) {
	uint32_t hi, low;
	int32_t ret = kernel_tic32(sec, &hi, &low);
	if(usec != NULL)
		*usec = ((uint64_t)hi) << 32 | low;
	return ret;
}

uint64_t kernel_tic_ms(int zone){
	uint64_t sec,  usec;
	kernel_tic(&sec, &usec);
	return (usec/1000);
}

#ifdef __cplusplus
}
#endif
