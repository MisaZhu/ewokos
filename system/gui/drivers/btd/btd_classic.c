/* btd_classic.c - classic BR/EDR HCI command wrappers and
                link / pairing event handlers.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

static int bt_hci_accept_connection(const uint8_t* addr) {
    uint8_t params[7];

    memcpy(params, addr, 6);
    /* role 0x01: remain slave - the reconnecting mouse initiated the
       link, and a role switch is just extra LMP round trips for a
       battery peripheral */
    params[6] = 0x01;
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_ACCEPT_CONN_REQ, params, sizeof(params));
}

int bt_hci_auth_request(uint16_t handle) {
    uint8_t params[2];

    params[0] = (uint8_t)(handle & 0xff);
    params[1] = (uint8_t)(handle >> 8);
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_AUTH_REQ, params, sizeof(params));
}

static int bt_hci_set_conn_encrypt(uint16_t handle) {
    uint8_t params[3];

    params[0] = (uint8_t)(handle & 0xff);
    params[1] = (uint8_t)(handle >> 8);
    params[2] = 0x01; /* enable link encryption */
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_SET_CONN_ENCRYPT, params, sizeof(params));
}

int bt_hci_disconnect(uint16_t handle) {
    uint8_t params[3];

    params[0] = (uint8_t)(handle & 0xff);
    params[1] = (uint8_t)(handle >> 8);
    params[2] = 0x13;
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_DISCONNECT, params, sizeof(params));
}

int bt_hci_create_connection(const bt_device_t* dev) {
    uint8_t params[13];
    uint16_t packet_type = 0xcc18;
    uint16_t clock_offset = dev != NULL ? dev->clock_offset : 0;
    uint8_t page_scan_rep_mode = dev != NULL ? dev->page_scan_rep_mode : 0x01;

    memset(params, 0, sizeof(params));
    memcpy(params, dev->addr, 6);
    params[6] = (uint8_t)(packet_type & 0xff);
    params[7] = (uint8_t)(packet_type >> 8);
    params[8] = page_scan_rep_mode;
    params[9] = 0;
    if (clock_offset != 0) {
        clock_offset |= 0x8000;
    }
    params[10] = (uint8_t)(clock_offset & 0xff);
    params[11] = (uint8_t)(clock_offset >> 8);
    params[12] = 1;
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_CREATE_CONN, params, sizeof(params));
}

int bt_hci_request_remote_name(const bt_device_t* dev) {
    uint8_t params[10];

    memset(params, 0, sizeof(params));
    memcpy(params, dev->addr, 6);
    params[6] = dev->page_scan_rep_mode;
    params[7] = 0;
    params[8] = (uint8_t)(dev->clock_offset & 0xff);
    params[9] = (uint8_t)(dev->clock_offset >> 8);
    return bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_REMOTE_NAME_REQ, params, sizeof(params));
}

