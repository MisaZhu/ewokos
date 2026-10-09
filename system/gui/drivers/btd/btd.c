/* btd.c - core: HCI transport, wait machinery, event dispatch,
         controller bring-up, control commands and main loop.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h.
   Machine-neutral: UART/pinmux/power/firmware are the machine's
   bsp_bt implementation, linked in from its libbsp. */
#include "btd_int.h"

vdevice_t* _bt_dev = NULL;

charbuf_t* _evt_buf = NULL;

uint32_t _idle_sleep_us = BT_IDLE_SLEEP_MIN_US;

bool _ready = false;

bool _powered = false;

/* classic handle with an outstanding Set_Connection_Encryption: if the
   controller rejects it (no Encryption_Change event will follow) the
   Command_Complete fallback brings the HID channels up on the still-
   authenticated link instead of waiting forever */
uint16_t _sec_encrypt_handle = 0;

bt_wait_cmd_t _wait_cmd;

bt_wait_debug_t _wait_debug;

/* Bring-up runs from bt_loop, not the mount callback. device_run registers
   IPC before mounted, but starts loop_step only after mounted returns.
   0 = not yet run, 1 = initialized, 2 = waiting to retry a failed attempt.
   The single-threaded initialization sequence still waits for HCI replies. */
static int      _init_state    = 0;
static uint64_t _init_retry_ms = 0;
static bool     _known_loaded  = false;

/* deferred adapter work: devcmd handlers must answer at once (an xbt click
   blocks on the reply), so "open"'s firmware reload and the known-device
   autoconnect run from bt_loop instead of inside the command handler.
   _autoconnect_due_ms doubles as pending-flag (0 = none) and drop-deadline. */
static bool     _open_pending = false;
static uint64_t _autoconnect_due_ms = 0;
#define BT_AUTOCONNECT_DEFER_MS 15000

/* the handle of the last user-requested disconnect: the command is sent
   fire-and-forget, so its async Command Status is where a rejection gets
   reported from */
static uint16_t _disc_pending_handle = 0;

static void bt_wakeup_readers(void) {
    if (_bt_dev != NULL && _bt_dev->mnt_info.node != 0) {
        vfs_wakeup(_bt_dev->mnt_info.node, VFS_EVT_RD);
    }
}

void bt_emit(const char* fmt, ...) {
    va_list ap;
    char line[MAX_EVT_LINE];
    int len;
    int i;

    if (_evt_buf == NULL) {
        return;
    }

    va_start(ap, fmt);
    len = vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    if (len < 0) {
        return;
    }
    if (len >= (int)sizeof(line)) {
        len = (int)sizeof(line) - 1;
    }

    ipc_disable();
    for (i = 0; i < len; ++i) {
        charbuf_push(_evt_buf, line[i], true);
    }
    charbuf_push(_evt_buf, '\n', true);
    ipc_enable();
    bt_wakeup_readers();
}

static size_t bt_ret_len(const char* ret, size_t ret_sz) {
    size_t n = 0;

    while (n < ret_sz && ret[n] != 0) {
        ++n;
    }
    return n;
}

void bt_ret_append(char* ret, size_t ret_sz, const char* fmt, ...) {
    size_t used;
    va_list ap;

    if (ret == NULL || ret_sz == 0) {
        return;
    }

    used = bt_ret_len(ret, ret_sz);
    if (used >= (ret_sz - 1)) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(ret + used, ret_sz - used, fmt, ap);
    va_end(ap);
}

/* The H4 transport is the machine's bsp_bt (UART, pinmux, power sequencing
   and patchram image all live there); the daemon only forwards the byte
   stream through these thin wrappers so the rest of the core never names
   a register. */
int bt_hci_send_packet(uint8_t pkt_type, const uint8_t* data, size_t len) {
    return bsp_bt_send(pkt_type, data, len);
}

static int bt_uart_recv_timeout(uint32_t timeout_ms) {
    return bsp_bt_recv(timeout_ms);
}

static int bt_uart_flush(void) {
    return bsp_bt_flush();
}

/* short platform register snapshot ("lsr=0x.. msr=0x.." on the raspi5
   16550, "fr=0x.." on the raspix PL011, "" on a stub) for the timeout and
   error log lines; single-threaded daemon, static buffer, never NULL */
static const char* bt_diag_str(void) {
    static char diag[64];
    bsp_bt_diag_str(diag, sizeof(diag));
    return diag;
}

static int bt_hci_send_command_raw(uint16_t opcode, const uint8_t* params, uint8_t param_len) {
    uint8_t command[3 + 255];

    if (param_len > 0 && params == NULL) {
        return -1;
    }
    command[0] = hci_opcode_lo(opcode);
    command[1] = hci_opcode_hi(opcode);
    command[2] = param_len;
    if (param_len > 0) {
        memcpy(command + 3, params, param_len);
    }
    return bt_hci_send_packet(HCI_PKT_COMMAND, command, (size_t)param_len + 3);
}

int bt_hci_send_command(uint16_t ogf, uint16_t ocf, const uint8_t* params, uint8_t param_len) {
    return bt_hci_send_command_raw(HCI_OPCODE(ogf, ocf), params, param_len);
}

static void bt_update_wait_cmd_complete(uint16_t opcode, int status) {
    if (_wait_cmd.active && _wait_cmd.opcode == opcode) {
        _wait_cmd.status = status;
        _wait_cmd.done = true;
    }
}

static void bt_handle_command_complete(const uint8_t* payload, size_t len) {
    uint16_t opcode;
    int status = 0;

    if (len < 3) {
        return;
    }

    opcode = (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
    if (len >= 4) {
        status = payload[3];
    }
    /* capture the return parameters (everything after the status byte) so
       synchronous commands like READ_BUFFER_SIZE can read them back */
    if (_wait_cmd.active && _wait_cmd.opcode == opcode) {
        if (len > 4) {
            size_t n = len - 4;
            if (n > sizeof(_wait_cmd.ret)) {
                n = sizeof(_wait_cmd.ret);
            }
            memcpy(_wait_cmd.ret, payload + 4, n);
            _wait_cmd.ret_len = (uint8_t)n;
            _wait_cmd.got_ret = true;
        }
        else {
            _wait_cmd.ret_len = 0;
            _wait_cmd.got_ret = false;
        }
    }
    _wait_debug.last_opcode = opcode;
    _wait_debug.last_status = status;
    bt_update_wait_cmd_complete(opcode, status);

    /* classic HID fallback: Set_Connection_Encryption was rejected (the peer
       does not support link encryption), so no Encryption_Change event will
       arrive. The link is still authenticated, which is enough for many
       keyboards - bring the HID channels up now instead of waiting forever. */
    if (opcode == HCI_OPCODE(HCI_OGF_LINK_CTRL, HCI_OCF_SET_CONN_ENCRYPT) &&
            status != 0 && _sec_encrypt_handle != 0) {
        uint16_t h = _sec_encrypt_handle;
        bt_device_t* dev = bt_find_device_by_handle(h);
        _sec_encrypt_handle = 0;
        if (dev != NULL && dev->hid_after_sec) {
            dev->hid_after_sec = false;
            if (!bt_hogp_blocks_classic(dev)) {
                bt_hid_start(h, dev->addr);
            }
        }
    }

    /* Read RSSI completion: return params are [handle(2), rssi(1)]. Refresh
       the live signal strength of the connected link so xbt shows a real
       dBm value instead of the unknown placeholder. */
    if (opcode == HCI_OPCODE(HCI_OGF_STATUS, HCI_OCF_READ_RSSI) &&
            status == 0 && len >= 7) {
        uint16_t h = (uint16_t)payload[4] | ((uint16_t)payload[5] << 8);
        bt_device_t* d = bt_find_device_by_handle(h);
        if (d != NULL) {
            d->rssi = (int8_t)payload[6];
        }
    }
}

static void bt_handle_command_status(const uint8_t* payload, size_t len) {
    uint16_t opcode;
    int status;

    if (len < 4) {
        return;
    }

    status = payload[0];
    opcode = (uint16_t)payload[2] | ((uint16_t)payload[3] << 8);
    _wait_debug.last_opcode = opcode;
    _wait_debug.last_status = status;
    bt_update_wait_cmd_complete(opcode, status);

    if (_wait_cmd.active && _wait_cmd.opcode == opcode) {
        return; /* claimed by a synchronous wait (bring-up, autoconnect) */
    }

    /* completions for fire-and-forget commands: the devcmd reply could not
       wait for them, so a rejection is surfaced here instead */
    if (opcode == HCI_OPCODE(HCI_OGF_LINK_CTRL, HCI_OCF_CREATE_CONN)) {
        if (status != 0 &&
                (_pending.type == BT_PENDING_CONNECT ||
                 _pending.type == BT_PENDING_PAIR)) {
            char addr_str[24];
            bt_addr_to_str(_pending.addr, addr_str, sizeof(addr_str));
            bt_emit("%s_fail %s status=%d\n",
                _pending.type == BT_PENDING_PAIR ? "pair" : "connect",
                addr_str, status);
            bt_clear_pending();
        }
    }
    else if (opcode == HCI_OPCODE(HCI_OGF_LINK_CTRL, HCI_OCF_DISCONNECT)) {
        if (status != 0 && _disc_pending_handle != 0) {
            bt_emit("disconnect_fail handle=0x%04X status=%d\n",
                _disc_pending_handle, status);
        }
        _disc_pending_handle = 0;
    }
}

int bt_hci_command_sync_ret(uint16_t ogf, uint16_t ocf,
        const uint8_t* params, uint8_t param_len, uint32_t timeout_ms,
        uint8_t* ret_params, uint8_t ret_cap, uint8_t* ret_len) {
    uint16_t opcode = HCI_OPCODE(ogf, ocf);
    int status;

    if (ret_len != NULL) {
        *ret_len = 0;
    }
    if (bt_hci_send_command_raw(opcode, params, param_len) != 0) {
        slog("bluetooth cmd send_failed opcode=0x%04x\n", opcode);
        return -1;
    }
    status = bt_wait_for_opcode(opcode, timeout_ms);
    if (status == 0 && ret_params != NULL && ret_len != NULL && _wait_cmd.got_ret) {
        uint8_t n = _wait_cmd.ret_len;
        if (n > ret_cap) {
            n = ret_cap;
        }
        memcpy(ret_params, _wait_cmd.ret, n);
        *ret_len = n;
    }
    return status;
}

static int bt_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        int oflag, void* p) {
    return hid_vdev_open(dev, fd, from_pid, node, oflag, p);
}

