#ifndef BSP_BOOTFS_H
#define BSP_BOOTFS_H

#include <stdint.h>

/*
 * Boot-initramfs block backend behind sdfsd's "-b" mode.
 *
 * The boot image is a complete ext2/ext3 filesystem image handed to user
 * space by the firmware.  bsp_bootfs_map() maps it into this process's
 * address space and fails (-1) on platforms without one; the block
 * callbacks then serve it from memory.  The volume is read-only: the
 * write callbacks are always rejected.  sdfsd keeps the rest of the boot
 * path (no device probe, read-only mount, no flush) generic on top of
 * this contract.
 */

int bsp_bootfs_map(void);
void bsp_bootfs_unmap(void);

int32_t bsp_bootfs_read_block(int32_t block, void* buf);
int32_t bsp_bootfs_read_blocks(int32_t block, void* buf, uint32_t count);
int32_t bsp_bootfs_write_block(int32_t block, const void* buf);
int32_t bsp_bootfs_write_blocks(int32_t block, const void* buf, uint32_t count);

#endif
