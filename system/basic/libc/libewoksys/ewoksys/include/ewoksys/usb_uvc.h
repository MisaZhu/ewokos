#ifndef EWOKSYS_USB_UVC_H
#define EWOKSYS_USB_UVC_H

#include <stdint.h>

/*
 * Shared protocol between the USB host driver (usbhostd, which claims a
 * UVC webcam's video-streaming interface through the machine bsp layer)
 * and a preview/capture app. Control goes over FS_CMD_DEV_CNTL/dev_cntl
 * on the host driver's device node (e.g. /dev/usb0); frames are handed
 * over through a keyed shm segment whose id USB_UVC_CMD_INFO returns.
 *
 * Command numbers live in their own range, disjoint from USBMSC_CMD_*
 * (10-15), USBFS_CMD_* (21-22) and USBHCI_CMD_* (30-35).
 */

/* out: claimed, ready, streaming, format, w, h, shm_id (7 x int).
 * format: UVC_FRAME_FMT_*; shm_id <= 0 when no segment exists yet. */
#define USB_UVC_CMD_INFO   40
/* in: none; probe/commit the selected format and start isochronous
 * capture. out: none (status only). Fails when no camera is claimed. */
#define USB_UVC_CMD_START  41
/* in: none; stop capture and release the isochronous endpoint */
#define USB_UVC_CMD_STOP   42
/* out: frames, dropped, bytes, errors (4 x int), cumulative */
#define USB_UVC_CMD_STATS  43

#define UVC_FRAME_FMT_YUY2  0
#define UVC_FRAME_FMT_MJPEG 1

/*
 * Layout of the shm segment (all fields little-endian native):
 *
 *   usb_uvc_shm_header_t        (64 bytes)
 *   frame buffer 0              (cap bytes)
 *   frame buffer 1              (cap bytes)
 *
 * The driver fills the buffer NOT selected by `index`, then publishes it
 * by storing the new sequence and flipping `index`. A reader snapshots
 * `index`/`seq`, copies the selected buffer, and re-checks `index`: an
 * unchanged index means the copy is a complete frame. `cap` is fixed for
 * the lifetime of the segment; `size` is the valid byte count of the
 * latest frame (raw YUY2 pixels or one complete JPEG image).
 */
typedef struct {
	uint32_t magic;      /* USB_UVC_SHM_MAGIC */
	uint32_t version;    /* 1 */
	uint32_t w, h;       /* frame dimensions */
	uint32_t format;     /* UVC_FRAME_FMT_* */
	uint32_t cap;        /* per-buffer capacity in bytes */
	volatile uint32_t index; /* buffer holding the latest frame (0/1) */
	volatile uint32_t seq;   /* frame sequence, 0 = no frame yet */
	uint32_t size;       /* valid bytes of the latest frame */
	uint32_t reserved[7];
} usb_uvc_shm_header_t;

#define USB_UVC_SHM_MAGIC 0x31435655u /* "UVC1" */

#endif