static void bt_handle_inquiry_result_common(
    const uint8_t* addr,
    uint8_t page_scan_rep_mode,
    uint32_t class_of_device,
    uint16_t clock_offset,
    int8_t rssi,
    const char* name) {
    bt_device_t* dev = bt_find_device(addr, true);

    if (dev == NULL) {
        return;
    }

    /* everything that arrives through an inquiry result is by definition
       reachable over BR/EDR, which is what keeps a dual-mode device from
       being sent down the LE bring-up path */
    dev->classic = true;
    dev->page_scan_rep_mode = page_scan_rep_mode;
    dev->class_of_device = class_of_device;
    dev->clock_offset = clock_offset;
    dev->rssi = rssi;
    /* an inquiry response is a live sighting: stamp it so the live-devices
       freshness test treats this peer as on the air, exactly like an LE
       advertising report does */
    dev->last_seen_ms = kernel_tic_ms(0);
    if (name != NULL && name[0] != 0) {
        strncpy(dev->name, name, sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = 0;
        bt_trim_name(dev->name);
    }
    bt_emit_device_line("device", dev);
}

void bt_handle_inquiry_result(const uint8_t* payload, size_t len) {
    uint8_t num;
    size_t i;
    size_t off_addr;
    size_t off_psrm;
    size_t off_cod;
    size_t off_clk;

    if (len < 1) {
        return;
    }

    num = payload[0];
    off_addr = 1;
    off_psrm = off_addr + (size_t)num * 6;
    off_cod = off_psrm + (size_t)num * 2;
    off_clk = off_cod + (size_t)num * 3;

    if (len < off_clk + (size_t)num * 2) {
        return;
    }

    for (i = 0; i < num; ++i) {
        const uint8_t* addr = payload + off_addr + i * 6;
        uint8_t psrm = payload[off_psrm + i];
        uint32_t cod = (uint32_t)payload[off_cod + i * 3] |
            ((uint32_t)payload[off_cod + i * 3 + 1] << 8) |
            ((uint32_t)payload[off_cod + i * 3 + 2] << 16);
        uint16_t clk = (uint16_t)payload[off_clk + i * 2] |
            ((uint16_t)payload[off_clk + i * 2 + 1] << 8);

        bt_handle_inquiry_result_common(addr, psrm, cod, clk, 127, NULL);
    }
}

void bt_handle_inquiry_result_rssi(const uint8_t* payload, size_t len) {
    uint8_t num;
    size_t i;
    size_t off_addr;
    size_t off_psrm;
    size_t off_cod;
    size_t off_clk;
    size_t off_rssi;

    if (len < 1) {
        return;
    }

    num = payload[0];
    off_addr = 1;
    off_psrm = off_addr + (size_t)num * 6;
    off_cod = off_psrm + (size_t)num * 2;
    off_clk = off_cod + (size_t)num * 3;
    off_rssi = off_clk + (size_t)num * 2;

    if (len < off_rssi + num) {
        return;
    }

    for (i = 0; i < num; ++i) {
        const uint8_t* addr = payload + off_addr + i * 6;
        uint8_t psrm = payload[off_psrm + i];
        uint32_t cod = (uint32_t)payload[off_cod + i * 3] |
            ((uint32_t)payload[off_cod + i * 3 + 1] << 8) |
            ((uint32_t)payload[off_cod + i * 3 + 2] << 16);
        uint16_t clk = (uint16_t)payload[off_clk + i * 2] |
            ((uint16_t)payload[off_clk + i * 2 + 1] << 8);
        int8_t rssi = (int8_t)payload[off_rssi + i];

        bt_handle_inquiry_result_common(addr, psrm, cod, clk, rssi, NULL);
    }
}

void bt_handle_extended_inquiry_result(const uint8_t* payload, size_t len) {
    char name[64];
    uint32_t cod;
    uint16_t clk;

    if (len < 255 || payload[0] == 0) {
        return;
    }

    cod = (uint32_t)payload[9] |
        ((uint32_t)payload[10] << 8) |
        ((uint32_t)payload[11] << 16);
    clk = (uint16_t)payload[12] | ((uint16_t)payload[13] << 8);
    bt_parse_eir_name(payload + 15, len - 15, name, sizeof(name));
    bt_handle_inquiry_result_common(payload + 1, payload[7], cod, clk, (int8_t)payload[14], name);
}

void bt_handle_remote_name_complete(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    char addr[24];
    size_t i;

    if (len < 7) {
        return;
    }

    dev = bt_find_device(payload + 1, true);
    if (dev == NULL) {
        return;
    }

    memset(dev->name, 0, sizeof(dev->name));
    for (i = 0; i < sizeof(dev->name) - 1 && (i + 7) < len; ++i) {
        unsigned char ch = payload[7 + i];
        if (ch == 0) {
            break;
        }
        dev->name[i] = isprint(ch) ? (char)ch : '.';
    }
    bt_trim_name(dev->name);
    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    bt_emit("name %s status=%u value=%s\n", addr, payload[0], dev->name[0] ? dev->name : "-");
    /* A name request that came back empty (failed lookup on a rotating
       secondary BDADDR) created a bare slot at bt_find_device above and
       wrote nothing into it. Release it if it still carries no identity, so
       it cannot linger as a nameless phantom `devices` row. A resolved name
       or any pre-existing identity (class/key/connected) keeps the slot. */
    bt_device_drop_if_identity_less(dev);
}

/* CoD major class 0x05 (Peripheral) with the keyboard/pointing minor bits:
   a mouse, keyboard or combo. Used to decide the classic security-then-HID
   flow. */
static bool bt_cod_is_hid_peripheral(uint32_t cod) {
    return ((cod >> 8) & 0x1f) == 0x05 && (cod & 0xc0) != 0;
}

/* CoD major class 0x05 (Peripheral) with the joystick (minor bit 2) or
   gamepad (minor bit 3) marker. Real values: joystick 0x002504, gamepad
   0x002508 (what a DualShock 4 / DualSense reports). These carry HID over
   the same control+interrupt L2CAP channels as a keyboard/mouse and also
   need the link secured before reports flow, so they are admitted into the
   same security-then-HID path. */
bool bt_cod_is_gamepad(uint32_t cod) {
    return ((cod >> 8) & 0x1f) == 0x05 && (cod & 0x0c) != 0;
}

/* Name matching identifies a controller family, not its supported transport.
   Third-party controllers may advertise the same name as an Xbox controller. */
bool bt_dev_is_xbox_gamepad(const bt_device_t* dev) {
    if (dev == NULL || dev->name[0] == 0) {
        return false;
    }
    return strstr(dev->name, "Xbox") != NULL ||
           strstr(dev->name, "X-Box") != NULL ||
           strstr(dev->name, "XB") != NULL;
}

/* A name hint can enable HID probing when CoD is missing. It cannot establish
   LE support or identify the report descriptor served by the current mode. */
bool bt_dev_is_named_gamepad(const bt_device_t* dev) {
    if (dev == NULL || dev->name[0] == 0) {
        return false;
    }
    if (bt_dev_is_xbox_gamepad(dev)) {
        return true;
    }
    return strstr(dev->name, "GameSir") != NULL;
}

/* When HOGP is already streaming, the only case where classic HIDP must be
   suppressed is the SAME physical peripheral already served over LE (opening
   classic channels to it as well is redundant and confuses it). A DIFFERENT
   classic peripheral is a normal, supported combination on this controller -
   the common desktop case is a BLE mouse on HOGP plus a classic keyboard on
   HIDP, and blocking the latter (as an earlier revision did) left the
   keyboard permanently unconnected. CoD is not consulted: a classic keyboard
   alongside an LE mouse must be allowed. */
bool bt_hogp_blocks_classic(bt_device_t* dev) {
    int i;
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state != LE_ST_READY || _les[i].hogp.n_subscribed == 0) {
            continue; /* this slot is not streaming HOGP */
        }
        if (dev->le || bt_addr_equal(dev->addr, _les[i].le.addr)) {
            return true; /* same device already on HOGP */
        }
    }
    return false; /* different device: classic HID is fine */
}