static int bt_vdev_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
        fsinfo_t* fsinfo, void* p) {
    return hid_vdev_close(dev, fd, from_pid, node, fsinfo, p);
}

static int bt_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p) {
    return hid_vdev_fcntl(dev, fd, from_pid, info, cmd, in, out, p);
}

static void bt_handle_event(uint8_t event_code, const uint8_t* payload, size_t len) {
    _wait_debug.last_event_code = event_code;
    _wait_debug.last_event_len = (uint8_t)len;
    switch (event_code) {
    case EVT_CMD_COMPLETE:
        bt_handle_command_complete(payload, len);
        break;
    case EVT_CMD_STATUS:
        bt_handle_command_status(payload, len);
        break;
    case EVT_HARDWARE_ERROR:
        /* the cyw chip reports its post-launch fault through this: the
           1-byte code is the only clue it gives us */
        slog("bluetooth hw_error code=0x%02x\n", len > 0 ? payload[0] : 0xff);
        break;
    case EVT_INQUIRY_COMPLETE:
        /* the inquiry slice ended, not the scan session: bt_le_step owns
           _scanning and either flips to the LE slice or closes the
           session when its total time is up */
        _inquiry_running = false;
        if (_scanning && _scan_slice == BT_SCAN_SLICE_CLASSIC) {
            _scan_slice_end_ms = 0; /* hand over immediately */
        }
        if (!_scanning) {
            bt_emit("scan_done status=%u", len > 0 ? payload[0] : 0xff);
        }
        break;
    case EVT_INQUIRY_RESULT:
        bt_handle_inquiry_result(payload, len);
        break;
    case EVT_INQUIRY_RESULT_RSSI:
        bt_handle_inquiry_result_rssi(payload, len);
        break;
    case EVT_EXTENDED_INQUIRY_RESULT:
        bt_handle_extended_inquiry_result(payload, len);
        break;
    case EVT_REMOTE_NAME_COMPLETE:
        bt_handle_remote_name_complete(payload, len);
        break;
    case EVT_CONN_COMPLETE:
        bt_handle_connection_complete(payload, len);
        break;
    case EVT_DISCONN_COMPLETE:
        bt_handle_disconnection_complete(payload, len);
        break;
    case EVT_AUTH_COMPLETE:
        bt_handle_auth_complete(payload, len);
        break;
    case EVT_PIN_CODE_REQUEST:
        bt_handle_pin_code_request(payload, len);
        break;
    case EVT_LINK_KEY_REQUEST:
        bt_handle_link_key_request(payload, len);
        break;
    case EVT_LINK_KEY_NOTIFY:
        bt_handle_link_key_notify(payload, len);
        break;
    case EVT_CONN_REQUEST:
        bt_handle_conn_request(payload, len);
        break;
    case EVT_IO_CAPABILITY_REQUEST:
        bt_handle_io_capability_request(payload, len);
        break;
    case EVT_USER_CONFIRMATION_REQUEST:
        bt_handle_user_confirmation_request(payload, len);
        break;
    case EVT_USER_PASSKEY_REQUEST:
        bt_handle_user_passkey_request(payload, len);
        break;
    case EVT_SIMPLE_PAIRING_COMPLETE:
        bt_handle_simple_pairing_complete(payload, len);
        break;
    case EVT_ENCRYPTION_CHANGE:
        bt_handle_encryption_change(payload, len);
        break;
    case EVT_ENCRYPTION_KEY_REFRESH: {
        /* SC bonds re-encrypt with a key refresh (no enable byte), not a
           change event; normalize to the change layout or the deferred
           classic HID bring-up would never fire on an SC reconnect. */
        uint8_t norm[4];
        if (len >= 3) {
            norm[0] = payload[0];
            norm[1] = payload[1];
            norm[2] = payload[2];
            norm[3] = payload[0] == 0 ? 1 : 0;
            bt_handle_encryption_change(norm, sizeof(norm));
        }
        break;
    }
    case EVT_LE_META:
        bt_handle_le_meta(payload, len);
        break;
    case EVT_NUM_COMPLETED_PKTS:
        bt_handle_num_completed_pkts(payload, len);
        break;
    default:
        break;
    }
}

static int bt_recv_exact(uint8_t* buf, size_t len, uint32_t timeout_ms) {
    size_t i;

    for (i = 0; i < len; ++i) {
        int c = bt_uart_recv_timeout(timeout_ms);
        if (c < 0) {
            return -1;
        }
        buf[i] = (uint8_t)c;
    }
    return 0;
}

static void bt_drop_bytes(size_t len) {
    uint8_t scratch[32];

    while (len > 0) {
        size_t step = len > sizeof(scratch) ? sizeof(scratch) : len;
        if (bt_recv_exact(scratch, step, 10) != 0) {
            return;
        }
        len -= step;
    }
}

int bt_poll_once(uint32_t first_timeout_ms) {
    int pkt_type;
    uint8_t hdr[4];
    uint8_t payload[MAX_HCI_PAYLOAD];
    uint16_t acl_len;

    /* bsp_bt_recv reports "no byte" while the transport is down, so a poll
       before/after a failed bring-up is a safe no-op */
    pkt_type = bt_uart_recv_timeout(first_timeout_ms);
    if (pkt_type < 0) {
        return 0;
    }

    ++_wait_debug.packets_seen;
    _wait_debug.last_pkt_type = (uint8_t)pkt_type;

    if (pkt_type == HCI_PKT_EVENT) {
        ++_wait_debug.event_packets;
        if (bt_recv_exact(hdr, 2, BT_UART_PKT_FOLLOW_TIMEOUT_MS) != 0) {
            slog("bluetooth poll event_header_timeout %s\n", bt_diag_str());
            return -1;
        }
        if (bt_recv_exact(payload, hdr[1], BT_UART_PKT_FOLLOW_TIMEOUT_MS) != 0) {
            slog("bluetooth poll event_payload_timeout evt=0x%02x len=%u %s\n",
                hdr[0], hdr[1], bt_diag_str());
            return -1;
        }
        bt_handle_event(hdr[0], payload, hdr[1]);
        return 1;
    }

    if (pkt_type == HCI_PKT_ACL) {
        uint16_t handle;
        uint8_t pb;

        ++_wait_debug.acl_packets;
        if (bt_recv_exact(hdr, 4, BT_UART_PKT_FOLLOW_TIMEOUT_MS) != 0) {
            slog("bluetooth poll acl_header_timeout %s\n", bt_diag_str());
            return -1;
        }
        acl_len = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
        handle = (uint16_t)(((uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8)) & 0x0fff);
        pb = (uint8_t)((hdr[1] >> 4) & 0x03);

        {
            uint8_t fragment[4 + L2CAP_MTU_DEFAULT];
            if (acl_len > sizeof(fragment)) {
                bt_drop_bytes(acl_len);
                return 1;
            }
            if (bt_recv_exact(fragment, acl_len, BT_UART_PKT_FOLLOW_TIMEOUT_MS) != 0) {
                slog("bluetooth poll acl_payload_timeout h=0x%04x\n", handle);
                return -1;
            }
            l2cap_recv_acl(handle, pb, fragment, acl_len);
        }
        return 1;
    }

    ++_wait_debug.other_packets;

    return 1;
}

int bt_wait_for_opcode(uint16_t opcode, uint32_t timeout_ms) {
    uint32_t waited = 0;

    memset(&_wait_cmd, 0, sizeof(_wait_cmd));
    _wait_cmd.active = true;
    _wait_cmd.opcode = opcode;
    _wait_cmd.status = -1;
    memset(&_wait_debug, 0, sizeof(_wait_debug));
    _wait_debug.last_pkt_type = 0xff;
    _wait_debug.last_event_code = 0xff;
    _wait_debug.last_event_len = 0xff;
    _wait_debug.last_opcode = 0xffff;
    _wait_debug.last_status = -1;

    /* wall-clock bound: a continuous garbage rx stream (wrong baud, floating
       line) keeps bt_poll_once returning > 0, so counting only idle polls
       never reaches the timeout and the daemon hangs */
    {
        uint64_t start_ms = kernel_tic_ms(0);
        while (!_wait_cmd.done && (uint32_t)(kernel_tic_ms(0) - start_ms) < timeout_ms) {
            if (bt_poll_once(1) <= 0) {
                ++waited;
            }
            l2cap_step();
        }
        waited = (uint32_t)(kernel_tic_ms(0) - start_ms);
    }

    _wait_cmd.active = false;
    if (!_wait_cmd.done) {
        slog("bluetooth wait timeout opcode=0x%04x waited=%u seen=%u evt=%u acl=%u other=%u last_pkt=0x%02x last_evt=0x%02x len=%u last_opcode=0x%04x last_status=%d %s\n",
            opcode, waited, _wait_debug.packets_seen, _wait_debug.event_packets,
            _wait_debug.acl_packets, _wait_debug.other_packets,
            _wait_debug.last_pkt_type, _wait_debug.last_event_code,
            _wait_debug.last_event_len, _wait_debug.last_opcode,
            _wait_debug.last_status, bt_diag_str());
    }
    return _wait_cmd.done ? _wait_cmd.status : -1;
}

