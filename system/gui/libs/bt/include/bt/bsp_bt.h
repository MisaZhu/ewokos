#ifndef BSP_BT_H
#define BSP_BT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * bsp_bt: per-machine bluetooth HCI transport contract, the bluetooth twin
 * of bsp_usb. The shared btd daemon (system/basic/drivers/btd) owns the
 * HCI/L2CAP/HIDP/HOGP stack and the /dev/bt0 device surface; each machine's
 * libbsp implements the five hooks below so the daemon never touches a
 * UART, a pin or a mailbox itself.
 *
 * Every machine provides an implementation: machines with a combo chip
 * (raspi5, raspix) a real one, all others a no-op stub, exactly the
 * bsp_usb pattern. The daemon stays in its bounded retry loop when
 * bsp_bt_init() keeps failing, so shipping the binary on a machine without
 * bluetooth is harmless (and init.rd simply doesn't launch it there).
 *
 * Typical call sequence in btd:
 *   bsp_bt_init(false)            (retried until it succeeds)
 *   HCI_Reset / firmware download (bsp_bt_firmware + bsp_bt_send/_recv)
 *   steady state: bsp_bt_send / bsp_bt_recv(0) polling
 *   "close" command: bsp_bt_power_off()
 *   "open" command after close: bsp_bt_init(true)
 */

/* Full transport bring-up: map MMIO, power the BT core (combo-chip
   coordination such as a co-resident wifi device is the machine's own
   business), program pinmux and UART at the HCI default baud, drain stale
   RX bytes. Leaves the controller ready to answer HCI_Reset.
   recovery=true means the radio is being re-opened after a power_off or a
   failed attempt, so the machine should use the long power pulse.
   Returns 0 on success, <0 when there is no usable bluetooth transport;
   btd retries a failed bring-up on a timer, so a stub simply returns -1. */
int bsp_bt_init(bool recovery);

/* Power the BT core off ("close" command). WL coexistence lines stay
   untouched. No-op on machines without bluetooth. */
void bsp_bt_power_off(void);

/* Send one complete H4 packet: the 1-byte packet type (0x01 cmd / 0x02 ACL)
   followed by len payload bytes. Returns 0 on success, <0 on timeout or
   when the transport is not up. */
int bsp_bt_send(uint8_t pkt_type, const uint8_t* data, size_t len);

/* Receive one byte of the H4 stream, waiting up to timeout_ms (0 = poll
   once, no wait). Returns the byte value 0-255, or <0 on timeout / when the
   transport is not up. Must be safe to call before bsp_bt_init() ever
   succeeded (return <0 then). */
int bsp_bt_recv(uint32_t timeout_ms);

/* Drain and discard pending RX bytes (used right after UART setup).
   Bounded internally; returns how many bytes were dropped. */
int bsp_bt_flush(void);

/* Patchram download image for the controller, or *data=NULL / *len=0 when
   the chip needs none (stub). The image is machine-specific data and lives
   in the machine's own bsp (raspi5/raspix carry their CYW43455
   bcm4345c0_hcd blob as firmware_4345c0.h next to their bsp_bt.c). */
void bsp_bt_firmware(const uint8_t** data, uint32_t* len);

/* One short "k=v k=v" snapshot of the transport registers, spliced into
   btd's timeout/error slog lines (the raspi5 lsr/msr dump). Writes "" or a
   truncated string when nothing is mapped yet. buf is always NUL-terminated
   by the implementation. */
void bsp_bt_diag_str(char* buf, size_t size);

#endif