/* Rate-limit the migration slog/emit: an LE-only pad holding a stale classic
   bond re-pages BR/EDR in a tight loop (several reconnects per second while
   cycling its BDADDR), and logging every one buries /dev/log without adding
   information. */
static uint64_t _le_migrate_log_ms = 0;

/* Retain LE preference for a named controller only when this address has an
   LE bond or advertised gamepad appearance. A classic name/CoD or a silent
   HID session alone must never manufacture an LE target. */
static bool bt_le_only_classic_migrate(bt_device_t* dev, uint16_t handle,
        const char* addr) {
    uint64_t now = kernel_tic_ms(0);
    bool le_proven = dev->has_ltk || bt_appearance_is_gamepad(dev->appearance);

    /* Without LE evidence, keep the established classic transport and probe HID. */
    if (!le_proven) {
        dev->le = false;
        dev->classic = true;
        return false;
    }

    bt_known_prefer_le_by_name(dev->name);
    dev->le = true;
    dev->classic = false;
    dev->hid_after_sec = false;

    /* Abandon any classic connect/pair bookkeeping for this pad. On the first
       conn-complete the name is not resolved yet, so bt_dev_is_named_gamepad
       is false and the CoD-gamepad path latches _pending=BT_PENDING_PAIR and
       requests a classic auth the pad never answers. Once the name is known we
       migrate here instead, but the disconnect-complete only clears _pending on
       an exact handle match, which the pad's alternating handles (0x000b/0x000c)
       defeat - leaving BT_PENDING_PAIR stuck forever. Both bt_autoconnect_known
       and bt_le_autoconnect_retry_step bail while _pending.type != NONE, so the
       LE scan is never armed and the pad is never bonded even while it adverts.
       Migration abandons the classic path, so drop the pending with it. */
    if (_pending.type != BT_PENDING_NONE) {
        bt_clear_pending();
    }

    if ((uint32_t)(now - _le_migrate_log_ms) >= 2000) {
        _le_migrate_log_ms = now;
        slog("bluetooth le_only_classic_to_le %s h=0x%04x\n", addr, handle);
        bt_emit("le_migrate %s\n", addr);
    }
    /* Stop auto-accepting the pad's classic re-pages BEFORE dropping this ACL,
       otherwise the controller immediately accepts the next page and the pad
       never stays on the LE air long enough for LE_Create_Connection to win. */
    bt_xbox_page_scan_suspend();
    bt_hci_disconnect(handle);
    /* Open the bounded LE discovery window, not just an autoconnect kick. This
       pad is presence-proved (it just paged classic) but a first-time GameSir
       has no `le && !has_key` bond-store record, which is the ONLY thing
       bt_le_autoconnect_retry_step arms a scan for - so without this window its
       real LE identity (a DIFFERENT BDADDR, appearance 964) is never scanned
       for, the bring-up never starts, and page scan restores on the deadline
       only for the pad to re-page classic in an endless migrate/disconnect
       loop. The window is independent of store contents and self-refreshes
       while the pad keeps re-paging. */
    bt_le_discovery_kick();
    return true;
}