int bt_hci_command_sync(uint16_t ogf, uint16_t ocf,
        const uint8_t* params, uint8_t param_len, uint32_t timeout_ms) {
    uint16_t opcode = HCI_OPCODE(ogf, ocf);

    if (bt_hci_send_command_raw(opcode, params, param_len) != 0) {
        slog("bluetooth cmd send_failed opcode=0x%04x\n", opcode);
        return -1;
    }
    return bt_wait_for_opcode(opcode, timeout_ms);
}

static int bt_load_firmware(void) {
    uint32_t offset = 0;
    uint32_t chunk_idx = 0;
    uint8_t opcodebytes[2];
    uint8_t length;
    const uint8_t* data;
    uint32_t size;
    int ret;

    /* the machine's bsp supplies the patchram image when its combo chip
       needs one (CYW43455 on raspi5/raspix); a platform may legitimately
       need no download at all */
    bsp_bt_firmware(&data, &size);
    if (data == NULL || size == 0) {
        return 0;
    }

    ret = bt_hci_command_sync(HCI_OGF_VENDOR, HCI_OCF_VENDOR_LOAD_FIRMWARE, NULL, 0, 1000);
    if (ret != 0) {
        slog("bluetooth fw enter_download_failed ret=%d\n", ret);
        return ret;
    }

    /* Broadcom Download Minidrv requires 50ms after its completion. The
       faster FIFO transfer must not rely on accidental per-byte delays. */
    usleep(50000);
    while (offset < size) {
        uint16_t opcode;

        if (size - offset < 3 || (uint32_t)data[2] > size - offset - 3) {
            slog("bluetooth fw malformed_chunk idx=%u offset=%u\n", chunk_idx, offset);
            return -1;
        }
        opcodebytes[0] = *data++;
        opcodebytes[1] = *data++;
        length = *data++;
        opcode = (uint16_t)opcodebytes[0] | ((uint16_t)opcodebytes[1] << 8);

        ret = bt_hci_send_command_raw(opcode, data, length);
        if (ret != 0) {
            slog("bluetooth fw send_chunk_failed idx=%u offset=%u opcode=0x%04x len=%u ret=%d\n",
                chunk_idx, offset, opcode, length, ret);
            return ret;
        }
        ret = bt_wait_for_opcode(opcode, 1000);
        if (ret != 0) {
            slog("bluetooth fw wait_chunk_failed idx=%u offset=%u opcode=0x%04x len=%u ret=%d\n",
                chunk_idx, offset, opcode, length, ret);
            return ret;
        }

        data += length;
        offset += (uint32_t)length + 3;
        ++chunk_idx;
    }

    return 0;
}

static int bt_configure_controller(void) {
    uint8_t mask[8] = {0xff, 0xff, 0xfb, 0xff, 0x07, 0xf8, 0xbf, 0x3d};
    uint8_t scan_enable = 0x03;
    uint8_t auth_enable = 0x01;
    uint8_t inquiry_mode = 0x02;
    uint8_t simple_pair = 0x01;
    uint8_t sec_conn = 0x01;
    /* Present as a laptop-class host (Windows CoD): the firmware default
       0x000000 makes us a classless host to anything that reads our FHS at
       page time, and host-adaptive device firmware classifies by it before
       it ever browses SDP. */
    uint8_t host_cod[3] = {0x0c, 0x01, 0x7a};
    /* Allow peer-initiated sniff mode and role switch, as Linux, Windows
       and the BTstack HID host all do. A HID peripheral parks its link in
       sniff mode right after setup to save power; at the reset default
       (0x0000) the controller answers every LMP_sniff_req with
       LMP_not_accepted and the host never learns about it. Hold and park
       stay disabled: neither is used by HID devices. */
    uint8_t link_policy[2] = {
        (uint8_t)(HCI_LP_ROLE_SWITCH | HCI_LP_SNIFF), 0x00
    };
    uint8_t buf_ret[8];
    uint8_t buf_ret_len = 0;

    /* bt_driver_init already completed the post-firmware Reset. */
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_SET_EVENT_MASK, mask, sizeof(mask), 1000);
    /* READ_BUFFER_SIZE (OGF_INFO 0x04, ret: status already stripped ->
       [acl_pkt_len_lo, acl_pkt_len_hi, sco_len, acl_num, sco_num]):
       acl_num is our host->controller ACL credit pool */
    if (bt_hci_command_sync_ret(HCI_OGF_INFO, HCI_OCF_READ_BUFFER_SIZE, NULL, 0,
            1000, buf_ret, sizeof(buf_ret), &buf_ret_len) == 0 && buf_ret_len >= 5) {
        _acl_credits = (uint16_t)buf_ret[3] | ((uint16_t)buf_ret[4] << 8);
    }
    if (_acl_credits == 0) {
        _acl_credits = 8; /* sane fallback if the controller stays mute */
    }
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_AUTH_ENABLE, &auth_enable, 1, 1000);
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_CLASS_OF_DEVICE, host_cod, sizeof(host_cod), 1000);
    (void)bt_hci_command_sync(HCI_OGF_LINK_POLICY,
        HCI_OCF_WRITE_DEF_LINK_POLICY, link_policy, sizeof(link_policy), 1000);
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_INQUIRY_MODE, &inquiry_mode, 1, 1000);
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_SIMPLE_PAIRING_MODE, &simple_pair, 1, 1000);
    /* Pair like every real host (Windows/macOS/BlueZ all enable this): with
       Secure Connections host support, NEW bonds derive the link key over
       P-256/AES instead of legacy E22. Existing legacy bonds keep working
       unchanged, and a controller that rejects the command just stays at
       legacy behavior — log the result so the re-pair test can tell
       "controller refused" apart from "pad negotiated legacy anyway". */
    {
        int sc_ret = bt_hci_command_sync(HCI_OGF_HOST_CTRL,
            HCI_OCF_WRITE_SECURE_CONN_HOST, &sec_conn, 1, 1000);
        slog("bluetooth sec_conn ret=%d\n", sc_ret);
    }
    (void)bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_SCAN_ENABLE, &scan_enable, 1, 1000);
    /* LE support is optional: a controller that refuses LE_Set_Event_Mask
       simply leaves _le_supported clear and discovery stays classic-only */
    (void)bt_le_controller_init();
    return 0;
}

static int bt_driver_init(void) {
    int ret;
    int attempt;

    /* the machine's bsp maps its MMIO window, sequences the combo-chip
       power (a retry passes recovery=true for the long off-pulse) and
       brings the HCI UART up at the default baud; failing here keeps the
       daemon in bt_loop's bounded bring-up retry */
    if (bsp_bt_init(_init_state == 2) != 0) {
        slog("bluetooth init transport_failed\n");
        return -1;
    }
    bt_uart_flush();
    /* BT_ON release already allowed 100ms (300ms on recovery) to settle. */
    ret = bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_RESET, NULL, 0, 3000);
    if (ret != 0) {
        slog("bluetooth init reset_chip_failed ret=%d\n", ret);
        return ret;
    }
    usleep(100000); /* Broadcom reset-completion settling time */

    ret = bt_load_firmware();
    if (ret != 0) {
        slog("bluetooth init load_firmware_failed ret=%d\n", ret);
        return ret;
    }
    usleep(300000);

    /* the patchram launch record reboots the chip into RAM firmware; the
       uart glitches during the handoff (stray 0x00 bytes) and the fresh
       firmware may need a few hundred ms before answering - flush the
       garbage and retry the post-fw reset instead of failing once. A chip
       that is truly wedged never answers these, so keep the window short;
       recovery for that case is the outer power-cycle retry in bt_loop. */
    ret = -1;
    for (attempt = 0; attempt < 3 && ret != 0; ++attempt) {
        usleep(200000);
        bt_uart_flush();
        ret = bt_hci_command_sync(HCI_OGF_HOST_CTRL, HCI_OCF_RESET, NULL, 0, 1000);
        if (ret != 0) {
            slog("bluetooth init post_fw_reset retry=%d ret=%d\n", attempt, ret);
        }
    }
    if (ret != 0) {
        slog("bluetooth init post_fw_reset_failed\n");
        /* is anything still alive out there? READ_LOCAL_VERSION is legal
           both in rom download mode and in the patchram firmware */
        if (bt_hci_command_sync(HCI_OGF_INFO, HCI_OCF_READ_LOCAL_VERSION, NULL, 0, 500) == 0) {
            slog("bluetooth init post_fw_probe answered (chip alive, fw state wrong)\n");
        }
        else {
            slog("bluetooth init post_fw_probe silent (chip wedged)\n");
        }
        return ret;
    }

    usleep(100000); /* settle the successful post-firmware Reset */
    ret = bt_configure_controller();
    if (ret != 0) {
        slog("bluetooth init configure_controller_failed ret=%d\n", ret);
        return ret;
    }

    _ready = true;
    return 0;
}

