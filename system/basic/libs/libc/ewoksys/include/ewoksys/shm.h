#ifndef SHM_H
#define SHM_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t shm_get(int32_t id, unsigned int size, int flag);
void* shm_at(int32_t id);
int   shm_dt(void* p);

#ifdef __cplusplus
}
#endif

#endif
