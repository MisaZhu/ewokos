/* btd_l2cap.c - ACL data path, L2CAP channels and inbound
             signaling.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

/* host->controller ACL flow control: credits start at the controller's
   total ACL buffer count (READ_BUFFER_SIZE) and are returned by
   Number_Of_Completed_Packets events */
uint16_t _acl_credits = 0;

/* Classic HID and LE ATT/SMP fragments may be interleaved by the controller.
   Keep incomplete PDUs per ACL handle, sized for the MTU we advertise. */
typedef struct {
    bool active;
    uint16_t handle;
    uint16_t total;
    uint16_t pos;
    uint8_t data[4 + L2CAP_MTU_DEFAULT];
} l2cap_reassembly_t;
static l2cap_reassembly_t _reassembly[MAX_L2CAP_CHANS];

l2cap_chan_t _l2chans[MAX_L2CAP_CHANS];

uint16_t _l2_next_cid = 0x0040;

uint8_t _l2_next_sig_id = 1;

void l2cap_rx_reset(void) {
    memset(_reassembly, 0, sizeof(_reassembly));
}

void l2cap_link_closed(uint16_t handle) {
    int i;

    /* Reset the session before detaching its pointers; retain disconnect
       notification semantics while releasing every channel, including orphans. */
    bt_hid_link_closed(handle, "acl_disconnect");
    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (_l2chans[i].used && _l2chans[i].acl_handle == handle) {
            l2cap_chan_close(&_l2chans[i], false);
        }
        if (_reassembly[i].active && _reassembly[i].handle == handle) {
            memset(&_reassembly[i], 0, sizeof(_reassembly[i]));
        }
    }
}

void l2cap_recv_acl(uint16_t handle, uint8_t pb, const uint8_t* data, size_t len) {
    l2cap_reassembly_t* r = NULL;
    int i;

    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (_reassembly[i].active && _reassembly[i].handle == handle) {
            r = &_reassembly[i];
            break;
        }
    }
    if (pb == 0 || pb == 2) {
        uint16_t payload_len;
        if (r != NULL) {
            r->active = false; /* a new start replaces this handle's old PDU */
        }
        if (len < 4) {
            return;
        }
        payload_len = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
        if (payload_len > L2CAP_MTU_DEFAULT || len > (size_t)payload_len + 4) {
            return;
        }
        if (r == NULL) {
            for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
                if (!_reassembly[i].active) {
                    r = &_reassembly[i];
                    break;
                }
            }
        }
        if (r == NULL) {
            return;
        }
        r->active = true;
        r->handle = handle;
        r->total = payload_len + 4;
        r->pos = 0;
    }
    else if (pb != 1 || r == NULL) {
        return;
    }
    if (len > (size_t)(r->total - r->pos)) {
        r->active = false;
        return;
    }
    memcpy(r->data + r->pos, data, len);
    r->pos += (uint16_t)len;
    if (r->pos == r->total) {
        uint8_t complete[4 + L2CAP_MTU_DEFAULT];
        uint16_t total = r->total;
        memcpy(complete, r->data, total);
        r->active = false;
        /* A handler may perform a synchronous HCI wait and reuse this slot. */
        l2cap_dispatch(handle, complete, total);
    }
}

/* ---------------- host->controller ACL data path ---------------- */

static int bt_hci_send_acl(uint16_t handle, const uint8_t* data, size_t len) {
    uint8_t pkt[4 + 264];

    if (len > 260) {
        return -1;
    }
    pkt[0] = (uint8_t)(handle & 0xff);
    /* PB=0b10: first automatically-flushable packet of a new L2CAP PDU
       (our outbound PDUs always fit a single ACL packet) */
    pkt[1] = (uint8_t)(((handle >> 8) & 0x0f) | 0x20);
    pkt[2] = (uint8_t)(len & 0xff);
    pkt[3] = (uint8_t)(len >> 8);
    memcpy(pkt + 4, data, len);

    if (_acl_credits > 0) {
        --_acl_credits;
    }
    else {
        /* out of controller buffer credits: still push the packet (the
           uart's hardware flow control protects the transport) but say
           so - sustained credit exhaustion would mean lost reports */
        slog("bluetooth acl no_credits handle=0x%04x\n", handle);
    }
    return bt_hci_send_packet(HCI_PKT_ACL, pkt, 4 + len);
}