static int bt_start_connection(const uint8_t* addr, bool pair, const char* pin,
        char* ret_text, size_t ret_text_sz) {
    bt_device_t* dev = bt_find_device(addr, true);
    char addr_str[24];
    int ret;
    const char* action = pair ? "pair" : "connect";

    if (dev == NULL) {
        bt_addr_to_str(addr, addr_str, sizeof(addr_str));
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "%s_fail %s reason=unknown_device\n", action, addr_str);
        }
        return -1;
    }

    bt_addr_to_str(addr, addr_str, sizeof(addr_str));

    /* a BLE-only peripheral has no BR/EDR page to answer, so it takes the
       LE bring-up path instead; a dual-mode device keeps the classic one,
       which already works and needs no SMP round */
    if (dev->le && !dev->classic) {
        return bt_le_request(dev, pair, ret_text, ret_text_sz);
    }

    if (dev->connected) {
        if (pair) {
            memset(&_pending, 0, sizeof(_pending));
            _pending.type = BT_PENDING_PAIR;
            memcpy(_pending.addr, addr, 6);
            _pending.handle = dev->handle;
            strncpy(_pending.pin, (pin != NULL && pin[0] != 0) ? pin : "0000", sizeof(_pending.pin) - 1);
            bt_hci_auth_request(dev->handle);
            if (ret_text != NULL && ret_text_sz != 0) {
                snprintf(ret_text, ret_text_sz, "pair_wait_auth %s\n", addr_str);
            }
            return 0;
        }
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "connect_ok %s handle=0x%04X\n", addr_str, dev->handle);
        }
        return 0;
    }

    memset(&_pending, 0, sizeof(_pending));
    _pending.type = pair ? BT_PENDING_PAIR : BT_PENDING_CONNECT;
    memcpy(_pending.addr, addr, 6);
    strncpy(_pending.pin, (pin != NULL && pin[0] != 0) ? pin : "0000", sizeof(_pending.pin) - 1);

    ret = bt_hci_create_connection(dev);
    if (ret != 0) {
        bt_clear_pending();
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "%s_fail %s status=%d\n", action, addr_str, ret);
        }
        return ret;
    }

    /* no wait for the Command Status: this reply must come back at once
       (xbt's click blocks on it). A controller rejection is reported by the
       asynchronous command-status handler; success and page failure both
       arrive as Connection Complete events. */
    if (ret_text != NULL && ret_text_sz != 0) {
        snprintf(ret_text, ret_text_sz, "%s_begin %s", action, addr_str);
    }
    return 0;
}

/* walk the persistent store and page every device we connected with
   before; each attempt is bounded (1.5s cmd status + 3s page wait) so a
   drawer full of absent devices cannot stall the daemon for long */
void bt_autoconnect_known(void) {
    int le_known = 0;
    int i;

    if (!_ready || _scanning || _pending.type != BT_PENDING_NONE) {
        return;
    }

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        bt_device_t* dev;
        char addr_str[24];
        uint64_t start_ms;

        if (!_known[i].used) {
            continue;
        }
        /* a bonded LE peripheral can only be reached while it advertises,
           so paging it is pointless: it is counted here and picked up by
           the bounded scan session armed below */
        if (_known[i].le && !_known[i].has_key) {
            int s;
            bool live = false;
            /* don't count one already linked on a session, or a scan would
               be armed for a device that is already streaming */
            for (s = 0; s < MAX_LE_SESSIONS; ++s) {
                if (_les[s].le.handle_valid &&
                        bt_addr_equal(_les[s].le.addr, _known[i].addr)) {
                    live = true;
                    break;
                }
            }
            if (!live) {
                ++le_known;
            }
            continue;
        }
        dev = bt_find_device(_known[i].addr, true);
        if (dev == NULL || dev->connected) {
            continue;
        }

        bt_addr_to_str(_known[i].addr, addr_str, sizeof(addr_str));
        bt_emit("auto_connect %s\n", addr_str);
        if (bt_hci_create_connection(dev) != 0) {
            continue;
        }
        if (bt_wait_for_opcode(HCI_OPCODE(HCI_OGF_LINK_CTRL, HCI_OCF_CREATE_CONN), 1500) != 0) {
            continue;
        }
        /* the conn-complete event arrives asynchronously; poll it out */
        start_ms = kernel_tic_ms(0);
        while (!dev->connected &&
                (uint32_t)(kernel_tic_ms(0) - start_ms) < 3000) {
            bt_poll_once(10);
            l2cap_step();
        }
    }

    if (le_known > 0 && _le_supported && !_scanning && !_le_req_active &&
            le_session_free() >= 0) {
        _le_autoconnect = true;
        (void)bt_start_scan(BT_LE_AUTOCONNECT_SCAN_S);
    }
}

/* Re-arm the bounded LE autoconnect scan periodically so a bonded mouse
   that was asleep (or out of range) at power-on still reconnects on its own
   once it advertises again. Deliberately lightweight and non-blocking: it
   never pages classic devices and never waits, it only starts a scan session
   when some known LE peripheral is still not on a session.
   The cadence has to stay tight: an Xbox controller only advertises for a
   ~10s window after power-on, and a 30s arm cycle left the radio blind
   two thirds of the time, so the window was routinely missed and the pad
   never came back. With a 10s scan and a 3s re-arm gap the reconnect scan
   is near-continuous while nothing is linked; once a session is live the
   loop below finds it and stops arming, and bt_le_step suspends the slices
   while any HID link is streaming, so this costs radio time only when some
   known peripheral is actually absent. */
#define BT_LE_AUTOCONNECT_RETRY_MS 3000
static uint64_t _le_autoconnect_retry_ms = 0;

/* arm the reconnect scan on the next bt_loop tick (e.g. right after an LE
   link dropped, instead of waiting out the retry interval) */
void bt_le_autoconnect_kick(void) {
    _le_autoconnect_retry_ms = 0;
}

/* Discovery window for a first-time (not yet bonded) LE-only gamepad. The
   known-record loop in bt_le_autoconnect_retry_step can only keep the scan
   armed for a device that is already in the bond store; a pad that has never
   been paired has no such record, so on a fresh store nothing would ever scan
   for it and it could never be discovered. When one pages classic
   (bt_le_only_classic_migrate) it is physically present and trying to
   connect, so open a bounded window during which the retry step keeps the
   scan armed on its behalf.

   The window SLIDES: every kick refreshes the deadline. A pad in pairing mode
   re-pages classic every ~15s and only advertises its real LE identity BETWEEN
   pages, so the discovery scan must stay armed continuously or the LE link
   never wins and the classic migrate/disconnect loop runs forever (the pad's
   LE identity is never on the air at the instant a one-shot scan happens to be
   listening). Because a kick only arrives while the pad is physically present
   and re-paging, the window is self-limiting: it lapses BT_LE_DISCOVERY_MS
   after the pad goes quiet, and once the LE link is up bt_hid_live() stops the
   retry step re-arming it at all.

   The two harms an always-open window would cause are handled elsewhere, so
   the slide is safe: (1) a manual `scan` preempts the background scan and
   installs a cooldown via bt_le_discovery_close (so it never sees scan_busy),
   and (2) bt_le_step alternates LE/classic slices while the window is open and
   no HID link is live (so BR/EDR inquiry is never starved and an Xbox pad stays
   discoverable). The _le_discovery_not_before_ms cooldown is therefore ONLY a
   manual-scan guard, not a per-window throttle. */
#define BT_LE_DISCOVERY_MS 30000
#define BT_LE_DISCOVERY_GAP_MS 30000
static uint64_t _le_discovery_until_ms = 0;
static uint64_t _le_discovery_not_before_ms = 0;

void bt_le_discovery_kick(void) {
    uint64_t now = kernel_tic_ms(0);

    /* Honour ONLY the cooldown a manual `scan` installed
       (bt_le_discovery_close) so the foreground scan gets uninterrupted radio
       time; otherwise slide the deadline so a looping pad keeps the LE scan
       armed until its LE identity is caught. */
    if (now < _le_discovery_not_before_ms) {
        return;
    }
    _le_discovery_until_ms = now + BT_LE_DISCOVERY_MS;
    bt_le_autoconnect_kick();
}

/* Close the discovery window and start its cooldown immediately. A manual
   `scan` calls this so the background window cannot re-arm over the user's
   foreground scan; auto-connect resumes on a later kick once the gap elapses. */
static void bt_le_discovery_close(void) {
    _le_discovery_until_ms = 0;
    _le_discovery_not_before_ms = kernel_tic_ms(0) + BT_LE_DISCOVERY_GAP_MS;
}

/* True while a first-time discovery window opened by bt_le_discovery_kick is
   still running. bt_le_step uses this to tell a first-time discovery scan apart
   from a background known-device reconnect: the reconnect stays LE-only so it
   cannot page-scan over a live HID link, but a first-time window alternates
   LE/classic slices so it cannot starve BR/EDR inquiry and hide other
   controllers. */
bool bt_le_discovery_active(void) {
    return _le_discovery_until_ms != 0 && kernel_tic_ms(0) < _le_discovery_until_ms;
}

static void bt_le_autoconnect_retry_step(void) {
    uint64_t now = kernel_tic_ms(0);
    int i, s;

    if (_le_autoconnect_retry_ms != 0 && now < _le_autoconnect_retry_ms) {
        return;
    }
    _le_autoconnect_retry_ms = now + BT_LE_AUTOCONNECT_RETRY_MS;

    /* Never arm a reconnect scan while an input link is live. The by-address
       "live" test below only matches a record whose OWN address is on a
       session; a dual-identity pad migrated off classic (a GameSir whose LE
       BDADDR differs from its classic one, converted by
       bt_known_prefer_le_by_name) leaves a le&&!has_key record whose classic
       address never goes live, so without this gate the step re-arms a scan
       every 3s even while the pad streams - and the latched _le_autoconnect
       defeats bt_le_step's radio-contention suspension, starving the live
       link. bt_hid_live() is the transport-independent "something is streaming"
       test that restores the intended stop-while-live behaviour; it clears the
       moment the link drops, so reconnect re-arms normally. */
    if (!_ready || !_le_supported || _scanning || _le_req_active ||
            _pending.type != BT_PENDING_NONE || le_session_free() < 0 ||
            bt_hid_live()) {
        return;
    }
    /* Bounded first-time discovery window opened by bt_le_only_classic_migrate
       when a never-bonded LE-only pad pages classic. The known-record loop
       below can only arm a scan for a device already in the bond store; a
       first-time GameSir has no `le && !has_key` record (its real LE identity
       is a DIFFERENT BDADDR that has never been seen), so without this window
       nothing ever arms the scan and the pad's appearance-964 BLE identity is
       never discovered even while it advertises. Presence is proven by the
       incoming classic page, so arm discovery on the window alone, independent
       of store contents. The window slides (see bt_le_discovery_kick): every
       re-page refreshes the deadline so the LE scan stays armed continuously
       until the pad's LE identity is caught, and it lapses BT_LE_DISCOVERY_MS
       after the pad goes quiet. A manual `scan` and the LE/classic alternation
       in bt_le_step keep it from locking out the user or starving inquiry. */
    if (_le_discovery_until_ms != 0 && now < _le_discovery_until_ms) {
        _le_autoconnect = true;
        (void)bt_start_scan(BT_LE_AUTOCONNECT_SCAN_S);
        return;
    }
    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        bool live = false;

        if (!_known[i].used || !(_known[i].le && !_known[i].has_key)) {
            continue;
        }
        for (s = 0; s < MAX_LE_SESSIONS; ++s) {
            if (_les[s].le.handle_valid &&
                    bt_addr_equal(_les[s].le.addr, _known[i].addr)) {
                live = true;
                break;
            }
        }
        if (!live) {
            _le_autoconnect = true;
            (void)bt_start_scan(BT_LE_AUTOCONNECT_SCAN_S);
            return;
        }
    }
}