void bt_handle_connection_complete(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    uint8_t status;
    uint16_t handle;
    char addr[24];

    if (len < 11) {
        return;
    }

    status = payload[0];
    handle = (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
    dev = bt_find_device(payload + 3, true);
    if (dev == NULL) {
        return;
    }

    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    if (status == 0) {
        dev->connected = true;
        dev->handle = handle;
        /* the peer just came up on our radio: stamp it seen so it does not
           fall out of the live-devices list the instant it later disconnects */
        dev->last_seen_ms = kernel_tic_ms(0);
        /* Migration requires this address's LE evidence, not just its name. */
        if (bt_dev_is_named_gamepad(dev) &&
                bt_le_only_classic_migrate(dev, handle, addr)) {
            return;
        }
        bt_known_touch_from_device(dev);
        bt_emit("connect_ok %s handle=0x%04X\n", addr, handle);
        /* A bond-restored controller may have no inquiry CoD. Use the name as
           a gamepad hint so security/HID probing still runs in report protocol.
           This synthesized CoD is not evidence of a successful HID handshake. */
        if (bt_dev_is_named_gamepad(dev) && dev->class_of_device == 0) {
            dev->class_of_device = 0x002508;
        }
        if ((bt_cod_is_hid_peripheral(dev->class_of_device) ||
                bt_cod_is_gamepad(dev->class_of_device)) &&
                !bt_hogp_blocks_classic(dev)) {
            /* A classic HID peripheral (mouse/keyboard/gamepad) will not send
               input reports over a plain link: secure it first. Authenticate now,
               request encryption from the auth-complete handler, and only bring
               the L2CAP HID channels up from the encryption-change event.
               Opening them here - on the still-unauthenticated link - is what
               made a connected keyboard produce no keystrokes. Force the
               pending target to PAIR so SSP Just Works is auto-accepted even
               when the user issued a plain "connect".
               Skip entirely when HOGP is already streaming: the peripheral
               refused classic L2CAP in that state and the radio contention
               kills the LE link. */
            dev->hid_after_sec = true;
            if (_pending.type != BT_PENDING_PAIR ||
                    !bt_addr_equal(_pending.addr, dev->addr)) {
                memset(&_pending, 0, sizeof(_pending));
                _pending.type = BT_PENDING_PAIR;
                memcpy(_pending.addr, dev->addr, 6);
                strncpy(_pending.pin, "0000", sizeof(_pending.pin) - 1);
            }
            _pending.handle = handle;
            bt_hci_auth_request(handle);
            bt_emit("pair_wait_auth %s\n", addr);
        }
        else if (_pending.type == BT_PENDING_PAIR && bt_addr_equal(_pending.addr, dev->addr)) {
            _pending.handle = handle;
            bt_hci_auth_request(handle);
            bt_emit("pair_wait_auth %s\n", addr);
        }
        else if (_pending.type == BT_PENDING_CONNECT && bt_addr_equal(_pending.addr, dev->addr)) {
            bt_clear_pending();
        }
    }
    else {
        bt_emit("connect_fail %s status=%u\n", addr, status);
        if (bt_pending_matches_addr(dev->addr)) {
            bt_clear_pending();
        }
        /* A failed connection still created a bare slot at bt_find_device
           above; if it carries no identity (never named, no class, no key)
           release it so a failed page to a rotating BDADDR cannot leave a
           phantom `devices` row behind. A known device we merely failed to
           reconnect keeps its identity and is untouched. */
        bt_device_drop_if_identity_less(dev);
    }
}

void bt_handle_disconnection_complete(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    uint16_t handle;
    char addr[24];

    if (len < 4 || payload[0] != 0) {
        return; /* a failed disconnect leaves the link intact */
    }

    handle = ((uint16_t)payload[1] | ((uint16_t)payload[2] << 8)) & 0x0fff;
    dev = bt_find_device_by_handle(handle);
    if (dev != NULL) {
        dev->connected = false;
        dev->handle = 0;
        dev->hid_after_sec = false;
        bt_addr_to_str(dev->addr, addr, sizeof(addr));
        bt_emit("disconnect %s status=%u reason=%u\n", addr, payload[0], payload[3]);
        /* A disconnect reason is not evidence of LE support. Keep transport
           identity and bond material unchanged, even if HID never sent input.
           Otherwise an ordinary remote disconnect rewrites a classic bond as
           an unverified LE target and prevents a normal bonded reconnect. */
        /* Release an identity-less runtime slot on drop: a peripheral's
           rotating secondary BDADDR caught mid classic reconnect before its
           name resolved leaves a phantom `devices` row (class=0x000000,
           name=-). A real bond carries a name/class/key/LTK or the LE flag,
           so it is kept. */
        bt_device_drop_if_identity_less(dev);
    }
    else {
        bt_emit("disconnect handle=0x%04X status=%u reason=%u\n",
            handle, payload[0], payload[3]);
    }
    slog("bluetooth acl_disconnect h=0x%04x reason=0x%02x\n", handle, payload[3]);
    /* Release all channel slots before the controller can reuse this handle. */
    l2cap_link_closed(handle);
    bt_le_link_closed(handle, payload[3]);
    if (_pending.handle == handle) {
        bt_clear_pending();
    }
}

void bt_handle_auth_complete(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    uint16_t handle;
    char addr[24];

    if (len < 3) {
        return;
    }

    handle = (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
    dev = bt_find_device_by_handle(handle);
    if (dev == NULL) {
        return;
    }

    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    if (payload[0] == 0) {
        bt_known_touch_from_device(dev);
        bt_emit("pair_ok %s handle=0x%04X\n", addr, handle);
        /* the link is authenticated; a classic HID peripheral still needs it
           encrypted before it will send reports. Request encryption and bring
           the HID channels up from the encryption-change event. If the send
           itself fails, fall back to starting HID on the authenticated link. */
        if (dev->hid_after_sec) {
            _sec_encrypt_handle = handle;
            if (bt_hci_set_conn_encrypt(handle) != 0) {
                _sec_encrypt_handle = 0;
                dev->hid_after_sec = false;
                if (!bt_hogp_blocks_classic(dev)) {
                    bt_hid_start(handle, dev->addr);
                }
            }
        }
    }
    else {
        bt_emit("pair_fail %s status=%u\n", addr, payload[0]);
        dev->hid_after_sec = false;
    }
    if (_pending.type == BT_PENDING_PAIR && _pending.handle == handle) {
        bt_clear_pending();
    }
}

void bt_handle_pin_code_request(const uint8_t* payload, size_t len) {
    uint8_t params[23];
    char addr[24];

    if (len < 6) {
        return;
    }

    bt_addr_to_str(payload, addr, sizeof(addr));
    if (_pending.type != BT_PENDING_PAIR || !bt_pending_matches_addr(payload)) {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_PIN_CODE_REQ_NEG_REPLY, payload, 6);
        bt_emit("pair_reject %s reason=no_pin\n", addr);
        return;
    }

    memset(params, 0, sizeof(params));
    memcpy(params, payload, 6);
    params[6] = (uint8_t)strlen(_pending.pin);
    memcpy(params + 7, _pending.pin, params[6]);
    bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_PIN_CODE_REQ_REPLY, params, sizeof(params));
    bt_emit("pair_pin %s len=%u\n", addr, params[6]);
}