void bt_handle_num_completed_pkts(const uint8_t* payload, size_t len) {
    uint8_t num;
    size_t i;

    if (len < 1) {
        return;
    }
    num = payload[0];
    for (i = 0; i < num && 1 + (i + 1) * 4 <= len; ++i) {
        uint16_t completed = (uint16_t)payload[3 + i * 4] |
            ((uint16_t)payload[4 + i * 4] << 8);
        _acl_credits = (uint16_t)(_acl_credits + completed);
    }
}

/* ---------------- L2CAP (basic mode, HID PSMs only) ---------------- */

/* Dynamic CIDs belong to an ACL link, not to the controller globally. */
static l2cap_chan_t* l2cap_find_by_local(uint16_t handle, uint16_t cid) {
    int i;

    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (_l2chans[i].used && _l2chans[i].acl_handle == handle &&
                _l2chans[i].local_cid == cid) {
            return &_l2chans[i];
        }
    }
    return NULL;
}

static l2cap_chan_t* l2cap_find_any(uint16_t handle, uint16_t psm) {
    int i;

    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (_l2chans[i].used && _l2chans[i].acl_handle == handle &&
                _l2chans[i].psm == psm) {
            return &_l2chans[i];
        }
    }
    return NULL;
}

static uint16_t l2cap_alloc_cid(void) {
    uint16_t cid = _l2_next_cid++;

    if (_l2_next_cid >= 0xfff0) { /* keep clear of the reserved top range */
        _l2_next_cid = 0x0040;
    }
    return cid;
}

static uint8_t l2cap_next_sig_id(void) {
    uint8_t id = _l2_next_sig_id++;

    if (_l2_next_sig_id == 0) {
        _l2_next_sig_id = 1;
    }
    return id;
}

/* one L2CAP PDU wrapped into a single ACL packet (PB=0b10) */
int l2cap_send_pdu(uint16_t handle, uint16_t cid,
        const uint8_t* payload, uint16_t payload_len) {
    uint8_t pdu[4 + 256];

    if (payload_len > 256) {
        return -1;
    }
    pdu[0] = (uint8_t)(payload_len & 0xff);
    pdu[1] = (uint8_t)(payload_len >> 8);
    pdu[2] = (uint8_t)(cid & 0xff);
    pdu[3] = (uint8_t)(cid >> 8);
    if (payload_len > 0 && payload != NULL) {
        memcpy(pdu + 4, payload, payload_len);
    }
    return bt_hci_send_acl(handle, pdu, 4 + payload_len);
}

static int l2cap_send_signal(uint16_t handle, uint8_t code, uint8_t id,
        const uint8_t* data, uint8_t data_len) {
    uint8_t cmd[4 + 64];

    if (data_len > 64) {
        return -1;
    }
    cmd[0] = code;
    cmd[1] = id;
    cmd[2] = data_len;
    cmd[3] = 0;
    if (data_len > 0) {
        memcpy(cmd + 4, data, data_len);
    }
    return l2cap_send_pdu(handle, L2CAP_CID_SIGNAL, cmd, 4 + data_len);
}

static void l2cap_send_conn_req(l2cap_chan_t* ch) {
    uint8_t data[4];

    if (ch->sig_id == 0) {
        ch->sig_id = l2cap_next_sig_id();
    }
    data[0] = (uint8_t)(ch->psm & 0xff);
    data[1] = (uint8_t)(ch->psm >> 8);
    data[2] = (uint8_t)(ch->local_cid & 0xff);
    data[3] = (uint8_t)(ch->local_cid >> 8);
    l2cap_send_signal(ch->acl_handle, L2CAP_SIG_CONN_REQ, ch->sig_id, data, sizeof(data));
}

/* request the default MTU and accept whatever options the device asks
   for on the other direction (boot mouse reports are a few bytes) */