/* A peer belongs in the live `devices` list only while it is actually on the
   air: connected now, or detected within the last BT_DEVICE_PRESENT_MS. A bond
   seeded from /etc/bt/bt.json that is not currently answering (last_seen_ms ==
   0, or stale) is left out - presence governs the live list, while the
   persistent bond stays visible in `known`. last_seen_ms is stamped on every
   live sighting (LE advertising report, classic inquiry result, connection
   complete); the != 0 guard keeps a never-seeded-never-seen store record from
   passing the window test during the first BT_DEVICE_PRESENT_MS after boot. */
#define BT_DEVICE_PRESENT_MS 30000
static bool bt_device_present(const bt_device_t* dev, uint64_t now) {
    if (dev->connected) {
        return true;
    }
    return dev->last_seen_ms != 0 &&
            (uint32_t)(now - dev->last_seen_ms) < BT_DEVICE_PRESENT_MS;
}

static void bt_list_devices_ret(char* ret, size_t ret_sz) {
    uint64_t now = kernel_tic_ms(0);
    int i;
    int count = 0;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (!_devices[i].used || !bt_device_present(&_devices[i], now)) {
            continue;
        }
        bt_ret_append_device_line(i, ret, ret_sz, "device", &_devices[i]);
        ++count;
    }
    bt_ret_append(ret, ret_sz, "devices_done count=%d\n", count);
}

static void bt_list_known_ret(char* ret, size_t ret_sz) {
    int i;
    int count = 0;

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        char addr[24];

        if (!_known[i].used) {
            continue;
        }
        bt_addr_to_str(_known[i].addr, addr, sizeof(addr));
        bt_ret_append(ret, ret_sz, "%d: known %s paired=%d key=%d name=%s\n",
            i,
            addr,
            _known[i].paired ? 1 : 0,
            _known[i].has_key ? 1 : 0,
            _known[i].name[0] ? _known[i].name : "-");
        ++count;
    }
    bt_ret_append(ret, ret_sz, "known_done count=%d\n", count);
}

static int bt_forget_device(const char* arg, char* ret, size_t ret_sz) {
    uint8_t addr[6];
    bt_known_t* k;
    bt_device_t* dev;

    if (!bt_parse_addr(arg, addr)) {
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "forget_fail reason=bad_addr\n");
        }
        return -1;
    }

    k = bt_known_find(addr);
    if (k == NULL) {
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "forget_fail %s reason=not_found\n", arg);
        }
        return -1;
    }

    k->used = false;
    /* drop the cached link key too, or the next connect would silently
       reuse the very credentials the user just asked to forget */
    dev = bt_find_device(addr, false);
    if (dev != NULL) {
        dev->has_link_key = false;
        memset(dev->link_key, 0, sizeof(dev->link_key));
        dev->has_ltk = false;
        memset(dev->ltk, 0, sizeof(dev->ltk));
        memset(dev->ltk_rand, 0, sizeof(dev->ltk_rand));
        dev->ediv = 0;
    }
    bt_known_save();
    if (ret != NULL && ret_sz != 0) {
        snprintf(ret, ret_sz, "forget_ok %s\n", arg);
    }
    return 0;
}

/* "unpair" = forget + drop the live link: forget only rewrites the store,
   so a connection authenticated with the old key would otherwise keep
   running (an LE session would even keep streaming reports). Works on a
   runtime-only bond too (store write pending or failed), and evicts the
   peer from the controller's resolving list when it had an IRK, or a
   rotating private address would keep resolving onto a bond that no longer
   exists. */
static int bt_unpair_device(const char* arg, char* ret, size_t ret_sz) {
    uint8_t addr[6];
    uint8_t id_addr[6];
    uint8_t id_addr_type = 0;
    bool have_id = false;
    bt_known_t* k;
    bt_device_t* dev;
    int s;

    if (!bt_parse_addr(arg, addr)) {
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "unpair_fail reason=bad_addr\n");
        }
        return -1;
    }

    k = bt_known_find(addr);
    dev = bt_find_device(addr, false);
    if (k == NULL && (dev == NULL ||
            (!dev->has_link_key && !dev->has_ltk && !dev->has_irk))) {
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "unpair_fail %s reason=not_paired\n", arg);
        }
        slog("bt unpair_fail %s reason=not_paired\n", arg);
        return -1;
    }

    /* every live link to the peer goes down first: the classic link on
       dev->handle, plus any LE session carrying this address. All sends are
       fire-and-forget (this reply must not stall): the disconnect-complete
       events do the session/channel teardown through the normal handlers. */
    if (dev != NULL && dev->connected) {
        bt_hci_disconnect(dev->handle);
    }
    for (s = 0; s < MAX_LE_SESSIONS; ++s) {
        if (_les[s].le.handle_valid && bt_addr_equal(_les[s].le.addr, addr)) {
            bt_hci_disconnect(_les[s].le.handle);
        }
    }

    if (dev != NULL && dev->has_irk && dev->has_id_addr) {
        have_id = true;
        id_addr_type = dev->id_addr_type;
        memcpy(id_addr, dev->id_addr, 6);
    }
    else if (k != NULL && k->has_irk && k->has_id_addr) {
        have_id = true;
        id_addr_type = k->id_addr_type;
        memcpy(id_addr, k->id_addr, 6);
    }
    if (have_id && _ready && _le_resolving) {
        uint8_t p[7];
        p[0] = id_addr_type;
        memcpy(p + 1, id_addr, 6);
        (void)bt_hci_send_command(HCI_OGF_LE, HCI_OCF_LE_REMOVE_DEV_RESOLV_LIST,
                p, sizeof(p));
    }

    if (dev != NULL) {
        dev->has_link_key = false;
        memset(dev->link_key, 0, sizeof(dev->link_key));
        dev->has_ltk = false;
        memset(dev->ltk, 0, sizeof(dev->ltk));
        memset(dev->ltk_rand, 0, sizeof(dev->ltk_rand));
        dev->ediv = 0;
        dev->has_irk = false;
        memset(dev->irk, 0, sizeof(dev->irk));
        dev->has_id_addr = false;
        memset(dev->id_addr, 0, sizeof(dev->id_addr));
    }
    if (k != NULL) {
        k->used = false;
        bt_known_save();
    }
    bt_emit("unpair_ok %s\n", arg);
    /* mirror into /dev/log: a re-pair test is otherwise impossible to
       verify from a plain log capture (bt_emit only feeds /dev/bt0) */
    slog("bt unpair_ok %s\n", arg);
    if (ret != NULL && ret_sz != 0) {
        snprintf(ret, ret_sz, "unpair_ok %s\n", arg);
    }
    return 0;
}

static void bt_dump_state_ret(char* ret, size_t ret_sz) {
    int i;
    int count = 0;
    int le_only = 0;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (_devices[i].used) {
            ++count;
            if (_devices[i].le && !_devices[i].classic) {
                ++le_only;
            }
        }
    }
    /* one line of named fields: xbt reads powered=/ready=/scanning= out of
       it and ignores the rest, and the LE half is what makes "the scan
       found nothing" diagnosable from the device itself */
    bt_ret_append(ret, ret_sz,
        "state powered=%d ready=%d scanning=%d slice=%d devices=%d le_only=%d "
        "pending=%d le_supported=%d le_state=%d le_handle=0x%04X le_enc=%d "
        "hogp_boot=%d hogp_sub=%d hogp_mtu=%u\n",
        _powered ? 1 : 0,
        _ready ? 1 : 0,
        _scanning ? 1 : 0,
        (int)_scan_slice,
        count,
        le_only,
        (int)_pending.type,
        _le_supported ? 1 : 0,
        (int)_le.state,
        _le.handle,
        _le.encrypted ? 1 : 0,
        _hogp.boot_mode_ok ? 1 : 0,
        _hogp.n_subscribed,
        (unsigned)_hogp.mtu);
}

static void bt_help_emit(void) {
    bt_emit("open\n");
    bt_emit("close\n");
    bt_emit("scan [seconds]\n");
    bt_emit("stop\n");
    bt_emit("devices\n");
    bt_emit("known\n");
    bt_emit("forget <bdaddr>\n");
    bt_emit("unpair <bdaddr>\n");
    bt_emit("state\n");
    bt_emit("name <bdaddr>\n");
    bt_emit("connect <bdaddr>\n");
    bt_emit("pair <bdaddr> [pin]\n");
    bt_emit("disconnect <bdaddr|handle>\n");
    bt_emit("hid_open <bdaddr>\n");
    bt_emit("hid_close\n");
    bt_emit("hid_state\n");
}