void bt_handle_link_key_request(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    uint8_t params[22];
    char addr[24];

    if (len < 6) {
        return;
    }

    dev = bt_find_device(payload, false);
    bt_addr_to_str(payload, addr, sizeof(addr));
    if (dev == NULL || !dev->has_link_key) {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_LINK_KEY_REQ_NEG_REPLY, payload, 6);
        bt_emit("link_key_miss %s\n", addr);
        /* bt_emit only feeds /dev/bt0 subscribers; mirror the bond decisions
           into /dev/log so a fresh pairing is distinguishable from a
           stored-key reconnect in a plain log capture */
        slog("bt link_key_miss %s\n", addr);
        return;
    }

    memcpy(params, payload, 6);
    memcpy(params + 6, dev->link_key, 16);
    bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_LINK_KEY_REQ_REPLY, params, sizeof(params));
    bt_emit("link_key_use %s\n", addr);
    slog("bt link_key_use %s\n", addr);
}

void bt_handle_link_key_notify(const uint8_t* payload, size_t len) {
    bt_device_t* dev;
    char addr[24];

    if (len < 23) {
        return;
    }

    dev = bt_find_device(payload, true);
    if (dev == NULL) {
        return;
    }

    memcpy(dev->link_key, payload + 6, 16);
    dev->has_link_key = true;
    bt_known_touch_from_device(dev);
    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    bt_emit("link_key_saved %s type=%u\n", addr, payload[22]);
    /* key type is the Secure-Connections verdict: 0x07/0x08 = P-256,
       0x04/0x05 = legacy P-192, so it must be visible in /dev/log */
    slog("bt link_key_saved %s type=%u\n", addr, (unsigned)payload[22]);
}

