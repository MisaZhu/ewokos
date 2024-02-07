#ifndef CMAIN_H
#define CMAIN_H

#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  main(int argc, char* argv[]);
const char* cmain_get_work_dir(void);

#ifdef __cplusplus
}
#endif


#endif
