#ifndef MARIO_BC_H
#define MARIO_BC_H

#include "mario.h"

void vm_dump_out(vm_t* vm);
void vm_gen_mbc(vm_t* vm, const char* fname_out);
bool vm_load_mbc(vm_t* vm, const char* fname);

#endif