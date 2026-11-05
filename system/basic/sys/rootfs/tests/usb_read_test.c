#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ewoksys/usbmsc.h>

#define SECTOR_SIZE 512u
static uint32_t block_size, calls, delivered, expect_lba;
static int reject_bulk, fail_single;
static int32_t fail_lba = -1;
static uint32_t sd_get_block_size(void) { return block_size; }
#define klog(...) ((void)0)

/* Deterministic disk contents, including each byte's absolute LBA. */
static uint8_t disk_byte(uint32_t lba, uint32_t offset) {
    return (uint8_t)((lba * 31u) ^ (offset * 13u) ^ (lba >> 8));
}
static int usb_msc_io(int write, int32_t lba, void* buf, uint32_t n) {
    assert(!write && lba >= 0 && n > 0 && n <= USBMSC_MAX_SECTORS);
    assert((uint32_t)lba == expect_lba);
    calls++;
    if ((reject_bulk && n > 1) || fail_single || lba == fail_lba) {
        /* A failed transport is allowed to have touched the destination. */
        memset(buf, 0xcc, n * SECTOR_SIZE);
        return -1;
    }
    for (uint32_t s = 0; s < n; s++)
        for (uint32_t b = 0; b < SECTOR_SIZE; b++)
            ((uint8_t*)buf)[s * SECTOR_SIZE + b] = disk_byte(lba + s, b);
    expect_lba += n;
    delivered += n;
    return 0;
}
#include "../usb_ext_read.h"

static void reset(uint32_t size, uint32_t first) {
    block_size = size;
    expect_lba = first;
    calls = delivered = 0;
    reject_bulk = fail_single = 0;
    fail_lba = -1;
    _usb_read_max = USBMSC_MAX_SECTORS;
}
static void check(const uint8_t* p, uint32_t first, uint32_t sectors) {
    for (uint32_t s = 0; s < sectors; s++)
        for (uint32_t b = 0; b < SECTOR_SIZE; b++)
            assert(p[s * SECTOR_SIZE + b] == disk_byte(first + s, b));
}
int main(void) {
    uint8_t buf[65536 + 2];
    for (uint32_t size = 1024; size <= 4096; size *= 2) {
        uint32_t sectors = 13 * (size / SECTOR_SIZE), first = 23 * (size / SECTOR_SIZE);
        reset(size, first);
        memset(buf, 0xa5, sizeof(buf));
        assert(usb_ext_read_blocks(23, buf + 1, 13) == 0);
        check(buf + 1, first, sectors);
        assert(buf[0] == 0xa5 && buf[1 + sectors * SECTOR_SIZE] == 0xa5);
        /* A performance budget: continuous reads must not regress to 512B IPC. */
        assert(calls == (sectors + USBMSC_MAX_SECTORS - 1) / USBMSC_MAX_SECTORS);
        reset(size, first);
        assert(usb_ext_read_block(23, buf) == 0);
        check(buf, first, size / SECTOR_SIZE);
        assert(calls == 1);
        reset(size, first);
        reject_bulk = 1;
        assert(usb_ext_read_blocks(23, buf, 13) == 0);
        check(buf, first, sectors);
        assert(calls == sectors + 1 && _usb_read_max == 1);
        assert(usb_ext_read_block(36, buf) == 0);
        check(buf, first + sectors, size / SECTOR_SIZE);
    }
    reset(4096, 0);
    assert(usb_ext_read_blocks(0, NULL, 0) == 0 && calls == 0);
    assert(usb_ext_read_blocks(-1, buf, 1) == -1 && calls == 0);
    assert(usb_ext_read_blocks(INT_MAX, buf, 1) == -1 && calls == 0);
    assert(usb_ext_read_blocks(0, buf, UINT_MAX) == -1 && calls == 0);
    assert(usb_ext_read_blocks(0, NULL, 1) == -1 && calls == 0);
    block_size = 513;
    assert(usb_ext_read_block(0, buf) == -1 && calls == 0);
    reset(4096, 0x7ffffff8u);
    assert(usb_ext_read_block(0x0fffffffu, buf) == 0 && delivered == 8);
    reset(4096, 0);
    fail_single = 1;
    assert(usb_ext_read_blocks(0, buf, 2) == -1 && calls == 2 && delivered == 0);
    reset(4096, 0);
    fail_lba = 8;
    assert(usb_ext_read_blocks(0, buf, 4) == -1 && calls == 3 && delivered == 8);
    puts("USB ext reads: PASS (1K/2K/4K, batching, fallback, errors, bounds)");
    return 0;
}
