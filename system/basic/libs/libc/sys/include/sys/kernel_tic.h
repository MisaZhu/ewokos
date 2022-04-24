#ifndef KERNEL_TIC_H 
#define KERNEL_TIC_H 

#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

int32_t kernel_tic(uint32_t* sec, uint64_t* usec);

#ifdef __cplusplus
} 
#endif

#endif
