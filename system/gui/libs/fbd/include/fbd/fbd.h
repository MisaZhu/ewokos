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
    /* optional: driver-side rotation. When set (and not zoomed), libfbd
     * hands over the un-rotated source graph so the driver can rotate
     * straight into the scan-out buffer, skipping the intermediate
     * rotate buffer and the extra full-frame copy. */
    uint32_t  (*flush_rotate)(const fbinfo_t* fbinfo, const graph_t* g, int rotate);
    int32_t   (*init)(uint32_t w, uint32_t h, uint32_t dep);
    int32_t   (*read)(uint8_t *buf, uint32_t size);
    fbinfo_t* (*get_info)(void);
    void      (*splash)(graph_t* g, const char* logo);
} fbd_t;

extern int fbd_run(fbd_t* fbd, const char* mnt_name,
    uint32_t def_w,
    uint32_t def_h,
    const char* conf_file);

/* Generic flush_rotate implementation: rotates the un-rotated client
 * frame (32bpp) straight into fbinfo->pointer, walking the destination
 * row-major so Normal-NC write-combine buffers merge the stores into
 * DRAM bursts. Only suitable for drivers whose flush is a plain memory
 * blit into fbinfo->pointer (NOT for SPI/command push panels). Returns
 * bytes written, or 0 (caller falls back to the generic path). */
extern uint32_t fbd_rotate_to(const fbinfo_t* fbinfo, const graph_t* g, int rotate);

#ifdef __cplusplus
}
#endif

#endif
