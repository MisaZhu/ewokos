#include <sys/core.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

inline void schd_core_lock() {
	syscall0(SYS_SCHD_CORE_LOCK);
}

inline void schd_core_unlock() {
	syscall0(SYS_SCHD_CORE_UNLOCK);
}

#ifdef __cplusplus
}
#endif