static void bt_help_ret(char* ret, size_t ret_sz) {
    bt_ret_append(ret, ret_sz, "open: power on the bluetooth adapter\n");
    bt_ret_append(ret, ret_sz, "close: power off the bluetooth adapter\n");
    bt_ret_append(ret, ret_sz, "scan [seconds]: alternate LE and BR/EDR discovery until the time is up\n");
    bt_ret_append(ret, ret_sz, "stop\n");
    bt_ret_append(ret, ret_sz, "devices\n");
    bt_ret_append(ret, ret_sz, "known: list devices remembered in /etc/bt/bt.json\n");
    bt_ret_append(ret, ret_sz, "forget <bdaddr>: drop a remembered device\n");
    bt_ret_append(ret, ret_sz, "unpair <bdaddr>: forget the bond and drop its link first\n");
    bt_ret_append(ret, ret_sz, "state\n");
    bt_ret_append(ret, ret_sz, "name <bdaddr>\n");
    bt_ret_append(ret, ret_sz, "connect <bdaddr>: BR/EDR page, or LE connect + pair + HID-over-GATT for a BLE device\n");
    bt_ret_append(ret, ret_sz, "pair <bdaddr> [pin]\n");
    bt_ret_append(ret, ret_sz, "disconnect <bdaddr|handle>\n");
    bt_ret_append(ret, ret_sz, "hid_open <bdaddr>: open HID ctrl+intr channels, or bring up an LE HID device\n");
    bt_ret_append(ret, ret_sz, "hid_close: tear the HID session down\n");
    bt_ret_append(ret, ret_sz, "hid_state: HID session + subscriber summary\n");
}

static int bt_name_request(const uint8_t* addr, char* ret_text, size_t ret_text_sz) {
    bt_device_t* dev = bt_find_device(addr, false);
    char addr_str[24];
    int ret;

    bt_addr_to_str(addr, addr_str, sizeof(addr_str));
    if (dev == NULL) {
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "name_fail %s reason=unknown_device\n", addr_str);
        }
        return -1;
    }

    ret = bt_hci_request_remote_name(dev);
    if (ret != 0) {
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "name_fail %s status=%d\n", addr_str, ret);
        }
        return ret;
    }
    ret = bt_wait_for_opcode(HCI_OPCODE(HCI_OGF_LINK_CTRL, HCI_OCF_REMOTE_NAME_REQ), 1500);
    if (ret != 0) {
        if (ret_text != NULL && ret_text_sz != 0) {
            snprintf(ret_text, ret_text_sz, "name_fail %s status=%d\n", addr_str, ret);
        }
        return ret;
    }
    if (ret_text != NULL && ret_text_sz != 0) {
        snprintf(ret_text, ret_text_sz, "name_begin %s\n", addr_str);
    }
    return 0;
}

static int bt_disconnect_target(const char* arg, char* ret_text, size_t ret_text_sz) {
    uint8_t addr[6];
    bt_device_t* dev = NULL;
    uint16_t handle;
    char* endptr;
    unsigned long value;

    if (bt_parse_addr(arg, addr)) {
        dev = bt_find_device(addr, false);
        if (dev == NULL || !dev->connected) {
            if (ret_text != NULL && ret_text_sz != 0) {
                snprintf(ret_text, ret_text_sz, "disconnect_fail %s reason=not_connected\n", arg);
            }
            return -1;
        }
        handle = dev->handle;
    }
    else {
        value = strtoul(arg, &endptr, 0);
        if (*arg == 0 || *endptr != 0 || value > 0xffffUL) {
            if (ret_text != NULL && ret_text_sz != 0) {
                snprintf(ret_text, ret_text_sz, "disconnect_fail %s reason=bad_arg\n", arg);
            }
            return -1;
        }
        handle = (uint16_t)value;
    }

    /* fire-and-forget: the link teardown rides the Disconnection Complete
       event like any other drop; a rejection surfaces through the async
       command-status handler (_disc_pending_handle) */
    bt_hci_disconnect(handle);
    _disc_pending_handle = handle;
    if (ret_text != NULL && ret_text_sz != 0) {
        snprintf(ret_text, ret_text_sz, "disconnect_begin handle=0x%04X\n", handle);
    }
    return 0;
}

static void bt_disconnect_all(void) {
    int i;

    /* HCI_Disconnect is transport-agnostic, so the LE link goes down with
       the classic ones; an LE bring-up in flight is released by the
       disconnection-complete event it triggers */
    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (_devices[i].used && _devices[i].connected) {
            bt_hci_disconnect(_devices[i].handle);
        }
    }
    if (_le.handle_valid) {
        bt_hci_disconnect(_le.handle);
    }
}

static void bt_mark_all_disconnected(void) {
    int i;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (_devices[i].used) {
            _devices[i].connected = false;
            _devices[i].handle = 0;
        }
    }
    _le_autoconnect = false;
    _le_supported = false; /* re-probed by bt_le_controller_init on open */
    _scan_slice = BT_SCAN_SLICE_NONE;
    _scan_slice_end_ms = 0;
    _scan_total_end_ms = 0;
    bt_hid_stack_reset();
}

/* "open": bring the radio up. A cold open power-cycles BT_ON and reloads the
   firmware via bt_driver_init(); a warm open just re-enables scan so the
   adapter is discoverable + connectable again. Both reply at once: the cold
   path takes seconds, so it is deferred to bt_loop (_open_pending) and the
   known-device autoconnect always runs from bt_loop (_autoconnect_due_ms). */
static int bt_open_adapter(char* ret, size_t ret_sz) {
    uint8_t scan_enable = 0x03;

    /* the background bring-up in bt_loop owns the firmware download until it
       reports ready; an "open" arriving before that must not start a second
       bt_driver_init on top of it */
    if (_init_state != 1) {
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "open_busy reason=initializing\n");
        }
        return 0;
    }

    if (_ready && _powered) {
        (void)bt_hci_send_command(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_SCAN_ENABLE,
                &scan_enable, 1);
        bt_emit("power_on state=already_on\n");
        _autoconnect_due_ms = kernel_tic_ms(0) + BT_AUTOCONNECT_DEFER_MS;
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "open_ok state=already_on\n");
        }
        return 0;
    }

    /* cold open: the firmware reload is seconds of blocking HCI traffic, so
       it is armed here and executed from bt_loop; progress is observable
       through the state poll and the power_on/open_fail events */
    _open_pending = true;
    bt_emit("power_on state=powering_on\n");
    if (ret != NULL && ret_sz != 0) {
        snprintf(ret, ret_sz, "open_begin state=powering_on\n");
    }
    return 0;
}

/* "close": drop every link, go non-discoverable, then pull BT_ON low so the
   BT core powers down. WL_ON is left untouched so a co-resident wifi chip on
   the same combo module keeps running. */
static int bt_close_adapter(char* ret, size_t ret_sz) {
    uint8_t scan_enable = 0x00;

    /* a close cancels any deferred power-on work */
    _open_pending = false;
    _autoconnect_due_ms = 0;

    if (!_powered) {
        _ready = false;
        _scanning = false;
        _le_autoconnect = false;
        _scan_manual = false;
        bt_clear_pending();
        bt_emit("power_off state=already_off\n");
        if (ret != NULL && ret_sz != 0) {
            snprintf(ret, ret_sz, "close_ok state=already_off\n");
        }
        return 0;
    }

    if (_scanning) {
        (void)bt_stop_scan();
    }
    bt_disconnect_all();
    if (_ready) {
        (void)bt_hci_send_command(HCI_OGF_HOST_CTRL, HCI_OCF_WRITE_SCAN_ENABLE,
                &scan_enable, 1);
    }

    /* BT_ON low (the machine's bsp leaves the WiFi side of a combo chip
       alone) */
    bsp_bt_power_off();

    _powered = false;
    _ready = false;
    _scanning = false;
    _le_autoconnect = false;
    _scan_manual = false;
    bt_clear_pending();
    bt_mark_all_disconnected();
    bt_emit("power_off state=powered_off\n");
    if (ret != NULL && ret_sz != 0) {
        snprintf(ret, ret_sz, "close_ok state=powered_off\n");
    }
    return 0;
}

