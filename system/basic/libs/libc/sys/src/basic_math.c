#include <sys/basic_math.h>
#include <sys/kernel_tic.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t _r_mask = 0x13579abc;
inline uint32_t random_u32(void) {
	uint64_t usec;
	kernel_tic(NULL, &usec);
	uint32_t ret = (uint32_t)(usec&0xffffffff) * _r_mask;
	_r_mask *= 0x79efd1;
	_r_mask += 0xe1;
	return ret;
}

inline uint32_t random_to(uint32_t to) {
	uint32_t r = random_u32();	
	return r % to;
}

#ifdef __cplusplus
}
#endif