static void l2cap_send_conf_req(l2cap_chan_t* ch) {
    uint8_t data[8];

    /* A retransmission is the same transaction: retain its identifier so
       a delayed response to the first transmission still matches. */
    if (!ch->conf_req_sent) {
        ch->sig_id = l2cap_next_sig_id();
    }
    data[0] = (uint8_t)(ch->remote_cid & 0xff);
    data[1] = (uint8_t)(ch->remote_cid >> 8);
    data[2] = 0;
    data[3] = 0;
    data[4] = 0x01; /* option: MTU */
    data[5] = 0x02;
    data[6] = (uint8_t)(L2CAP_MTU_DEFAULT & 0xff);
    data[7] = (uint8_t)(L2CAP_MTU_DEFAULT >> 8);
    ch->conf_req_sent = true;
    l2cap_send_signal(ch->acl_handle, L2CAP_SIG_CONF_REQ, ch->sig_id, data, sizeof(data));
}

static void l2cap_send_conf_rsp(l2cap_chan_t* ch, uint8_t id, uint16_t flags,
        uint16_t result) {
    uint8_t data[6];

    /* Source CID identifies the requester's endpoint, i.e. the remote CID. */
    data[0] = (uint8_t)(ch->remote_cid & 0xff);
    data[1] = (uint8_t)(ch->remote_cid >> 8);
    data[2] = (uint8_t)flags;
    data[3] = (uint8_t)(flags >> 8);
    data[4] = (uint8_t)(result & 0xff);
    data[5] = (uint8_t)(result >> 8);
    ch->conf_rsp_sent = l2cap_send_signal(ch->acl_handle, L2CAP_SIG_CONF_RSP,
        id, data, sizeof(data)) == 0 && flags == 0 && result == L2CAP_CONF_SUCCESS;
}

static void l2cap_send_conn_rsp(uint16_t handle, uint8_t id, uint16_t dcid,
        uint16_t scid, uint16_t result) {
    uint8_t data[8];

    data[0] = (uint8_t)(dcid & 0xff);
    data[1] = (uint8_t)(dcid >> 8);
    data[2] = (uint8_t)(scid & 0xff);
    data[3] = (uint8_t)(scid >> 8);
    data[4] = (uint8_t)(result & 0xff);
    data[5] = (uint8_t)(result >> 8);
    data[6] = 0; /* status */
    data[7] = 0;
    l2cap_send_signal(handle, L2CAP_SIG_CONN_RSP, id, data, sizeof(data));
}

/* open (or reuse) one channel of the HID pair on an established link;
   a reconnecting mouse may already have opened one side itself, which
   l2cap_chan_open detects and leaves alone */
l2cap_chan_t* l2cap_chan_open(uint16_t handle, uint16_t psm) {
    l2cap_chan_t* ch = l2cap_find_any(handle, psm);
    int i;

    if (ch != NULL) {
        /* A closing channel still owns its CID until the response/timeout.
           ACL disconnect cleanup, not an unrelated start, releases it. */
        return ch;
    }
    else {
        for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
            if (!_l2chans[i].used) {
                ch = &_l2chans[i];
                break;
            }
        }
        if (ch == NULL) {
            slog("bluetooth l2cap no_channel handle=0x%04x psm=0x%04x\n", handle, psm);
            return NULL;
        }
        memset(ch, 0, sizeof(*ch));
        ch->used = true;
        ch->local_cid = l2cap_alloc_cid();
    }

    ch->acl_handle = handle;
    ch->psm = psm;
    ch->state = L2CAP_STATE_CONN_REQ_SENT;
    ch->retries = 0;
    ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
    l2cap_send_conn_req(ch);
    return ch;
}

