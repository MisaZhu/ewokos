#include <ewoksys/sys.h>
#include <ewoksys/kernel_tic.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int32_t kernel_tic32(uint32_t* sec, uint32_t* usec_hi, uint32_t* usec_low) {
	if(_vsyscall_info == NULL)
		return -1;

	if(sec != NULL)
		*sec = _vsyscall_info->kernel_usec / 1000000;
	if(usec_hi != NULL)
		*usec_hi = _vsyscall_info->kernel_usec >> 32;
	if(usec_low != NULL)
		*usec_low = _vsyscall_info->kernel_usec & 0xffffffff;
	return 0;
}

inline int32_t kernel_tic(uint32_t* sec, uint64_t* usec) {
	if(_vsyscall_info == NULL)
		return -1;

	if(usec != NULL)
		*usec = _vsyscall_info->kernel_usec;
	if(sec != NULL)
		*sec = _vsyscall_info->kernel_usec / 1000000;
	return 0;
}

uint64_t kernel_tic_ms(int zone){
	return _vsyscall_info->kernel_usec / 1000;
}

#ifdef __cplusplus
}
#endif