void bt_handle_conn_request(const uint8_t* payload, size_t len) {
    char addr[24];

    if (len < 10) {
        return;
    }

    bt_addr_to_str(payload, addr, sizeof(addr));
    bt_emit("incoming_connect %s class=0x%02X%02X%02X link=%u\n",
        addr, payload[8], payload[7], payload[6], payload[9]);
    /* accept so a reconnecting mouse (or any known device paging us)
       gets its ACL link back; role 0x01 keeps us slave */
    bt_hci_accept_connection(payload);
}

void bt_handle_io_capability_request(const uint8_t* payload, size_t len) {
    uint8_t params[9];
    char addr[24];

    if (len < 6) {
        return;
    }

    memset(params, 0, sizeof(params));
    memcpy(params, payload, 6);
    /* IO_Capability DisplayYesNo: this host HAS a screen and confirm input,
       and it is what Windows/BlueZ answer. Against a DisplayYesNo pad the
       SSP model becomes Numeric Comparison with an AUTHENTICATED
       combination key (type 5/8) instead of the unauthenticated type 4
       that NoInputNoOutput produced; Xbox-lineage firmware treats the
       unauthenticated bond as second-class. NoInputNoOutput peers (mice,
       keyboards) still degrade to Just Works, so existing devices pair
       unchanged. */
    params[6] = 0x01;
    /* no OOB pairing data present */
    params[7] = 0x00;
    /* Authentication_Requirements = MITM Not Required - General Bonding.
       This MUST request bonding: a No-Bonding value makes the controller
       finish SSP without deriving a link key, so LINK_KEY_NOTIFY never
       arrives, has_link_key stays false, the device shows paired=0 and it
       cannot re-authenticate on the next connection. */
    params[8] = 0x04;
    bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_IO_CAPABILITY_REQ_REPLY, params, sizeof(params));
    bt_addr_to_str(payload, addr, sizeof(addr));
    bt_emit("pair_io_cap %s capability=noinput bonding=general\n", addr);
    slog("bt pair_io_cap %s\n", addr);
}

