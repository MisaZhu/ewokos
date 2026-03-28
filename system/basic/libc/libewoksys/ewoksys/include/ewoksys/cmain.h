#ifndef CMAIN_H
#define CMAIN_H

#include <sys/errno.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  main(int argc, char* argv[]);
const char* cmain_get_own_dir(char* ret, uint32_t len);

#ifdef __cplusplus
}
#endif


#endif
