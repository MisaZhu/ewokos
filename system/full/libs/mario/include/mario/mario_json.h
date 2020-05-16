#ifndef MARIO_JSON
#define MARIO_JSON

#ifdef __cplusplus
extern "C" {
#endif

#include "mario_vm.h"

extern var_t* json_parse(vm_t* vm, const char* str);

#ifdef __cplusplus
}
#endif
#endif
