#include "native_builtin.h"
#include "object/native_object.h"
#include "number/native_number.h"
#include "array/native_array.h"
#include "string/native_string.h"
//#include "math/native_math.h"
#include "bytes/native_bytes.h"
#include "console/native_console.h"
#include "json/native_json.h"
#include "system/native_system.h"
#include "fs/native_fs.h"
#include "dir/native_dir.h"
//#include "socket/native_socket.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_basic_natives(vm_t* vm) {
	reg_native_object(vm);
	reg_native_string(vm);
	reg_native_console(vm);
	reg_native_number(vm);
	reg_native_array(vm);
	//reg_native_math(vm);
	reg_native_json(vm);
	reg_native_bytes(vm);
	reg_native_system(vm);
	reg_native_fs(vm);
	reg_native_dir(vm);
	//reg_native_socket(vm);
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
