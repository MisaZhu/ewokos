#include "native_builtin.h"
#include "x/native_x.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_x_natives(vm_t* vm) {
	reg_native_x(vm);
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