void l2cap_chan_close(l2cap_chan_t* ch, bool send_req) {
    if (ch == NULL || !ch->used) {
        return;
    }
    if (_hid.up && (_hid.ctrl == ch || _hid.intr == ch)) {
        char addr[24];
        bt_addr_to_str(_hid.addr, addr, sizeof(addr));
        bt_emit("hid_disconnect %s reason=l2cap_channel_closed\n", addr);
    }
    if (_hid.ctrl == ch || _hid.intr == ch) {
        _hid.boot_protocol_pending = false;
    }
    if (_hid.ctrl == ch) {
        _hid.ctrl = NULL;
        _hid.up = false;
        _hid.boot_protocol_ok = false;
    }
    if (_hid.intr == ch) {
        _hid.intr = NULL;
        _hid.up = false;
    }
    if (send_req && ch->state == L2CAP_STATE_CLOSING) {
        return;
    }
    if (send_req && ch->state != L2CAP_STATE_CLOSED && ch->remote_cid != 0) {
        uint8_t data[4];

        data[0] = (uint8_t)(ch->remote_cid & 0xff);
        data[1] = (uint8_t)(ch->remote_cid >> 8);
        data[2] = (uint8_t)(ch->local_cid & 0xff);
        data[3] = (uint8_t)(ch->local_cid >> 8);
        ch->sig_id = l2cap_next_sig_id();
        l2cap_send_signal(ch->acl_handle, L2CAP_SIG_DISCONN_REQ,
            ch->sig_id, data, sizeof(data));
        ch->state = L2CAP_STATE_CLOSING;
        ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
        return;
    }
    memset(ch, 0, sizeof(*ch));
}

/* Called only after configuration completes in both directions. */
static void l2cap_mark_open(l2cap_chan_t* ch) {
    ch->state = L2CAP_STATE_OPEN;
    if (ch->psm == L2CAP_PSM_HID_CTRL && _hid.active && _hid.ctrl == ch) {
        /* Preserve the control channel's initiator when progressing the pair. */
        bt_hid_start(ch->acl_handle, _hid.addr);
    }
    bt_hid_check_up();
}

/* drive every channel's handshake forward; called from bt_loop and from
   the synchronous command-wait pump so setup progresses either way */
void l2cap_step(void) {
    uint64_t now = kernel_tic_ms(0);
    int i;

    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        l2cap_chan_t* ch = &_l2chans[i];

        if (!ch->used) {
            continue;
        }
        switch (ch->state) {
        case L2CAP_STATE_CONN_REQ_SENT:
            if (now >= ch->retry_ms) {
                if (++ch->retries > L2CAP_STEP_MAX_RETRIES) {
                    slog("bluetooth l2cap conn_req_timeout psm=0x%04x\n", ch->psm);
                    l2cap_chan_close(ch, false);
                    break;
                }
                ch->retry_ms = now + L2CAP_STEP_TIMEOUT_MS;
                l2cap_send_conn_req(ch);
            }
            break;
        case L2CAP_STATE_CONF_SENT:
            /* Neither an ACL connection nor one-sided configuration makes
               the channel ready. Keep driving both sides during BLE waits. */
            if (ch->conf_rsp_recv && ch->conf_req_recv && ch->conf_rsp_sent) {
                l2cap_mark_open(ch);
            }
            else if (now >= ch->retry_ms) {
                if (++ch->retries > L2CAP_STEP_MAX_RETRIES) {
                    slog("bluetooth l2cap config_timeout h=0x%04x psm=0x%04x local=0x%04x remote=0x%04x id=%u req_rx=%d rsp_rx=%d rsp_tx=%d\n",
                        ch->acl_handle, ch->psm, ch->local_cid, ch->remote_cid,
                        ch->sig_id, ch->conf_req_recv, ch->conf_rsp_recv,
                        ch->conf_rsp_sent);
                    l2cap_chan_close(ch, true);
                    break;
                }
                ch->retry_ms = now + L2CAP_STEP_TIMEOUT_MS;
                if (!ch->conf_rsp_recv) {
                    l2cap_send_conf_req(ch);
                }
            }
            break;
        case L2CAP_STATE_CLOSING:
            if (now >= ch->retry_ms) {
                l2cap_chan_close(ch, false);
            }
            break;
        default:
            break;
        }
    }
    bt_hid_step();
}

/* ---------------- inbound L2CAP signaling ---------------- */

