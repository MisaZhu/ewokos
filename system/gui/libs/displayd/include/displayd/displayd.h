#ifndef DISPLAYD_H
#define DISPLAYD_H

#include <stdint.h>
#include <stddef.h>
#include <ewoksys/fbinfo.h>
#include <graph/graph_ex.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t  (*flush)(const fbinfo_t* fbinfo, const graph_t* g);
    /* optional: driver-side rotation. When set (and not zoomed), libfbdisplayd
     * hands over the un-rotated source graph so the driver can rotate
     * straight into the scan-out buffer, skipping the intermediate
     * rotate buffer and the extra full-frame copy. */
    uint32_t  (*flush_rotate)(const fbinfo_t* fbinfo, const graph_t* g, int rotate);
    int32_t   (*init)(uint32_t w, uint32_t h, uint32_t dep);
    int32_t   (*read)(uint8_t *buf, uint32_t size);
    fbinfo_t* (*get_info)(void);
    void      (*splash)(graph_t* g, const char* logo);
} fbdisplayd_t;

extern int fbdisplayd_run(fbdisplayd_t* fbdisplayd, const char* mnt_name,
    uint32_t def_w,
    uint32_t def_h,
    const char* conf_file,
    uint32_t display_index);

/* Generic flush_rotate implementation: rotates the un-rotated client
 * frame (32bpp) straight into fbinfo->pointer, walking the destination
 * row-major so Normal-NC write-combine buffers merge the stores into
 * DRAM bursts. Only suitable for drivers whose flush is a plain memory
 * blit into fbinfo->pointer (NOT for SPI/command push panels). Returns
 * bytes written, or 0 (caller falls back to the generic path). */
extern uint32_t fbdisplayd_rotate_to(const fbinfo_t* fbinfo, const graph_t* g, int rotate);

/* Opt-in partial flush. Once registered, libfbdisplayd pushes only the rects the
 * client declared through display_set_dirty() instead of the whole frame. It is
 * a separate registration (not a fbdisplayd_t field) because several drivers leave
 * the tail of their fbdisplayd_t uninitialised. Only used when no rotation and no
 * zoom is active; returning 0 makes libfbdisplayd fall back to a full flush. */
extern void fbdisplayd_set_flush_rect(uint32_t (*flush_rect)(const fbinfo_t* fbinfo,
		const graph_t* g, const grect_t* r));

/* Generic flush_rect implementation for drivers whose flush is a plain
 * memory blit into fbinfo->pointer (32bpp and 16bpp, pitch aware). NOT for
 * SPI/command push panels. Returns bytes written, or 0 when the geometry
 * does not allow it. */
extern uint32_t fbdisplayd_flush_rect_to(const fbinfo_t* fbinfo, const graph_t* g, const grect_t* r);

/* Opt-in dev.cmd handler (see the `devcmd` tool), for panel side knobs like
 * the backlight that have no place in the fbinfo/fcntl API. Like
 * fbdisplayd_set_flush_rect() this is a separate registration (not a fbdisplayd_t field)
 * because several drivers leave the tail of their fbdisplayd_t uninitialised. The
 * returned string is malloc'ed and freed by the caller; NULL means the
 * command is not supported. */
extern void fbdisplayd_set_dev_cmd(char* (*dev_cmd)(int from_pid, int argc, char** argv));

/* Re-push the frame the client last drew, without waiting for it to redraw.
 * Needed by drivers whose dev.cmd changes how pixels reach the panel (a
 * contrast LUT, say), since on a static screen the next client flush may
 * never come. Returns 0 on success, -1 when there is no frame yet. */
extern int fbdisplayd_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
