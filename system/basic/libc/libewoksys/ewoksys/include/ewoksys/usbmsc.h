#ifndef EWOKSYS_USBMSC_H
#define EWOKSYS_USBMSC_H
#include <stdint.h>

/*
 * Shared protocol between the USB host driver (block transport over
 * FS_CMD_DEV_CNTL/fcntl on its device node) and the filesystem daemon
 * that consumes the USB mass-storage block device.
 */

/* fcntl commands served by usbhostd on its device node (e.g. /dev/usb0) */
#define USBMSC_CMD_INFO   10 /* out: present, sector_count, sector_size */
#define USBMSC_CMD_READ   11 /* in: sector, count; out: count*sector_size data */
#define USBMSC_CMD_WRITE  12 /* in: sector, count, data; out: none (status only) */
#define USBMSC_CMD_FLUSH  13 /* in: none; out: none (USB backend flush) */
#define USBMSC_CMD_AUTOMOUNT 14 /* in: 0/1; toggle automatic /mnt/udisk0 mount */

#define USBMSC_CMD_STATS 15 /* optional out: usbmsc_stats_t, cumulative */
#define USBFS_CMD_STATS 22 /* optional sdfsd out: usbfs_stats_t, cumulative */

/* Counters wrap modulo 2^32; take unsigned snapshot differences. Timing is
 * wall time in microseconds, including preemption. No reset races with
 * concurrent workers and no per-transfer UART logging. */
typedef struct {
    uint32_t commands, bytes, service_us, errors, max_depth;
    uint32_t waits, wait_us, pauses;
} usbmsc_stats_t;
typedef struct {
    uint32_t readers, writers, read_wait_us, write_wait_us;
    uint32_t read_wait_max_us, write_wait_max_us;
    uint32_t io_calls, io_bytes, io_us;
} usbfs_stats_t;

/* FS_CMD_DEV_CNTL command served by the filesystem daemon (fat32fsd) on USB
 * unplug. The generic umount command is DEV_CMD_UMOUNT in ewoksys/vdevice.h. */
#define USBFS_CMD_QUIT    21 /* device gone: best-effort flush and stop */

/* max sectors per USBMSC_CMD_READ/WRITE request */
#define USBMSC_MAX_SECTORS 8u

#endif
