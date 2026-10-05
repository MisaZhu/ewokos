/* bsp_bt.c: machine.virt (qemu) has no bluetooth controller - no-op stub so
   the shared btd daemon links (bsp_bt contract, system/gui/libs/bt), the
   same role the bsp_usb stub plays on machines without USB host.
   bsp_bt_init() always fails, so even if btd were launched it would just
   sit in its bounded bring-up retry loop; init.rd never launches it. */
#include <bt/bsp_bt.h>

int bsp_bt_init(bool recovery) {
    (void)recovery;
    return -1;
}

void bsp_bt_power_off(void) {
}

int bsp_bt_send(uint8_t pkt_type, const uint8_t* data, size_t len) {
    (void)pkt_type;
    (void)data;
    (void)len;
    return -1;
}

int bsp_bt_recv(uint32_t timeout_ms) {
    (void)timeout_ms;
    return -1;
}

int bsp_bt_flush(void) {
    return 0;
}

void bsp_bt_firmware(const uint8_t** data, uint32_t* len) {
    *data = NULL;
    *len = 0;
}

void bsp_bt_diag_str(char* buf, size_t size) {
    if (size > 0) {
        buf[0] = 0;
    }
}
