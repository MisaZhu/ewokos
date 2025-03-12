#ifndef DEV_FBD_H
#define DEV_FBD_H

#include <stdint.h>
#include <stddef.h>
#include <ewoksys/fbinfo.h>
#include <graph/graph_ex.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t  (*flush)(const fbinfo_t* fbinfo, const graph_t* g);
    int32_t   (*init)(uint32_t w, uint32_t h, uint32_t dep);
    fbinfo_t* (*get_info)(void);
    void      (*splash)(graph_t* g, const char* logo);
} fbd_t;

extern int fbd_run(fbd_t* fbd, const char* mnt_name,
    uint32_t def_w,
    uint32_t def_h,
    int32_t def_rotate);

#ifdef __cplusplus
}
#endif

#endif