static int bt_handle_cmd_args(int argc, char** argv, char* ret, size_t ret_sz) {
    const char* cmd;
    const char* arg1;
    const char* arg2;
    uint8_t addr[6];

    if (ret_sz == 0) {
        return -1;
    }
    ret[0] = 0;

    if (argc <= 0 || argv == NULL || argv[0] == NULL) {
        snprintf(ret, ret_sz, "missing command\n");
        return -1;
    }

    cmd = argv[0];
    arg1 = argc > 1 ? argv[1] : NULL;
    arg2 = argc > 2 ? argv[2] : NULL;

    /* IPC can interrupt loop-driven initialization. In particular, "stop"
       must not issue an HCI command and overwrite the firmware command wait,
       and "forget" must not race the deferred store load. Status stays usable. */
    if (_init_state != 1 && strcmp(cmd, "help") != 0 &&
            strcmp(cmd, "state") != 0 && strcmp(cmd, "devices") != 0 &&
            strcmp(cmd, "known") != 0 && strcmp(cmd, "hid_state") != 0) {
        snprintf(ret, ret_sz, "%s_busy reason=initializing\n", cmd);
        return 0;
    }

    if (strcmp(cmd, "help") == 0) {
        bt_help_ret(ret, ret_sz);
    }
    else if (strcmp(cmd, "open") == 0 || strcmp(cmd, "on") == 0 ||
            strcmp(cmd, "poweron") == 0) {
        bt_open_adapter(ret, ret_sz);
    }
    else if (strcmp(cmd, "close") == 0 || strcmp(cmd, "off") == 0 ||
            strcmp(cmd, "poweroff") == 0) {
        bt_close_adapter(ret, ret_sz);
    }
    else if (strcmp(cmd, "state") == 0) {
        bt_dump_state_ret(ret, ret_sz);
    }
    else if (strcmp(cmd, "devices") == 0) {
        bt_list_devices_ret(ret, ret_sz);
    }
    else if (strcmp(cmd, "known") == 0) {
        bt_list_known_ret(ret, ret_sz);
    }
    else if (strcmp(cmd, "forget") == 0) {
        if (arg1 == NULL) {
            snprintf(ret, ret_sz, "forget_fail reason=missing_target\n");
            return 0;
        }
        bt_forget_device(arg1, ret, ret_sz);
    }
    else if (strcmp(cmd, "unpair") == 0) {
        if (arg1 == NULL) {
            snprintf(ret, ret_sz, "unpair_fail reason=missing_target\n");
            return 0;
        }
        bt_unpair_device(arg1, ret, ret_sz);
    }
    else if (strcmp(cmd, "scan") == 0) {
        int seconds = arg1 != NULL ? atoi(arg1) : 10;
        if (!_ready) {
            snprintf(ret, ret_sz, "scan_fail reason=not_ready\n");
            return 0;
        }
        /* A manual scan is a foreground operation and must not be locked out by
           any background session that keeps _scanning set: neither the LE
           autoconnect/discovery scan (which would make every manual scan return
           scan_busy while a pad sits in pairing mode) nor a leftover "zombie"
           session - e.g. the autoconnect scan that just brought a mouse up,
           whose _le_autoconnect latch was cleared on connect, leaving
           _scanning set while the radio-contention guard suspends its slice.
           Keying the preempt on !_scan_manual (not _le_autoconnect) catches
           both, since only a user scan latches _scan_manual; a scan the user
           already started is never preempted, it just reports scan_busy. Close
           the discovery window with its cooldown so the retry step does not
           immediately re-arm over the user's scan. */
        if (_scanning && !_scan_manual) {
            bt_le_discovery_close();
            bt_stop_scan();
        }
        if (_scanning) {
            snprintf(ret, ret_sz, "scan_busy\n");
            return 0;
        }
        int scan_ret = bt_start_scan(seconds);
        if (scan_ret != 0) {
            snprintf(ret, ret_sz, "scan_fail status=%d\n", scan_ret);
            return 0;
        }
        snprintf(ret, ret_sz, "scan_begin seconds=%d\n", seconds > 0 ? seconds : 10);
    }
    else if (strcmp(cmd, "stop") == 0) {
        int stop_ret = bt_stop_scan();
        snprintf(ret, ret_sz, "scan_stop status=%d\n", stop_ret);
    }
    else if (strcmp(cmd, "name") == 0) {
        if (!_ready) {
            snprintf(ret, ret_sz, "name_fail reason=not_ready\n");
            return 0;
        }
        if (arg1 == NULL || !bt_parse_addr(arg1, addr)) {
            snprintf(ret, ret_sz, "name_fail reason=bad_addr\n");
            return 0;
        }
        bt_name_request(addr, ret, ret_sz);
    }
    else if (strcmp(cmd, "connect") == 0) {
        if (!_ready) {
            snprintf(ret, ret_sz, "connect_fail reason=not_ready\n");
            return 0;
        }
        if (arg1 == NULL || !bt_parse_addr(arg1, addr)) {
            snprintf(ret, ret_sz, "connect_fail reason=bad_addr\n");
            return 0;
        }
        bt_start_connection(addr, false, NULL, ret, ret_sz);
    }
    else if (strcmp(cmd, "pair") == 0) {
        if (!_ready) {
            snprintf(ret, ret_sz, "pair_fail reason=not_ready\n");
            return 0;
        }
        if (arg1 == NULL || !bt_parse_addr(arg1, addr)) {
            snprintf(ret, ret_sz, "pair_fail reason=bad_addr\n");
            return 0;
        }
        bt_start_connection(addr, true, arg2, ret, ret_sz);
    }
    else if (strcmp(cmd, "disconnect") == 0) {
        if (!_ready) {
            snprintf(ret, ret_sz, "disconnect_fail reason=not_ready\n");
            return 0;
        }
        if (arg1 == NULL) {
            snprintf(ret, ret_sz, "disconnect_fail reason=missing_target\n");
            return 0;
        }
        bt_disconnect_target(arg1, ret, ret_sz);
    }
    else if (strcmp(cmd, "hid_open") == 0) {
        bt_device_t* dev;

        if (!_ready) {
            snprintf(ret, ret_sz, "hid_open_fail reason=not_ready\n");
            return 0;
        }
        if (arg1 == NULL || !bt_parse_addr(arg1, addr)) {
            snprintf(ret, ret_sz, "hid_open_fail reason=bad_addr\n");
            return 0;
        }
        dev = bt_find_device(addr, false);
        if (dev == NULL) {
            snprintf(ret, ret_sz, "hid_open_fail reason=not_connected\n");
            return 0;
        }
        /* an LE HID device has no ctrl/intr channels to open: the link is
           either brought up and already reporting, or it still has to be
           connected, which bt_le_request queues for bt_le_step.
           A dual-mode peripheral that is ALREADY streaming reports through
           HOGP must not be given classic channels too: the radio scheduling
           conflict between the L2CAP retries and the LE connection interval
           kills the LE link (the keyboard drops off after a few seconds).
           The same holds while its LE bring-up is still mid-flight: the
           attach pollers fire hid_open every 2s and hid_state reads active=0
           until the session reaches READY, so without the handle test below a
           dual-mode pad in DISCOVERING would fall through to bt_hid_start on
           the LE handle - aliasing it into a session's acl_handle and letting the
           end-of-connect duplicate-transport suppression disconnect the very
           link it just brought up (the pad drops at READY and reconnect-loops).
           Any device whose current handle owns an LE session belongs to HOGP. */
        if (dev->le && (!dev->classic ||
                le_session_ready_by_addr(dev->addr) >= 0 ||
                le_session_by_handle(dev->handle) >= 0)) {
            int rsi = le_session_ready_by_addr(dev->addr);
            if (rsi >= 0) {
                snprintf(ret, ret_sz, "hid_open_ok %s le=1 handle=0x%04X\n",
                        arg1, _les[rsi].le.handle);
                return 0;
            }
            if (bt_le_request(dev, true, NULL, 0) != 0) {
                snprintf(ret, ret_sz, "hid_open_fail %s reason=le_busy\n", arg1);
            }
            else {
                snprintf(ret, ret_sz, "hid_open_begin %s le=1\n", arg1);
            }
            return 0;
        }
        if (!dev->connected || dev->handle == 0) {
            snprintf(ret, ret_sz, "hid_open_fail reason=not_connected\n");
            return 0;
        }
        /* If HOGP is already streaming reports from a BLE keyboard, opening
           classic HIDP to a DIFFERENT keyboard causes radio contention that
           kills the LE link. A classic MOUSE is fine (separate ACL, no
           conflict). Same-device check also applies. */
        if (bt_hogp_blocks_classic(dev)) {
            int s;
            uint16_t hh = 0;
            int rr = 0;
            for (s = 0; s < MAX_LE_SESSIONS; ++s) {
                if (_les[s].le.state == LE_ST_READY &&
                        _les[s].hogp.n_subscribed > 0) {
                    hh = _les[s].le.handle;
                    rr = _les[s].hogp.n_subscribed;
                    break;
                }
            }
            snprintf(ret, ret_sz,
                "hid_open_ok %s hogp_active handle=0x%04X reports=%d\n",
                arg1, hh, rr);
            return 0;
        }
        bt_hid_start(dev->handle, dev->addr);
        snprintf(ret, ret_sz, "hid_open_begin handle=0x%04X\n", dev->handle);
    }
    else if (strcmp(cmd, "hid_close") == 0) {
        int i;
        int closed = 0;

        /* no address argument: every live classic session is closed */
        for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
            if (_hids[i].active) {
                bt_hid_stop(&_hids[i]);
                ++closed;
            }
        }
        if (closed == 0) {
            snprintf(ret, ret_sz, "hid_close_fail reason=no_session\n");
            return 0;
        }
        snprintf(ret, ret_sz, "hid_close_ok count=%d\n", closed);
    }
    else if (strcmp(cmd, "hid_state") == 0) {
        char haddr[24];
        int sub_cnt = hid_srv_count(HID_REPORT_ID_MOUSE);
        int i;
        size_t off = 0;

        /* bt_moused polls this string and only attaches while active=0, so
           it has to say active=1 for an LE session too. One line per live
           classic session, in slot order; the pollers strstr for "active=1",
           so multiple lines and the trailing slot key are safe. */
        for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
            int n;

            if (!_hids[i].active) {
                continue;
            }
            bt_addr_to_str(_hids[i].addr, haddr, sizeof(haddr));
            n = snprintf(ret + off, off < ret_sz ? ret_sz - off : 0,
                "hid_state active=1 addr=%s handle=0x%04X up=%d boot=%d "
                "ctrl=%d intr=%d le=0 mouse_subs=%d slot=%d\n",
                haddr, _hids[i].acl_handle, _hids[i].up ? 1 : 0,
                _hids[i].boot_protocol_ok ? 1 : 0,
                _hids[i].ctrl != NULL ? _hids[i].ctrl->state : -1,
                _hids[i].intr != NULL ? _hids[i].intr->state : -1,
                sub_cnt, i);
            if (n > 0) {
                off += (size_t)n;
            }
        }
        if (off != 0) {
            return 0;
        }
        /* with several LE links live, report the first READY session; the
           mouse and keyboard daemons only test for active=1 and pick their
           device out of the device list, so naming one session is enough */
        {
            int s;
            for (s = 0; s < MAX_LE_SESSIONS; ++s) {
                if (_les[s].le.state == LE_ST_READY && _les[s].le.handle_valid) {
                    bt_addr_to_str(_les[s].le.addr, haddr, sizeof(haddr));
                    snprintf(ret, ret_sz,
                        "hid_state active=1 addr=%s handle=0x%04X up=1 boot=%d "
                        "le=1 reports=%d mouse_subs=%d slot=%d\n",
                        haddr, _les[s].le.handle,
                        _les[s].hogp.boot_mode_ok ? 1 : 0,
                        _les[s].hogp.n_subscribed, sub_cnt, s);
                    return 0;
                }
            }
        }
        snprintf(ret, ret_sz, "hid_state active=0 le_supported=%d le_state=%d "
                "mouse_subs=%d\n", _le_supported ? 1 : 0, (int)_le.state,
                sub_cnt);
    }
    else {
        snprintf(ret, ret_sz, "unknown command\n");
        return 0;
    }
    return 0;
}

