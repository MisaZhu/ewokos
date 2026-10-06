#ifndef EWOKSYS_USBHCI_H
#define EWOKSYS_USBHCI_H

/*
 * Shared protocol between the USB host driver (usbhostd, which claims a
 * Bluetooth HCI USB interface such as the Intel AX211/AX411/BE202
 * bluetooth function) and the btd daemon that consumes the adapter as an
 * H4 byte stream over FS_CMD_DEV_CNTL/dev_cntl on the host driver's
 * device node (e.g. /dev/hid0).
 *
 * Command numbers live in their own range, disjoint from USBMSC_CMD_*
 * (10-13) and USBFS_CMD_QUIT (21) in ewoksys/usbmsc.h.
 */

/* fcntl commands served by usbhostd on its device node (e.g. /dev/hid0) */
/* out: claimed, ready, vid, pid (4 x int) */
#define USBHCI_CMD_INFO   30
/* in: pkt_type (int: 0x01 cmd, 0x02 acl), data; out: none (status only) */
#define USBHCI_CMD_SEND   31
/* in: max_len (int); out: up to max_len bytes of the H4 stream
 * (events and ACL data, each frame prefixed with its H4 type byte);
 * an empty blob means no data is pending */
#define USBHCI_CMD_RECV   32
/* in: none; drop all pending RX bytes (stale bytes after a re-init) */
#define USBHCI_CMD_FLUSH  33
/* in: none; out: diag string blob (transport state for btd's diag) */
#define USBHCI_CMD_DIAG   34
/* in: none; re-run the firmware setup when the adapter was claimed but
 * not ready yet (e.g. the firmware file appeared later); a no-op while
 * already up */
#define USBHCI_CMD_SETUP  35

#endif