static void l2cap_handle_conn_req(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint16_t psm;
    uint16_t rcid;
    l2cap_chan_t* ch;
    int i;

    if (len < 4) {
        return;
    }
    psm = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    rcid = (uint16_t)data[2] | ((uint16_t)data[3] << 8);

    if (psm != L2CAP_PSM_HID_CTRL && psm != L2CAP_PSM_HID_INTR) {
        l2cap_send_conn_rsp(handle, id, 0, rcid, L2CAP_CONN_PSM_UNSUPPORTED);
        return;
    }

    if (rcid < 0x0040) {
        l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0006); /* invalid source CID */
        return;
    }
    if (_hid.active && _hid.acl_handle != handle) {
        /* This driver has one classic session; do not displace it on accept. */
        l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004);
        return;
    }
    /* Retransmitted inbound requests must not erase a configured channel.
       If requests cross, accept the peer's direction with a fresh local CID;
       a late response to the abandoned outbound request then cannot match. */
    ch = l2cap_find_any(handle, psm);
    if (ch != NULL && ch->state != L2CAP_STATE_CONN_REQ_SENT) {
        if (ch->remote_cid == rcid && ch->state != L2CAP_STATE_CLOSING) {
            l2cap_send_conn_rsp(handle, id, ch->local_cid, rcid, L2CAP_CONN_SUCCESS);
        }
        else {
            l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004);
        }
        return;
    }
    if (ch == NULL) {
        for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
            if (!_l2chans[i].used) {
                ch = &_l2chans[i];
                break;
            }
        }
    }
    if (ch == NULL) {
        l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004); /* no resources */
        return;
    }

    memset(ch, 0, sizeof(*ch));
    ch->used = true;
    ch->incoming = true;
    ch->acl_handle = handle;
    ch->psm = psm;
    ch->local_cid = l2cap_alloc_cid();
    ch->remote_cid = rcid;
    ch->state = L2CAP_STATE_CONF_SENT;
    ch->retries = 0;
    ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
    l2cap_send_conn_rsp(handle, id, ch->local_cid, rcid, L2CAP_CONN_SUCCESS);
    /* send our config request right away; the device's own request is
       answered in l2cap_handle_conf_req */
    l2cap_send_conf_req(ch);

    bt_hid_accept(ch);
}

static void l2cap_handle_conn_rsp(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint16_t dcid;
    uint16_t scid;
    uint16_t result;
    l2cap_chan_t* ch;

    (void)handle;
    if (len < 8) {
        return;
    }
    dcid = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    scid = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
    result = (uint16_t)data[4] | ((uint16_t)data[5] << 8);

    ch = l2cap_find_by_local(handle, scid);
    if (ch == NULL || ch->state != L2CAP_STATE_CONN_REQ_SENT || ch->sig_id != id) {
        return;
    }
    if (result == L2CAP_CONN_PENDING) {
        ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
        return; /* security/authorization is in progress, not a refusal */
    }
    if (result != L2CAP_CONN_SUCCESS || dcid < 0x0040) {
        slog("bluetooth l2cap conn_refused h=0x%04x psm=0x%04x result=%u\n",
            handle, ch->psm, result);
        l2cap_chan_close(ch, false);
        return;
    }
    ch->remote_cid = dcid;
    ch->state = L2CAP_STATE_CONF_SENT;
    ch->retries = 0;
    ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
    l2cap_send_conf_req(ch);
}

/* accept the device's options verbatim (boot mouse payloads are tiny,
   any default MTU works) */
static void l2cap_handle_conf_req(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint16_t dcid;
    l2cap_chan_t* ch;

    if (len < 4) {
        return;
    }
    /* Destination CID is local to this ACL link. A remote-CID fallback can
       accidentally configure the other HID channel when CID values overlap. */
    dcid = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    ch = l2cap_find_by_local(handle, dcid);
    if (ch == NULL) {
        uint8_t reject[6] = { 0x02, 0x00, 0, 0, 0, 0 }; /* invalid CID */
        reject[2] = (uint8_t)dcid;
        reject[3] = (uint8_t)(dcid >> 8);
        l2cap_send_signal(handle, L2CAP_SIG_CMD_REJECT, id, reject, sizeof(reject));
        return;
    }
    if (ch->state != L2CAP_STATE_CONF_SENT && ch->state != L2CAP_STATE_OPEN) {
        return;
    }
    {
        uint16_t flags = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
        ch->conf_req_recv = (flags & 1) == 0;
        l2cap_send_conf_rsp(ch, id, flags & 1, L2CAP_CONF_SUCCESS);
    }
}

