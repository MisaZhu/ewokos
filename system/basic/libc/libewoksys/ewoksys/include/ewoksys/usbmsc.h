#ifndef EWOKSYS_USBMSC_H
#define EWOKSYS_USBMSC_H

/*
 * Shared protocol between the USB host driver (block transport over
 * FS_CMD_DEV_CNTL/fcntl on its device node) and the filesystem daemon
 * that consumes the USB mass-storage block device.
 */

/* fcntl commands served by usbhostd on its device node (e.g. /dev/hid0) */
#define USBMSC_CMD_INFO   10 /* out: present, sector_count, sector_size */
#define USBMSC_CMD_READ   11 /* in: sector, count; out: count*sector_size data */
#define USBMSC_CMD_WRITE  12 /* in: sector, count, data; out: none (status only) */
#define USBMSC_CMD_FLUSH  13 /* in: none; out: none (SCSI SYNC cache best-effort) */

/* FS_CMD_DEV_CNTL commands served by the filesystem daemon (fat32fsd) */
#define USBFS_CMD_UMOUNT  20 /* flush, detach the mount, stop; -1 if busy */
#define USBFS_CMD_QUIT    21 /* device gone: best-effort flush and stop */

/* max sectors per USBMSC_CMD_READ/WRITE request */
#define USBMSC_MAX_SECTORS 8u

#endif
