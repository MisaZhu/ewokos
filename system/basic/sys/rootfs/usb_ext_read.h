/* USB ext filesystem reads. Included after usb_msc_io() so transport errors
 * (including a short IPC reply) are handled before attempting compatibility
 * reads. The filesystem may have several concurrent read workers. */
static uint32_t _usb_read_max = USBMSC_MAX_SECTORS;

static int32_t usb_ext_read_blocks(int32_t block, void* buf, uint32_t count) {
    uint32_t bytes = sd_get_block_size();
    if (block < 0 || bytes == 0 || bytes % SECTOR_SIZE != 0)
        return -1;
    if (count == 0)
        return 0;
    if (buf == NULL)
        return -1;

    uint64_t sectors = bytes / SECTOR_SIZE;
    uint64_t first = (uint64_t)(uint32_t)block * sectors;
    uint64_t left = (uint64_t)count * sectors;
    /* usb_msc_io uses signed relative LBAs. Check the whole range before
     * issuing anything; do not let block/count multiplication wrap. */
    if (first + left > 0x80000000ULL)
        return -1;

    uint8_t* p = (uint8_t*)buf;
    while (left != 0) {
        uint32_t max = __atomic_load_n(&_usb_read_max, __ATOMIC_RELAXED);
        uint32_t n = left > max ? max : (uint32_t)left;
        if (usb_msc_io(0, (int32_t)first, p, n) != 0) {
            if (n == 1)
                return -1;
            /* usbhostd has already recovered BOT. Retry the same range as
             * single sectors for bridges that cannot complete larger READ10
             * data phases. Keep subsequent requests in compatibility mode. */
            __atomic_store_n(&_usb_read_max, 1, __ATOMIC_RELAXED);
            klog("sdfsd: USB bulk read failed, using single-sector reads\n");
            continue;
        }
        first += n;
        left -= n;
        p += n * SECTOR_SIZE;
    }
    return 0;
}

static int32_t usb_ext_read_block(int32_t block, void* buf) {
    return usb_ext_read_blocks(block, buf, 1);
}