static void l2cap_handle_conf_rsp(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint16_t scid, result, flags;
    l2cap_chan_t* ch;

    if (len < 6) {
        return;
    }
    scid = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    flags = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
    result = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
    ch = l2cap_find_by_local(handle, scid);
    if (ch == NULL || ch->state != L2CAP_STATE_CONF_SENT ||
            !ch->conf_req_sent || ch->sig_id != id) {
        return; /* late/foreign responses cannot close a different transaction */
    }
    if (result == L2CAP_CONF_PENDING) {
        ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
        return;
    }
    if (result != L2CAP_CONF_SUCCESS) {
        slog("bluetooth l2cap conf_refused h=0x%04x psm=0x%04x result=%u\n",
            handle, ch->psm, result);
        l2cap_chan_close(ch, true);
        return;
    }
    if ((flags & 1) == 0) {
        ch->conf_rsp_recv = true;
    }
}

static void l2cap_handle_disconn_req(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint16_t dcid;
    uint16_t scid;
    l2cap_chan_t* ch;

    if (len < 4) {
        return;
    }
    dcid = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    scid = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
    ch = l2cap_find_by_local(handle, dcid);
    /* echo the request back as the response (spec: same dcid/scid) */
    l2cap_send_signal(handle, L2CAP_SIG_DISCONN_RSP, id, data, 4);
    if (ch != NULL && ch->remote_cid == scid) {
        l2cap_chan_close(ch, false);
        if (_hid.active && _hid.acl_handle == handle &&
                _hid.ctrl == NULL && _hid.intr == NULL) {
            bt_hid_link_closed(handle, "l2cap_disconnect");
        }
    }
    (void)scid;
}

/* Feature discovery can gate a peer's connection/configuration state machine.
   Reply even before a dynamic channel exists; do not advertise ERTM or LE's
   fixed channels on the BR/EDR signaling channel. */
static void l2cap_handle_info_req(uint16_t handle, uint8_t id,
        const uint8_t* data, size_t len) {
    uint8_t rsp[12] = {0};
    uint8_t rsp_len = 4;
    uint16_t type;

    if (len != 2) {
        return;
    }
    type = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    rsp[0] = data[0];
    rsp[1] = data[1];
    if (type == L2CAP_INFO_EXT_FEATURES) {
        rsp[4] = 0x80; /* fixed-channel information supported; basic mode only */
        rsp_len = 8;
    }
    else if (type == L2CAP_INFO_FIXED_CHANNELS) {
        rsp[4] = 0x02; /* bit 1: BR/EDR signaling CID 0x0001 */
        rsp_len = 12;
    }
    else {
        rsp[2] = 0x01; /* information type not supported (including connectionless MTU) */
    }
    l2cap_send_signal(handle, L2CAP_SIG_INFO_RSP, id, rsp, rsp_len);
}