static int bt_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    int i;

    if (size <= 0) {
        return VFS_ERR_RETRY;
    }

    /* a subscriber that picked a report id reads fixed events from its
       own queue (libhid's wire protocol); report_id 0 keeps the legacy
       text event stream so xbt/devcmd readers are unaffected */
    if (hid_srv_report_id(fd, from_pid) != 0) {
        return hid_vdev_read(dev, fd, from_pid, node, buf, size, offset, p);
    }

    for (i = 0; i < size; ++i) {
        if (charbuf_pop(_evt_buf, ((char*)buf) + i) != 0) {
            break;
        }
    }
    return i == 0 ? VFS_ERR_RETRY : i;
}

static char* bt_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    char* ret = (char*)malloc(BT_CMD_RET_SZ);
    if (ret == NULL) {
        return NULL;
    }
    memset(ret, 0, BT_CMD_RET_SZ);

    (void)dev;
    (void)from_pid;
    (void)p;

    if (bt_handle_cmd_args(argc, argv, ret, BT_CMD_RET_SZ) != 0) {
        return ret;
    }
    if (ret[0] == 0) {
        snprintf(ret, BT_CMD_RET_SZ, "ok");
    }
    return ret;
}

static uint32_t bt_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, void* p) {
    if (hid_srv_report_id(fd, from_pid) != 0) {
        return hid_vdev_check_poll_events(dev, fd, from_pid, node, p);
    }
    if (_evt_buf != NULL && !charbuf_is_empty(_evt_buf)) {
        return VFS_EVT_RD;
    }
    return 0;
}

/* A live input link (classic HID with both channels up, or a READY LE/HOGP
   session) means reports can arrive at any moment, so bt_loop must stay
   tightly polled to keep cursor latency at the floor. When nothing is
   connected the daemon is only scanning, and polling can relax. */
bool bt_hid_live(void) {
    int i;
    for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
        if (_hids[i].active && _hids[i].up) {
            return true;
        }
    }
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_READY && _les[i].le.handle_valid) {
            return true;
        }
    }
    return false;
}

/* Keep SD-card access and controller waits out of mounted. Failed controller
   attempts remain bounded and are retried without exiting the daemon. */
static void bt_bringup_step(void) {
    uint64_t start_ms;

    if (_init_state == 1) {
        return;
    }
    if (_init_state == 2 && (int64_t)(kernel_tic_ms(0) - _init_retry_ms) < 0) {
        return;
    }

    if (!_known_loaded) {
        /* A missing store is created on the first actual change, not during
           initialization. Retries must not reload stale bonds. */
        bt_known_load();
        if (bt_known_dedup_le() > 0) {
            bt_known_save();
        }
        bt_known_seed_devices();
        _known_loaded = true;
    }

    start_ms = kernel_tic_ms(0);
    if (bt_driver_init() == 0) {
        _powered = true;
        _init_state = 1;
        bt_help_emit();
        bt_autoconnect_known();
    }
    else {
        _init_state = 2;
        _init_retry_ms = kernel_tic_ms(0) + 3000;
        slog("bluetooth init retry_scheduled elapsed_ms=%u\n",
            (uint32_t)(kernel_tic_ms(0) - start_ms));
    }
}

/* Periodically refresh the live RSSI of connected links. Advertisements only
   populate rssi while scanning; once connected - especially an auto-connected
   device restored from the bond store, which never advertised this session -
   the field would otherwise stay at its "unknown" placeholder and xbt shows
   no signal bar. Read RSSI is cheap; one handle is polled per interval. */
#define BT_RSSI_POLL_MS 2000
static uint32_t _rssi_poll_ms = 0;
static int _rssi_rr = 0;

static void bt_rssi_poll_step(void) {
    uint32_t now = kernel_tic_ms(0);
    int i;

    if (_rssi_poll_ms != 0 && (int32_t)(now - _rssi_poll_ms) < 0) {
        return;
    }
    _rssi_poll_ms = now + BT_RSSI_POLL_MS;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        int idx = (_rssi_rr + i) % MAX_BT_DEVICES;
        if (_devices[idx].used && _devices[idx].connected &&
                _devices[idx].handle != 0) {
            uint8_t params[2];
            _rssi_rr = (idx + 1) % MAX_BT_DEVICES;
            params[0] = (uint8_t)(_devices[idx].handle & 0xff);
            params[1] = (uint8_t)(_devices[idx].handle >> 8);
            bt_hci_send_command(HCI_OGF_STATUS, HCI_OCF_READ_RSSI,
                params, sizeof(params));
            return; /* one handle per interval */
        }
    }
}

int bt_loop(vdevice_t* dev, void* p) {
    int packets = 0;

    (void)p;

    hid_set_node(dev->mnt_info.node);

    /* must run before the first bt_poll_once: until bt_driver_init maps the
       MMIO window the UART registers are not readable */
    bt_bringup_step();

    /* deferred "open" from the devcmd handler: the firmware reload blocks
       for seconds, so it runs here (exactly like the boot bring-up) and the
       command reply already went out as open_begin */
    if (_open_pending && _init_state == 1 && !_ready) {
        int r;
        _open_pending = false;
        r = bt_driver_init();
        if (r == 0) {
            _powered = true;
            bt_emit("power_on state=powered_on\n");
            _autoconnect_due_ms = kernel_tic_ms(0) + BT_AUTOCONNECT_DEFER_MS;
        }
        else {
            slog("bluetooth deferred open_failed ret=%d\n", r);
            bt_emit("open_fail status=%d\n", r);
        }
    }

    while (bt_poll_once(0) > 0) {
        ++packets;
        l2cap_step();
        if (packets >= 64) {
            break; /* continuous input must not starve timers and IPC */
        }
    }

    /* L2CAP channel state machine (connect/config retries, timeouts,
       SET_PROTOCOL kick once both HID channels are open) */
    l2cap_step();

    /* LE discovery slicing, queued LE bring-ups and the LE link's own
       deadlines all live here instead of in the command handlers, which
       have to stay short enough for a click in xbt to feel instant. This is
       the daemon's main context, so a queued bring-up's blocking waits here
       can still be preempted by an incoming dev.cmd IPC (from_loop=true). */
    bt_le_step(true);

    /* keep the live RSSI of connected links fresh for xbt's signal readout */
    bt_rssi_poll_step();

    /* a bonded mouse asleep at power-on still auto-connects once it wakes */
    bt_le_autoconnect_retry_step();

    /* deferred known-device autoconnect from "open": paging each known
       device is bounded but not instant, so it never runs inside the devcmd
       reply. The deadline drops the request when the radio stayed busy
       (the user started a scan/connect of their own right after power-on). */
    if (_autoconnect_due_ms != 0) {
        if ((int64_t)(kernel_tic_ms(0) - _autoconnect_due_ms) >= 0) {
            _autoconnect_due_ms = 0;
        }
        else if (_ready && _powered && !_scanning &&
                _pending.type == BT_PENDING_NONE && !_le_req_active) {
            _autoconnect_due_ms = 0;
            bt_autoconnect_known();
        }
    }

    /* a mouse subscriber's edge wake can get spent on a generic IPC wait:
       re-assert while queues still hold undrained events */
    if (_sub_reassert_ms != 0 && (int64_t)(kernel_tic_ms(0) - _sub_reassert_ms) >= 0) {
        if (hid_backlog()) {
            hid_rewake_backlog();
            _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
        }
        else {
            _sub_reassert_ms = 0;
        }
    }

    if (packets == 0) {
        /* bound the backoff by whether an input link is live: a connected
           mouse/keyboard must not wait more than BT_IDLE_SLEEP_HID_US for
           the next poll, while an idle (scanning-only) daemon may relax to
           the full ceiling to save CPU. */
        uint32_t cap = bt_hid_live() ? BT_IDLE_SLEEP_HID_US : BT_IDLE_SLEEP_MAX_US;
        usleep(_idle_sleep_us);
        if (_idle_sleep_us < cap) {
            _idle_sleep_us <<= 1;
            if (_idle_sleep_us > cap) {
                _idle_sleep_us = cap;
            }
        }
    }
    else {
        _idle_sleep_us = BT_IDLE_SLEEP_MIN_US;
    }
    return 0;
}

static int bt_mounted(vdevice_t* dev, ewokos_addr_t node, void* p) {
    (void)dev;
    (void)node;
    (void)p;

    /* No filesystem I/O, MMIO, sleeps, or HCI transactions in mounted. */
    _init_state = 0;
    _init_retry_ms = 0;
    _known_loaded = false;
    return 0;
}

int main(int argc, char** argv) {
    vdevice_t dev;
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/bt0";

    _evt_buf = charbuf_new(0);
    if (_evt_buf == NULL) {
        return -1;
    }

    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "bluetooth");
    dev.mounted = bt_mounted;
    dev.open = bt_vdev_open;
    dev.close = bt_vdev_close;
    dev.fcntl = bt_vdev_fcntl;
    dev.read = bt_read;
    dev.loop_step = bt_loop;
    dev.check_poll_events = bt_check_poll_events;
    dev.cmd = bt_dev_cmd;
    _bt_dev = &dev;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666, false);

    charbuf_free(_evt_buf);
    return 0;
}