void bt_handle_user_confirmation_request(const uint8_t* payload, size_t len) {
    char addr[24];

    if (len < 10) {
        return;
    }

    bt_addr_to_str(payload, addr, sizeof(addr));
    if (_pending.type == BT_PENDING_PAIR && bt_pending_matches_addr(payload)) {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_USER_CONFIRM_REQ_REPLY, payload, 6);
        bt_emit("pair_confirm %s auto=yes\n", addr);
        slog("bt pair_confirm %s auto=yes\n", addr);
    }
    else {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_USER_CONFIRM_REQ_NEG_REPLY, payload, 6);
        bt_emit("pair_confirm %s auto=no\n", addr);
        slog("bt pair_confirm %s auto=no\n", addr);
    }
}

void bt_handle_user_passkey_request(const uint8_t* payload, size_t len) {
    uint8_t params[10];
    char* endptr;
    unsigned long passkey;
    char addr[24];

    if (len < 6) {
        return;
    }

    bt_addr_to_str(payload, addr, sizeof(addr));
    if (_pending.type != BT_PENDING_PAIR || !bt_pending_matches_addr(payload)) {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_USER_PASSKEY_REQ_NEG_REPLY, payload, 6);
        bt_emit("pair_passkey %s auto=no\n", addr);
        return;
    }

    passkey = strtoul(_pending.pin, &endptr, 10);
    if (*_pending.pin == 0 || *endptr != 0 || passkey > 999999UL) {
        bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_USER_PASSKEY_REQ_NEG_REPLY, payload, 6);
        bt_emit("pair_passkey %s auto=no\n", addr);
        return;
    }

    memset(params, 0, sizeof(params));
    memcpy(params, payload, 6);
    params[6] = (uint8_t)(passkey & 0xff);
    params[7] = (uint8_t)((passkey >> 8) & 0xff);
    params[8] = (uint8_t)((passkey >> 16) & 0xff);
    params[9] = (uint8_t)((passkey >> 24) & 0xff);
    bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_USER_PASSKEY_REQ_REPLY, params, sizeof(params));
    bt_emit("pair_passkey %s auto=yes value=%lu", addr, passkey);
}

void bt_handle_simple_pairing_complete(const uint8_t* payload, size_t len) {
    char addr[24];

    if (len < 7) {
        return;
    }

    bt_addr_to_str(payload + 1, addr, sizeof(addr));
    bt_emit("pair_complete %s status=%u", addr, payload[0]);
}
