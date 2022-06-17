#ifndef DEV_FBD_H
#define DEV_FBD_H

#include <stdint.h>
#include <stddef.h>
#include <sys/fbinfo.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t  (*flush)(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate);
    int32_t   (*init)(uint32_t w, uint32_t h, uint32_t dep);
    fbinfo_t* (*get_info)(void);
} fbd_t;

extern int fbd_run(fbd_t* fbd, int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif
