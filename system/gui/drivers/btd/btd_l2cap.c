/* btd_l2cap.c - ACL data path, L2CAP channels and inbound
             signaling.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

#define SDP_IDLE_TIMEOUT_MS 30000

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
    int ret = bt_hci_send_packet(HCI_PKT_ACL, pkt, 4 + len);
    return ret;
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

/* ---------------- L2CAP (basic mode, SDP and HID PSMs) ---------------- */

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

    if (ch != NULL && psm == L2CAP_PSM_SDP && ch->incoming) {
        /* SDP has a channel per direction: the pad's inbound browse must
           not satisfy our outbound record-fetch open. */
        ch = NULL;
    }
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
    bt_hid_chan_t* h;

    if (ch == NULL || !ch->used) {
        return;
    }
    h = bt_hid_by_handle(ch->acl_handle);
    if (h != NULL && h->sdp == ch) {
        h->sdp = NULL; /* report-map client channel, unrelated to HID attach */
    }
    if (h != NULL && h->up && (h->ctrl == ch || h->intr == ch)) {
        char addr[24];
        bt_addr_to_str(h->addr, addr, sizeof(addr));
        bt_emit("hid_disconnect %s reason=l2cap_channel_closed\n", addr);
    }
    if (h != NULL && (h->ctrl == ch || h->intr == ch)) {
        h->boot_protocol_pending = false;
    }
    if (h != NULL && h->ctrl == ch) {
        h->ctrl = NULL;
        h->up = false;
        h->boot_protocol_ok = false;
    }
    if (h != NULL && h->intr == ch) {
        h->intr = NULL;
        h->up = false;
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

static void sdp_client_send(l2cap_chan_t* ch);
static void sdp_client_data(l2cap_chan_t* ch, const uint8_t* data, size_t len);

/* Called only after configuration completes in both directions. */
static void l2cap_mark_open(l2cap_chan_t* ch) {
    bt_hid_chan_t* h;

    ch->state = L2CAP_STATE_OPEN;
    if (ch->psm == L2CAP_PSM_SDP) {
        if (ch->sdp_client) {
            sdp_client_send(ch);
            return;
        }
        ch->retry_ms = kernel_tic_ms(0) + SDP_IDLE_TIMEOUT_MS;
        return; /* SDP never owns or advances either HID channel. */
    }
    h = bt_hid_by_handle(ch->acl_handle);
    if (ch->psm == L2CAP_PSM_HID_CTRL && h != NULL && h->ctrl == ch) {
        /* Preserve the control channel's initiator when progressing the pair. */
        bt_hid_start(ch->acl_handle, h->addr);
    }
    bt_hid_check_up(h);
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
        case L2CAP_STATE_OPEN:
            if (ch->psm == L2CAP_PSM_SDP && now >= ch->retry_ms) {
                l2cap_chan_close(ch, true); /* release SDP only, keep HID/ACL up */
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

    if (psm != L2CAP_PSM_SDP && psm != L2CAP_PSM_HID_CTRL &&
            psm != L2CAP_PSM_HID_INTR) {
        l2cap_send_conn_rsp(handle, id, 0, rcid, L2CAP_CONN_PSM_UNSUPPORTED);
        return;
    }

    if (rcid < 0x0040) {
        l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0006); /* invalid source CID */
        return;
    }
    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (_l2chans[i].used && _l2chans[i].acl_handle == handle &&
                _l2chans[i].remote_cid == rcid && _l2chans[i].psm != psm) {
            l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0007); /* source CID in use */
            return;
        }
    }
    if (psm != L2CAP_PSM_SDP && bt_hid_by_handle(handle) == NULL &&
            bt_hid_slot_free() == NULL) {
        /* Both classic HID sessions are taken by other links; SDP stays
           independent and is never counted against them. */
        l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004);
        return;
    }
    /* Retransmitted inbound requests must not erase a configured channel.
       If requests cross, accept the peer's direction with a fresh local CID;
       a late response to the abandoned outbound request then cannot match. */
    ch = l2cap_find_any(handle, psm);
    if (psm == L2CAP_PSM_SDP && ch != NULL &&
            (!ch->incoming || ch->remote_cid != rcid ||
             ch->state == L2CAP_STATE_CONN_REQ_SENT)) {
        /* SDP is exempt from the cross-collapse above: both directions are
           independent channels and routinely coexist (our device-record
           fetch while the pad browses our PnP record). l2cap_find_any
           returns our OUTBOUND client channel first, and reusing it here
           would wipe it — the late response to our request then matches
           nothing and the peer's config request on the CID it opened gets
           an invalid-CID reject. Only a same-rcid incoming channel is a
           retransmit of this request. */
        ch = NULL;
    }
    if (ch != NULL && ch->state != L2CAP_STATE_CONN_REQ_SENT) {
        if (ch->remote_cid == rcid && ch->state != L2CAP_STATE_CLOSING) {
            l2cap_send_conn_rsp(handle, id, ch->local_cid, rcid, L2CAP_CONN_SUCCESS);
        }
        else {
            l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004);
        }
        return;
    }
    if (ch == NULL && psm == L2CAP_PSM_SDP) {
        int sdp_channels = 0;
        for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
            if (_l2chans[i].used && _l2chans[i].psm == L2CAP_PSM_SDP) {
                ++sdp_channels;
            }
        }
        /* Discovery must leave room for every session's control/interrupt
           pair. */
        if (sdp_channels >= MAX_L2CAP_CHANS - MAX_CLASSIC_HID_SESSIONS * 2) {
            l2cap_send_conn_rsp(handle, id, 0, rcid, 0x0004);
            return;
        }
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

    if (psm != L2CAP_PSM_SDP) {
        bt_hid_accept(ch);
    }
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
        /* Includes the late answer to an outbound request a crossed inbound
           one collapsed: leave the peer's channel alone. Disconnecting it
           was tried and made the Zikway pad drop its HID binding outright;
           bt_hid_start's peer head start keeps the cross from happening. */
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
        bool hid_channel = ch->psm == L2CAP_PSM_HID_CTRL || ch->psm == L2CAP_PSM_HID_INTR;
        l2cap_chan_close(ch, false);
        if (hid_channel) {
            bt_hid_chan_t* h = bt_hid_by_handle(handle);
            if (h != NULL && h->ctrl == NULL && h->intr == NULL) {
                bt_hid_link_closed(handle, "l2cap_disconnect");
            }
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

/* ---------------- SDP server (independent of HID session ownership) ---------------- */

#define SDP_ERROR_RSP 0x01
#define SDP_SEARCH_REQ 0x02
#define SDP_SEARCH_RSP 0x03
#define SDP_ATTR_REQ 0x04
#define SDP_ATTR_RSP 0x05
#define SDP_SEARCH_ATTR_REQ 0x06
#define SDP_SEARCH_ATTR_RSP 0x07
#define SDP_ERR_HANDLE 0x0002
#define SDP_ERR_SYNTAX 0x0003
#define SDP_ERR_SIZE 0x0004
#define SDP_ERR_CONT 0x0005

/* All basic-mode peers support 48 bytes. Stay within that minimum and the
   existing 256-byte transmit limit without assuming a peer's negotiated MTU. */
#define SDP_RSP_MAX 48
#define SDP_CONT_SIZE 6
#define SDP_ATTR_CHUNK (SDP_RSP_MAX - 5 - 2 - 1 - SDP_CONT_SIZE)

/* Two records. The well-known Service Discovery Server record (handle zero),
   plus the PnP Information record every consumer host publishes: classic
   gamepads classify the host over SDP before streaming (observed on the
   GameSir/Xbox-mode pad: after its HIDP probe it ServiceSearchAttributes
   UUID 0x1200 for all attributes, then holds back input when the answer
   comes back empty). Only advertise what is true here: a HID host is not
   a HID device, so there is deliberately no 0x1124 record. Each row is a
   0x09 uint16 attribute ID header followed by one data element. */
static const uint8_t _sdp_server_attrs[] = {
    0x09, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, /* record handle 0 */
    0x09, 0x00, 0x01, 0x35, 0x03, 0x19, 0x10, 0x00, /* SDP server class */
    0x09, 0x02, 0x00, 0x35, 0x03, 0x09, 0x01, 0x00, /* SDP version 1.0 */
    0x09, 0x02, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x01  /* static DB state */
};

/* DID 1.3 identifying a PC-class host (Microsoft vendor via the Bluetooth
   SIG vendor source), the identity pads pair with in their PC/Xbox mode. */
static const uint8_t _sdp_did_attrs[] = {
    0x09, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, /* record handle 1 */
    0x09, 0x00, 0x01, 0x35, 0x03, 0x19, 0x12, 0x00, /* PnP Information */
    0x09, 0x02, 0x00, 0x09, 0x01, 0x03,             /* DID spec 1.3 */
    0x09, 0x02, 0x01, 0x09, 0x04, 0x5e,             /* vendor: Microsoft */
    0x09, 0x02, 0x02, 0x09, 0x00, 0x01,             /* product 1 */
    0x09, 0x02, 0x03, 0x09, 0x01, 0x00,             /* version 1.0 */
    0x09, 0x02, 0x04, 0x28, 0x01,                   /* primary record */
    0x09, 0x02, 0x05, 0x09, 0x00, 0x01              /* vendor source: SIG */
};

typedef struct {
    uint32_t handle;
    uint16_t class_uuid; /* sole ServiceClassIDList entry and search key */
    const uint8_t* attrs;
    uint16_t attrs_len;
} sdp_record_t;

static const sdp_record_t _sdp_records[] = {
    {0, 0x1000, _sdp_server_attrs, sizeof(_sdp_server_attrs)},
    {1, 0x1200, _sdp_did_attrs, sizeof(_sdp_did_attrs)}
};

#define SDP_RECORD_COUNT (sizeof(_sdp_records) / sizeof(_sdp_records[0]))

typedef struct {
    const uint8_t* data;
    size_t len;
} sdp_cursor_t;

static uint16_t sdp_be16(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t sdp_be32(const uint8_t* p) {
    return ((uint32_t)sdp_be16(p) << 16) | sdp_be16(p + 2);
}

static void sdp_put16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void sdp_put32(uint8_t* p, uint32_t v) {
    sdp_put16(p, (uint16_t)(v >> 16));
    sdp_put16(p + 2, (uint16_t)v);
}

/* Only flat sequences are legal in search patterns and attribute-ID lists.
   Accept all three sequence length encodings, without recursive parsing. */
static bool sdp_sequence(sdp_cursor_t* in, sdp_cursor_t* seq) {
    if (in->len < 2 || in->data[0] < 0x35 || in->data[0] > 0x37) {
        return false;
    }
    size_t size_bytes = (size_t)1 << (in->data[0] - 0x35);
    size_t header = 1 + size_bytes;
    if (in->len < header) return false;
    uint32_t n = 0;
    for (size_t i = 1; i < header; ++i) n = (n << 8) | in->data[i];
    if (n > in->len - header) return false;
    seq->data = in->data + header;
    seq->len = n;
    in->data += header + n;
    in->len -= header + n;
    return true;
}

/* Search pattern UUIDs widened to 32 bits; 0xffffffff marks a 128-bit UUID
   outside the Bluetooth base, which no 16-bit class list can ever match. */
typedef struct {
    uint32_t uuids[12];
    unsigned count;
} sdp_pattern_t;

typedef struct {
    uint16_t first, last;
} sdp_range_t;

#define SDP_MAX_RANGES 8

static bool sdp_search_pattern(sdp_cursor_t* in, sdp_pattern_t* pat) {
    static const uint8_t base[12] = {
        0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
        0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
    };
    sdp_cursor_t seq;

    if (!sdp_sequence(in, &seq) || seq.len == 0) return false;
    pat->count = 0;
    while (seq.len != 0) {
        uint8_t type = seq.data[0];
        size_t n = type == 0x19 ? 2 : type == 0x1a ? 4 : type == 0x1c ? 16 : 0;
        uint32_t uuid;

        if (n == 0 || n + 1 > seq.len || pat->count >= 12) return false;
        if (n == 2) {
            uuid = sdp_be16(seq.data + 1);
        }
        else if (n == 4) {
            uuid = sdp_be32(seq.data + 1);
        }
        else {
            uuid = seq.data[1] == 0 && seq.data[2] == 0 &&
                memcmp(seq.data + 5, base, sizeof(base)) == 0 ?
                sdp_be16(seq.data + 3) : 0xffffffffu;
        }
        pat->uuids[pat->count++] = uuid;
        seq.data += n + 1;
        seq.len -= n + 1;
    }
    return true;
}

/* SDP patterns AND all UUIDs; a single-UUID class list matches only when
   every pattern UUID equals it. */
static bool sdp_record_matches(const sdp_record_t* rec, const sdp_pattern_t* pat) {
    unsigned i;

    for (i = 0; i < pat->count; ++i) {
        if (pat->uuids[i] != rec->class_uuid) return false;
    }
    return true;
}

static bool sdp_attribute_ids(sdp_cursor_t* in, sdp_range_t* ranges,
        unsigned* count) {
    sdp_cursor_t seq;

    if (!sdp_sequence(in, &seq) || seq.len == 0) return false;
    *count = 0;
    while (seq.len != 0) {
        uint8_t type = seq.data[0];
        size_t n = type == 0x09 ? 2 : type == 0x0a ? 4 : 0;

        if (n == 0 || n + 1 > seq.len || *count >= SDP_MAX_RANGES) return false;
        ranges[*count].first = sdp_be16(seq.data + 1);
        ranges[*count].last = n == 2 ? ranges[*count].first : sdp_be16(seq.data + 3);
        if (ranges[*count].first > ranges[*count].last) return false;
        ++*count;
        seq.data += n + 1;
        seq.len -= n + 1;
    }
    return true;
}

/* One hand-built row: 0x09 + uint16 ID + one data element. Returns the row
   length and the attribute ID, 0 when the bytes are not a complete row. */
static size_t sdp_row_len(const uint8_t* p, size_t left, uint16_t* id) {
    size_t elem;

    if (left < 4 || p[0] != 0x09) return 0;
    switch (p[3]) {
    case 0x09: case 0x19: elem = 3; break; /* uint16/uuid16 */
    case 0x0a: case 0x1a: elem = 5; break; /* uint32/uuid32 */
    case 0x28: elem = 2; break;            /* boolean */
    case 0x35:                             /* SEQ8 */
        if (left < 5) return 0;
        elem = (size_t)p[4] + 2;
        break;
    default: return 0;
    }
    if (elem > left - 3) return 0;
    *id = sdp_be16(p + 1);
    return 3 + elem;
}

static uint8_t sdp_attr_mask(const sdp_record_t* rec,
        const sdp_range_t* ranges, unsigned count) {
    uint8_t mask = 0;
    const uint8_t* p = rec->attrs;
    size_t left = rec->attrs_len;
    unsigned bit = 0;

    while (left != 0 && bit < 8) {
        uint16_t id;
        size_t row = sdp_row_len(p, left, &id);
        unsigned i;

        if (row == 0) break;
        for (i = 0; i < count; ++i) {
            if (id >= ranges[i].first && id <= ranges[i].last) {
                mask |= (uint8_t)(1u << bit);
                break;
            }
        }
        p += row;
        left -= row;
        ++bit;
    }
    return mask;
}

static size_t sdp_attributes(uint8_t* out, const sdp_record_t* rec, uint8_t mask) {
    size_t pos = 2;
    const uint8_t* p = rec->attrs;
    size_t left = rec->attrs_len;
    unsigned bit = 0;

    out[0] = 0x35;
    while (left != 0 && bit < 8) {
        uint16_t id;
        size_t row = sdp_row_len(p, left, &id);

        if (row == 0) break;
        if ((mask & (1u << bit)) != 0) {
            memcpy(out + pos, p, row);
            pos += row;
        }
        p += row;
        left -= row;
        ++bit;
    }
    out[1] = (uint8_t)(pos - 2);
    return pos;
}

/* Consistency cookie, not a security token. The immutable database allows
   retrying a page with a new transaction ID. Bind the offset to this channel
   and the entire query (excluding the header and continuation state). */
static uint32_t sdp_query_cookie(l2cap_chan_t* ch, uint8_t pdu,
        const uint8_t* data, size_t len) {
    uint32_t hash = 2166136261u ^ ((uint32_t)ch->acl_handle << 16) ^ ch->local_cid;
    hash = (hash ^ pdu) * 16777619u;
    for (size_t i = 0; i < len; ++i) hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

static void sdp_reply(l2cap_chan_t* ch, uint16_t tid, uint8_t pdu,
        const uint8_t* params, size_t len) {
    uint8_t rsp[SDP_RSP_MAX];
    if (len > sizeof(rsp) - 5) return;
    rsp[0] = pdu;
    sdp_put16(rsp + 1, tid);
    sdp_put16(rsp + 3, (uint16_t)len);
    memcpy(rsp + 5, params, len);
    int ret = l2cap_send_pdu(ch->acl_handle, ch->remote_cid, rsp, (uint16_t)(len + 5));
    /* Only failures are worth a line; a successful browse is routine. */
    if (pdu == SDP_ERROR_RSP || ret != 0) {
        slog("bt sdp_tx h=%04x tid=%u pdu=%02x len=%u error=%04x ret=%d\n",
            ch->acl_handle, tid, pdu, (unsigned)(len + 5),
            pdu == SDP_ERROR_RSP ? sdp_be16(params) : 0, ret);
    }
}

static void sdp_error(l2cap_chan_t* ch, uint16_t tid, uint16_t error) {
    uint8_t params[2];
    sdp_put16(params, error);
    sdp_reply(ch, tid, SDP_ERROR_RSP, params, sizeof(params));
}

static void sdp_handle_data(l2cap_chan_t* ch, const uint8_t* data, size_t len) {
    if (len < 3) return; /* no complete transaction ID to echo */
    ch->retry_ms = kernel_tic_ms(0) + SDP_IDLE_TIMEOUT_MS;
    uint16_t tid = sdp_be16(data + 1);
    if (len < 5 || sdp_be16(data + 3) != len - 5) {
        sdp_error(ch, tid, SDP_ERR_SIZE);
        return;
    }
    uint8_t pdu = data[0];
    sdp_cursor_t in = {data + 5, len - 5};
    sdp_pattern_t pat = {{0}, 0}; /* parsed only on the search PDUs */
    sdp_range_t ranges[SDP_MAX_RANGES];
    unsigned range_count = 0;
    uint32_t handle = 0;
    if (pdu == SDP_SEARCH_REQ || pdu == SDP_SEARCH_ATTR_REQ) {
        if (!sdp_search_pattern(&in, &pat)) goto syntax_error;
    }
    else if (pdu == SDP_ATTR_REQ) {
        if (in.len < 4) goto syntax_error;
        handle = sdp_be32(in.data);
        in.data += 4;
        in.len -= 4;
    }
    else {
        goto syntax_error;
    }
    if (in.len < 2) goto syntax_error;
    uint16_t maximum = sdp_be16(in.data);
    in.data += 2;
    in.len -= 2;
    if (maximum < (pdu == SDP_SEARCH_REQ ? 1 : 7)) goto syntax_error;
    if (pdu != SDP_SEARCH_REQ &&
            !sdp_attribute_ids(&in, ranges, &range_count)) goto syntax_error;
    if (in.len == 0 || in.data[0] > 16 || in.len != (size_t)in.data[0] + 1) {
        sdp_error(ch, tid, SDP_ERR_CONT);
        return;
    }
    if (pdu == SDP_SEARCH_REQ) {
        /* Two handles always fit, so no search continuation is ever issued. */
        if (in.data[0] != 0) {
            sdp_error(ch, tid, SDP_ERR_CONT);
            return;
        }
        uint8_t params[4 + SDP_RECORD_COUNT * 4 + 1];
        size_t total = 0;
        size_t i;
        for (i = 0; i < SDP_RECORD_COUNT; ++i) {
            if (sdp_record_matches(&_sdp_records[i], &pat)) {
                sdp_put32(params + 4 + total * 4, _sdp_records[i].handle);
                ++total;
            }
        }
        sdp_put16(params, (uint16_t)total);
        sdp_put16(params + 2, (uint16_t)total);
        params[4 + total * 4] = 0; /* final continuation state */
        sdp_reply(ch, tid, SDP_SEARCH_RSP, params, 5 + total * 4);
        return;
    }
    /* 2 outer header + 2 list header per record + all attribute bytes */
    const sdp_record_t* rec = NULL;
    uint8_t list[2 + 2 * 2 + sizeof(_sdp_server_attrs) + sizeof(_sdp_did_attrs)];
    size_t total;
    if (pdu == SDP_ATTR_REQ) {
        size_t i;
        for (i = 0; i < SDP_RECORD_COUNT; ++i) {
            if (_sdp_records[i].handle == handle) rec = &_sdp_records[i];
        }
        if (rec == NULL) {
            sdp_error(ch, tid, SDP_ERR_HANDLE);
            return;
        }
        total = sdp_attributes(list, rec,
            sdp_attr_mask(rec, ranges, range_count));
    }
    else {
        size_t pos = 2;
        size_t i;
        list[0] = 0x35;
        for (i = 0; i < SDP_RECORD_COUNT; ++i) {
            const sdp_record_t* r = &_sdp_records[i];
            uint8_t mask;
            if (!sdp_record_matches(r, &pat)) continue;
            mask = sdp_attr_mask(r, ranges, range_count);
            if (mask == 0) continue; /* matched record without requested attrs */
            pos += sdp_attributes(list + pos, r, mask);
        }
        list[1] = (uint8_t)(pos - 2);
        total = pos; /* no matching attributes: an empty outer sequence */
    }
    uint32_t cookie = sdp_query_cookie(ch, pdu, data + 5, (size_t)(in.data - data - 5));
    size_t offset = 0;
    if (in.data[0] != 0) {
        if (in.data[0] != SDP_CONT_SIZE || sdp_be32(in.data + 3) != cookie) {
            sdp_error(ch, tid, SDP_ERR_CONT);
            return;
        }
        offset = sdp_be16(in.data + 1);
        if (offset == 0 || offset >= total) {
            sdp_error(ch, tid, SDP_ERR_CONT);
            return;
        }
    }
    size_t count = total - offset;
    if (count > maximum) count = maximum;
    if (count > SDP_ATTR_CHUNK) count = SDP_ATTR_CHUNK;
    uint8_t params[SDP_RSP_MAX - 5];
    sdp_put16(params, (uint16_t)count);
    memcpy(params + 2, list + offset, count);
    size_t pos = 2 + count;
    offset += count;
    if (offset < total) {
        params[pos++] = SDP_CONT_SIZE;
        sdp_put16(params + pos, (uint16_t)offset);
        sdp_put32(params + pos + 2, cookie);
        pos += SDP_CONT_SIZE;
    }
    else {
        params[pos++] = 0;
    }
    sdp_reply(ch, tid, pdu == SDP_ATTR_REQ ? SDP_ATTR_RSP : SDP_SEARCH_ATTR_RSP,
        params, pos);
    return;

syntax_error:
    sdp_error(ch, tid, SDP_ERR_SYNTAX);
}

/* ---------------- SDP client (report-map discovery) ----------------
   A real HID host reads the device's HID service record over SDP once per
   connection; the HIDDescriptorList attribute carries the report descriptor
   that defines this pad's report IDs and payload layout, and some pads hold
   back input streaming until such a host-style discovery has happened. One
   bounded query per gamepad session, kicked from bt_hid_step once the HID
   pair is up; the raw attribute bytes are dumped for offline decoding. */

#define SDP_CLI_MAX_BYTE_COUNT 0x02a0
#define SDP_CLI_MAX_CONT 4
#define SDP_CLI_REQ_TIMEOUT_MS 5000

void bt_sdp_client_kick(bt_hid_chan_t* h) {
    l2cap_chan_t* ch = NULL;
    int i;

    if (h == NULL || !h->active || h->sdp_map_started) {
        return;
    }
    for (i = 0; i < MAX_L2CAP_CHANS; ++i) {
        if (!_l2chans[i].used) {
            ch = &_l2chans[i];
            break;
        }
    }
    if (ch == NULL) {
        return; /* not latched: bt_hid_step retries once a channel frees */
    }
    h->sdp_map_started = true;
    memset(ch, 0, sizeof(*ch));
    ch->used = true;
    ch->sdp_client = true;
    ch->local_cid = l2cap_alloc_cid();
    ch->acl_handle = h->acl_handle;
    ch->psm = L2CAP_PSM_SDP;
    ch->state = L2CAP_STATE_CONN_REQ_SENT;
    ch->retries = 0;
    ch->retry_ms = kernel_tic_ms(0) + L2CAP_STEP_TIMEOUT_MS;
    h->sdp = ch;
    l2cap_send_conn_req(ch);
}

static void sdp_client_send(l2cap_chan_t* ch) {
    static const uint8_t query[] = {
        0x35, 0x03, 0x19, 0x11, 0x24, /* search pattern: UUID16 HID service */
        0x02, 0xa0, /* MaxAttributeByteCount 672: one response fits our MTU */
        0x35, 0x05, 0x0a, 0x00, 0x00, 0xff, 0xff /* attribute ID range: all */
    };
    bt_hid_chan_t* h = bt_hid_by_handle(ch->acl_handle);
    uint8_t req[5 + sizeof(query) + 17];
    size_t n = 5;

    if (h == NULL || h->sdp != ch) {
        l2cap_chan_close(ch, true);
        return;
    }
    if (++h->sdp_tid == 0) {
        ++h->sdp_tid;
    }
    req[0] = SDP_SEARCH_ATTR_REQ;
    sdp_put16(req + 1, h->sdp_tid);
    memcpy(req + n, query, sizeof(query));
    n += sizeof(query);
    if (h->sdp_cont[0] >= 1 && h->sdp_cont[0] <= 16) {
        memcpy(req + n, h->sdp_cont, (size_t)h->sdp_cont[0] + 1);
        n += (size_t)h->sdp_cont[0] + 1;
    }
    else {
        req[n++] = 0; /* no continuation state */
    }
    sdp_put16(req + 3, (uint16_t)(n - 5));
    ch->retry_ms = kernel_tic_ms(0) + SDP_CLI_REQ_TIMEOUT_MS;
    l2cap_send_pdu(ch->acl_handle, ch->remote_cid, req, (uint16_t)n);
}

/* SDP data element header: type in the top five bits, size index in the low
   three (0..4 fixed 1/2/4/8/16 bytes, 5..7 a 1/2/4-byte length follows).
   Fills header and payload sizes; false when the element overruns len. */
static bool sdp_elem(const uint8_t* p, size_t len, uint8_t* type,
        size_t* hdr, size_t* dlen) {
    static const size_t fixed[5] = {1, 2, 4, 8, 16};
    size_t idx, n;

    if (len < 1) return false;
    *type = p[0] >> 3;
    idx = p[0] & 7;
    if (idx < 5) {
        *hdr = 1;
        n = *type == 0 ? 0 : fixed[idx]; /* nil has no payload */
    }
    else {
        size_t lb = (size_t)1 << (idx - 5);
        if (len < 1 + lb) return false;
        n = 0;
        for (size_t i = 0; i < lb; ++i) n = (n << 8) | p[1 + i];
        *hdr = 1 + lb;
    }
    if (n > len - *hdr) return false;
    *dlen = n;
    return true;
}

/* Walk the reassembled AttributeLists for the HID record's HIDDescriptorList
   (0x0206): DES { DES { uint8 class type, string descriptor } ... }. The
   Report descriptor (class type 0x22) is fed to the shared joystick probe
   so bt_hid_handle_report can normalize this pad's frames by its real
   layout instead of guessing from the leading report ID. */
static void sdp_client_parse_hid(bt_hid_chan_t* h) {
    const uint8_t* p = h->sdp_rec;
    size_t len = h->sdp_rec_len, hdr, n;
    uint8_t type;

    h->joystick_ok = false;
    if (!sdp_elem(p, len, &type, &hdr, &n) || type != 6) return;
    p += hdr; len = n;                 /* list of records */
    while (len != 0) {
        if (!sdp_elem(p, len, &type, &hdr, &n) || type != 6) return;
        const uint8_t* rec = p + hdr;
        size_t rlen = n;
        p += hdr + n; len -= hdr + n;
        while (rlen != 0) {
            size_t ahdr, alen, vhdr, vlen;
            uint8_t at, vt;
            if (!sdp_elem(rec, rlen, &at, &ahdr, &alen) || at != 1 || alen != 2) return;
            uint16_t id = sdp_be16(rec + ahdr);
            rec += ahdr + alen; rlen -= ahdr + alen;
            if (!sdp_elem(rec, rlen, &vt, &vhdr, &vlen)) return;
            if (id == 0x0206 && vt == 6) {
                const uint8_t* d = rec + vhdr;
                size_t dlen = vlen;
                while (dlen != 0) {
                    size_t ihdr, ilen;
                    uint8_t it;
                    if (!sdp_elem(d, dlen, &it, &ihdr, &ilen) || it != 6) return;
                    const uint8_t* e = d + ihdr;
                    size_t elen = ilen;
                    d += ihdr + ilen; dlen -= ihdr + ilen;
                    size_t chdr, clen, shdr, slen;
                    uint8_t ct, st;
                    if (!sdp_elem(e, elen, &ct, &chdr, &clen) || ct != 1 || clen != 1) return;
                    uint8_t class_type = e[chdr];
                    e += chdr + clen; elen -= chdr + clen;
                    if (!sdp_elem(e, elen, &st, &shdr, &slen) || st != 4) return;
                    if (class_type != 0x22) continue;
                    h->joystick_ok = hid_probe_joystick_report(e + shdr, (int)slen, &h->joystick);
                    if (h->joystick_ok) {
                        /* Same profile choice as the HOGP path: Xbox-lineage
                           pads number their face buttons 1=A 2=B 3=X 4=Y. */
                        bt_device_t* dev = bt_find_device_by_handle(h->acl_handle);
                        h->joystick.map_type = bt_dev_is_xbox_gamepad(dev) ?
                            JS_MAP_XBOX : JS_MAP_DEFAULT;
                    }
                    return;
                }
            }
            rec += vhdr + vlen; rlen -= vhdr + vlen;
        }
    }
}

static void sdp_client_data(l2cap_chan_t* ch, const uint8_t* data, size_t len) {
    bt_hid_chan_t* h = bt_hid_by_handle(ch->acl_handle);
    size_t params, total, cont;

    if (h == NULL || h->sdp != ch || len < 5) {
        l2cap_chan_close(ch, true);
        return;
    }
    if (data[0] == SDP_ERROR_RSP) {
        slog("bt sdp_cli_error h=%04x tid=%u error=%04x\n",
            (unsigned)ch->acl_handle, sdp_be16(data + 1),
            len >= 7 ? sdp_be16(data + 5) : 0);
        h->sdp_map_done = true; /* failed: unblock the deferred HID channels */
        l2cap_chan_close(ch, true);
        return;
    }
    if (data[0] != SDP_SEARCH_ATTR_RSP || sdp_be16(data + 1) != h->sdp_tid ||
            sdp_be16(data + 3) != len - 5) {
        return; /* late or foreign traffic: our response/timeout decides */
    }
    params = len - 5;
    if (params < 3) {
        h->sdp_map_done = true;
        l2cap_chan_close(ch, true);
        return;
    }
    total = sdp_be16(data + 5); /* AttributeListsByteCount */
    if (total > params - 3) {
        h->sdp_map_done = true;
        l2cap_chan_close(ch, true);
        return;
    }
    cont = 2 + total; /* offset of the continuation state in params */
    if (data[5 + cont] > 16 || params != cont + 1 + data[5 + cont]) {
        h->sdp_map_done = true;
        l2cap_chan_close(ch, true);
        return;
    }
    h->sdp_map_off = (uint16_t)(h->sdp_map_off + total);
    if (total != 0 && h->sdp_rec_len + total <= sizeof(h->sdp_rec)) {
        memcpy(h->sdp_rec + h->sdp_rec_len, data + 7, total);
        h->sdp_rec_len = (uint16_t)(h->sdp_rec_len + total);
    }
    if (data[5 + cont] == 0 || ++h->sdp_cont_iters > SDP_CLI_MAX_CONT) {
        sdp_client_parse_hid(h);
        h->sdp_map_done = true; /* enumeration complete: HID channels may open */
        l2cap_chan_close(ch, true);
        return;
    }
    memcpy(h->sdp_cont, data + 5 + cont, data[5 + cont] + 1);
    sdp_client_send(ch);
}

static void l2cap_chan_data(l2cap_chan_t* ch, const uint8_t* payload, size_t len) {
    if (ch->psm == L2CAP_PSM_SDP) {
        if (ch->sdp_client) {
            sdp_client_data(ch, payload, len);
        }
        else {
            sdp_handle_data(ch, payload, len);
        }
        return;
    }
    bt_hid_chan_t* h = bt_hid_by_handle(ch->acl_handle);
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
            bt_hid_handle_report(h, h != NULL ? &h->held : NULL, report, rlen);
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