static void l2cap_handle_signal(uint16_t handle, const uint8_t* pdu, size_t len) {
    size_t off = 0;

    while (off + 4 <= len) {
        uint8_t code = pdu[off];
        uint8_t id = pdu[off + 1];
        uint16_t clen = (uint16_t)pdu[off + 2] | ((uint16_t)pdu[off + 3] << 8);
        const uint8_t* data = pdu + off + 4;

        if (off + 4 + clen > len) {
            break;
        }
        if (id == 0) {
            off += 4 + clen; /* identifier zero is reserved */
            continue;
        }
        switch (code) {
        case L2CAP_SIG_CONN_REQ:
            l2cap_handle_conn_req(handle, id, data, clen);
            break;
        case L2CAP_SIG_CONN_RSP:
            l2cap_handle_conn_rsp(handle, id, data, clen);
            break;
        case L2CAP_SIG_CONF_REQ:
            l2cap_handle_conf_req(handle, id, data, clen);
            break;
        case L2CAP_SIG_CONF_RSP:
            l2cap_handle_conf_rsp(handle, id, data, clen);
            break;
        case L2CAP_SIG_DISCONN_REQ:
            l2cap_handle_disconn_req(handle, id, data, clen);
            break;
        case L2CAP_SIG_DISCONN_RSP:
            if (clen >= 4) {
                uint16_t dcid = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
                uint16_t scid = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
                l2cap_chan_t* ch = l2cap_find_by_local(handle, scid);
                /* The response echoes our request: DCID is remote, SCID local. */
                if (ch != NULL && ch->remote_cid == dcid &&
                        ch->state == L2CAP_STATE_CLOSING && ch->sig_id == id) {
                    l2cap_chan_close(ch, false);
                }
            }
            break;
        case L2CAP_SIG_ECHO_REQ:
            /* answer with the same data so link supervision works */
            l2cap_send_signal(handle, L2CAP_SIG_ECHO_RSP, id, data,
                clen > 64 ? 64 : (uint8_t)clen);
            break;
        case L2CAP_SIG_INFO_REQ:
            l2cap_handle_info_req(handle, id, data, clen);
            break;
        case L2CAP_SIG_CMD_REJECT:
        case L2CAP_SIG_INFO_RSP:
        case L2CAP_SIG_ECHO_RSP:
            break; /* no reciprocal response, especially to Command Reject */
        default: {
            uint8_t reason[2] = {0, 0}; /* command not understood */
            l2cap_send_signal(handle, L2CAP_SIG_CMD_REJECT, id, reason, sizeof(reason));
            break;
        }
        }
        off += 4 + clen;
    }
}

static void l2cap_chan_data(l2cap_chan_t* ch, const uint8_t* payload, size_t len) {
    if (ch->psm == L2CAP_PSM_HID_CTRL) {
        bt_hid_handle_ctrl(ch, payload, len);
    }
    else if (ch->psm == L2CAP_PSM_HID_INTR) {
        if (len >= 1 && payload[0] == HIDP_DATA_INPUT) {
            const uint8_t* report = payload + 1;
            size_t rlen = len - 1;
            /* A boot keyboard report is exactly 8 bytes; a 9-byte report is
               always report-protocol with a leading Report ID octet. Strip
               it so the remaining 8 bytes line up with the boot layout
               [modifiers, reserved, key1..key6] that hid_keybd decodes.
               Gate on LENGTH only, not on boot_protocol_ok: some devices
               acknowledge SET_PROTOCOL with a handshake-success yet keep
               emitting report-id-prefixed reports, and trusting the flag
               left the Report ID (typically 0x01) misread as the modifier
               byte - a permanent phantom LCTRL that turns letters into
               control codes while digits/symbols pass through. */
            if (rlen == HID_KEYBOARD_REPORT_SIZE + 1) {
                report++;
                rlen--;
            }
            bt_hid_handle_report(report, rlen);
        }
    }
}

void l2cap_dispatch(uint16_t handle, const uint8_t* pdu, size_t len) {
    uint16_t pdu_len;
    uint16_t cid;
    l2cap_chan_t* ch;

    if (len < 4) {
        return;
    }
    pdu_len = (uint16_t)pdu[0] | ((uint16_t)pdu[1] << 8);
    cid = (uint16_t)pdu[2] | ((uint16_t)pdu[3] << 8);
    if (pdu_len != len - 4) {
        return; /* only complete, correctly framed PDUs may change state */
    }

    /* the LE fixed channels are connectionless from L2CAP's point of view:
       they are never opened by signalling and never appear in _l2chans, so
       they are handed to the LE stack before any channel lookup. */
    if (cid == L2CAP_CID_ATT || cid == L2CAP_CID_SMP ||
            cid == L2CAP_CID_LE_SIGNAL) {
        bt_le_l2cap_rx(handle, cid, pdu + 4, pdu_len);
        return;
    }

    if (cid == L2CAP_CID_SIGNAL) {
        l2cap_handle_signal(handle, pdu + 4, pdu_len);
        return;
    }
    ch = l2cap_find_by_local(handle, cid);
    if (ch == NULL || ch->state != L2CAP_STATE_OPEN) {
        return;
    }
    l2cap_chan_data(ch, pdu + 4, pdu_len);
}
