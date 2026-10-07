/* btd_le.c - the entire LE / HOGP stack: AES, RNG, scanning,
        GAP, ATT/GATT client, SMP and HID-over-GATT bring-up.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

bool _scanning = false;

le_session_t _les[MAX_LE_SESSIONS];

int _le_cur = 0;  /* session index used by blocking GATT/SMP code */

/* map a controller connection handle back to the session that owns it.
   Exported so the classic-HID paths can recognise an LE handle and never
   bind classic HIDP to it (see bt_hid_start / the hid_open handler). */
int le_session_by_handle(uint16_t handle) {
    int i;
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.handle_valid && _les[i].le.handle == handle)
            return i;
    }
    return -1;
}

/* first session slot that is free to start a new connection */
int le_session_free(void) {
    int i;
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_IDLE && !_les[i].le.handle_valid)
            return i;
    }
    return -1;
}

uint8_t _local_addr[6];

uint8_t _local_addr_type = BT_LE_ADDR_TYPE_PUBLIC;

bool _le_supported = false;

/* LE Secure Connections controller material. The local P-256 public key is
   read once at bring-up and cached; ECDH via HCI_LE_Generate_DHKey handles
   one peer at a time, which is fine because a bring-up is a single blocking
   sequence. Both results arrive asynchronously in an LE Meta sub-event, so
   a "done" flag releases the waiter even when the status byte is non-zero. */
static uint8_t _local_p256[64];
static bool _local_p256_valid = false;
static bool _p256_done = false;
static uint8_t _dhkey[32];
static bool _dhkey_valid = false;
static bool _dhkey_done = false;

/* An LE bring-up is queued here instead of running inside the command
   handler: it blocks for seconds and the caller is an IPC command from
   xbt or bt_moused. bt_le_step picks the request up. */
bool _le_req_active = false;

uint8_t _le_req_addr[6];

bool _le_req_pair = false;

uint64_t _le_req_ms = 0;

int _le_req_slot = 0;   /* which session slot to connect into */

bool _le_autoconnect = false;

bt_scan_slice_t _scan_slice = BT_SCAN_SLICE_NONE;

uint64_t _scan_slice_end_ms = 0;

uint64_t _scan_total_end_ms = 0;

bool _le_scan_enabled = false;

/* _le_scan_extended says the currently enabled scan is the extended one, so
   bt_scan_slice_stop disables it with the matching command; _le_ext_scan_supp
   caches the one-time probe of extended-scan support (-1 unknown, 0 no, 1 yes)
   so an unsupported controller is not re-probed on every slice. */
bool _le_scan_extended = false;

int _le_ext_scan_supp = -1;

bool _inquiry_running = false;

/* index of the session streaming HOGP for this address, or -1. Used by the
   classic hid_open path to recognise a dual-mode peripheral that is already
   reporting over one of the LE links. */
int le_session_ready_by_addr(const uint8_t* addr) {
    int i;
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_READY && _les[i].hogp.n_subscribed > 0 &&
                bt_addr_equal(_les[i].le.addr, addr)) {
            return i;
        }
    }
    return -1;
}

/* ================= Bluetooth LE (HOGP) =================
   The LE half of the daemon: it discovers the BLE-only mice and keyboards
   a BR/EDR inquiry can never see, then connects, pairs, walks the HID
   Service and subscribes to its input reports. Like the classic pairing
   path it is written as a bounded blocking sequence - send one HCI
   command or one ATT PDU, wait on a predicate with a deadline - and
   bt_poll_once keeps the radio serviced in between. */

/* ---------- AES-128, encryption only ----------
   SMP's security function e is plain AES-128 ECB over a single block with
   the most significant octet of key/plaintext/ciphertext at index 0
   (Vol 3 Part H 2.2.1). The controller could do it for us
   (HCI_LE_Encrypt) but that command's parameter byte order is the reverse
   of e's, so keeping a small encrypt-only AES here means the crypto can
   be checked against the spec's own worked examples instead of against a
   guess about which end an HCI buffer starts at. The S-box is generated
   on first use: a 256-byte constant table costs more flash than the few
   lines that build it, and it is built exactly once per boot. */
uint8_t _aes_sbox[256];

bool _aes_sbox_ready = false;

static uint8_t aes_xtime(uint8_t a) {
    return (uint8_t)((a << 1) ^ ((a & 0x80) ? 0x1b : 0x00));
}

static uint8_t aes_mul(uint8_t a, uint8_t b) {
    uint8_t r = 0;

    while (b != 0) {
        if (b & 1) {
            r ^= a;
        }
        a = aes_xtime(a);
        b = (uint8_t)(b >> 1);
    }
    return r;
}

/* multiplicative inverse in GF(2^8) by exhaustive search: at most 256
   tries, once per S-box entry, once per boot */
static uint8_t aes_inv(uint8_t a) {
    uint8_t i;

    if (a == 0) {
        return 0;
    }
    for (i = 1; i != 0; ++i) {
        if (aes_mul(a, i) == 1) {
            return i;
        }
    }
    return 0;
}

static void aes_sbox_init(void) {
    int i;

    if (_aes_sbox_ready) {
        return;
    }
    for (i = 0; i < 256; ++i) {
        uint8_t s = aes_inv((uint8_t)i);
        uint8_t v = s;
        int j;

        for (j = 0; j < 4; ++j) {
            v = (uint8_t)((v << 1) | (v >> 7)); /* rotate left within the octet */
            s ^= v;
        }
        _aes_sbox[i] = (uint8_t)(s ^ 0x63);
    }
    _aes_sbox_ready = true;
}

static void aes128_expand_key(const uint8_t key[16], uint8_t rk[176]) {
    static const uint8_t rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10,
                                     0x20, 0x40, 0x80, 0x1b, 0x36};
    int i;

    aes_sbox_init();
    memcpy(rk, key, 16);
    for (i = 4; i < 44; ++i) {
        uint8_t t[4];
        int j;

        memcpy(t, rk + (i - 1) * 4, 4);
        if ((i % 4) == 0) {
            uint8_t first = t[0];
            t[0] = (uint8_t)(_aes_sbox[t[1]] ^ rcon[i / 4 - 1]);
            t[1] = _aes_sbox[t[2]];
            t[2] = _aes_sbox[t[3]];
            t[3] = _aes_sbox[first];
        }
        for (j = 0; j < 4; ++j) {
            rk[i * 4 + j] = (uint8_t)(rk[(i - 4) * 4 + j] ^ t[j]);
        }
    }
}

/* FIPS-197 maps in[r + 4*c] onto state row r column c, so the byte stream
   and the column-major state are the same array and no transposing is
   needed */
static void aes128_encrypt(const uint8_t key[16], const uint8_t in[16],
        uint8_t out[16]) {
    uint8_t rk[176];
    uint8_t st[16];
    int rnd;
    int c;
    int r;
    int i;

    aes128_expand_key(key, rk);
    memcpy(st, in, 16);
    for (i = 0; i < 16; ++i) {
        st[i] ^= rk[i];
    }

    for (rnd = 1; rnd <= 10; ++rnd) {
        for (i = 0; i < 16; ++i) {
            st[i] = _aes_sbox[st[i]];
        }
        /* ShiftRows: row r rotates left by r positions, so rows 1..3 each
           rotate by one, two and three. Row r lives at st[r + 4*c] for
           column c, so new[c] = old[(c + r) % 4]. */
        for (r = 1; r < 4; ++r) {
            uint8_t tmp[4];
            for (c = 0; c < 4; ++c) {
                tmp[c] = st[r + 4 * ((c + r) % 4)];
            }
            for (c = 0; c < 4; ++c) {
                st[r + 4 * c] = tmp[c];
            }
        }
        if (rnd < 10) {
            for (c = 0; c < 4; ++c) {
                uint8_t* s = st + 4 * c;
                uint8_t a0 = s[0];
                uint8_t a1 = s[1];
                uint8_t a2 = s[2];
                uint8_t a3 = s[3];
                uint8_t all = (uint8_t)(a0 ^ a1 ^ a2 ^ a3);

                s[0] = (uint8_t)(a0 ^ all ^ aes_xtime((uint8_t)(a0 ^ a1)));
                s[1] = (uint8_t)(a1 ^ all ^ aes_xtime((uint8_t)(a1 ^ a2)));
                s[2] = (uint8_t)(a2 ^ all ^ aes_xtime((uint8_t)(a2 ^ a3)));
                s[3] = (uint8_t)(a3 ^ all ^ aes_xtime((uint8_t)(a3 ^ a0)));
            }
        }
        for (i = 0; i < 16; ++i) {
            st[i] ^= rk[rnd * 16 + i];
        }
    }
    memcpy(out, st, 16);
}

/* SMP puts every 128-bit value on the wire least-significant octet first
   while e() wants most-significant octet first, so the two orders are a
   plain reverse of each other. */
static void smp_rev16(const uint8_t* in, uint8_t* out) {
    int i;

    for (i = 0; i < 16; ++i) {
        out[i] = in[15 - i];
    }
}

static void smp_e(const uint8_t key_be[16], const uint8_t pt_be[16],
        uint8_t ct_be[16]) {
    aes128_encrypt(key_be, pt_be, ct_be);
}

/* c1(k, r, preq, pres, iat, rat, ia, ra) = e(k, e(k, r XOR p1) XOR p2),
   with every value below shown most-significant octet first (Vol 3 Part H
   2.2.3):
     p1 = pres || preq || rat' || iat'
     p2 = padding(32 zero bits) || ia || ra
   The buffers we hold are all over-the-air (LSB-first), so p1 and p2 are
   assembled by reversing the two 7-octet SMP PDUs and the two 6-octet
   addresses. Verified against the spec's own example: k = 0, r =
   5783D52156AD6F0E6388274EC6702EE0, p1 =
   05000800000302070710000001010001, p2 =
   00000000A1A2A3A4A5A6B1B2B3B4B5B6 yields
   1E1E3FEF878988EAD2A74DC5BEF13B86. */
static void smp_c1(const uint8_t tk[16], const uint8_t r_air[16],
        const uint8_t preq[7], const uint8_t pres[7],
        uint8_t iat, const uint8_t ia[6], uint8_t rat, const uint8_t ra[6],
        uint8_t out_air[16]) {
    uint8_t k_be[16];
    uint8_t p1_be[16];
    uint8_t p2_be[16];
    uint8_t tmp[16];
    int i;

    smp_rev16(tk, k_be);
    smp_rev16(r_air, tmp);

    for (i = 0; i < 7; ++i) {
        p1_be[i] = pres[6 - i];
        p1_be[7 + i] = preq[6 - i];
    }
    p1_be[14] = (uint8_t)(rat & 0x01);
    p1_be[15] = (uint8_t)(iat & 0x01);

    memset(p2_be, 0, 4);
    for (i = 0; i < 6; ++i) {
        p2_be[4 + i] = ia[5 - i];
        p2_be[10 + i] = ra[5 - i];
    }

    for (i = 0; i < 16; ++i) {
        tmp[i] ^= p1_be[i];
    }
    smp_e(k_be, tmp, tmp);
    for (i = 0; i < 16; ++i) {
        tmp[i] ^= p2_be[i];
    }
    smp_e(k_be, tmp, tmp);
    smp_rev16(tmp, out_air);
}

/* s1(k, r1, r2) = e(k, r') with r' = r1' || r2' and r1'/r2' the least
   significant 64 bits of r1/r2 (Vol 3 Part H 2.2.4). The STK is
   s1(TK, LP_RAND_R, LP_RAND_I) - the responder's random comes first. */
static void smp_s1(const uint8_t tk[16], const uint8_t r1_air[16],
        const uint8_t r2_air[16], uint8_t stk_air[16]) {
    uint8_t k_be[16];
    uint8_t r1_be[16];
    uint8_t r2_be[16];
    uint8_t rp_be[16];
    uint8_t tmp[16];

    smp_rev16(tk, k_be);
    smp_rev16(r1_air, r1_be);
    smp_rev16(r2_air, r2_be);
    memcpy(rp_be, r1_be + 8, 8);
    memcpy(rp_be + 8, r2_be + 8, 8);
    smp_e(k_be, rp_be, tmp);
    smp_rev16(tmp, stk_air);
}

/* ---------- LE Secure Connections crypto (Vol 3 Part H 2.2) -------------
   f4/f5/f6 are AES-CMAC constructions. The CMAC core below is RFC 4493 in
   the same most-significant-octet-first domain aes128_encrypt already uses
   (that is what the verified legacy c1 feeds it), so each helper reverses
   its over-the-air (LSB-first) inputs to MSB-first, runs the CMAC and
   reverses the 16-byte result back to air order for the wire / LTK. */

/* reverse n octets: over-the-air LSB-first <-> crypto MSB-first */
static void smp_rev(const uint8_t* in, uint8_t* out, size_t n) {
    size_t i;

    for (i = 0; i < n; ++i) {
        out[i] = in[n - 1 - i];
    }
}

static void cmac_shift_left1(const uint8_t in[16], uint8_t out[16]) {
    uint8_t carry = 0;
    int i;

    for (i = 15; i >= 0; --i) {
        out[i] = (uint8_t)((in[i] << 1) | carry);
        carry = (uint8_t)(in[i] >> 7);
    }
}

/* RFC 4493 AES-128-CMAC over an arbitrary-length MSB-first message. */
static void aes128_cmac(const uint8_t key[16], const uint8_t* msg, size_t len,
        uint8_t mac[16]) {
    uint8_t zero[16];
    uint8_t l[16];
    uint8_t k1[16];
    uint8_t k2[16];
    uint8_t m_last[16];
    uint8_t x[16];
    size_t n;
    size_t i;
    int complete;

    memset(zero, 0, sizeof(zero));
    aes128_encrypt(key, zero, l);
    cmac_shift_left1(l, k1);
    if (l[0] & 0x80) {
        k1[15] ^= 0x87;
    }
    cmac_shift_left1(k1, k2);
    if (k1[0] & 0x80) {
        k2[15] ^= 0x87;
    }

    n = (len + 15) / 16;
    if (n == 0) {
        n = 1;
        complete = 0;
    }
    else {
        complete = (len % 16) == 0;
    }

    if (complete) {
        memcpy(m_last, msg + (n - 1) * 16, 16);
        for (i = 0; i < 16; ++i) {
            m_last[i] ^= k1[i];
        }
    }
    else {
        size_t rem = len % 16;
        memset(m_last, 0, 16);
        memcpy(m_last, msg + (n - 1) * 16, rem);
        m_last[rem] = 0x80;
        for (i = 0; i < 16; ++i) {
            m_last[i] ^= k2[i];
        }
    }

    memset(x, 0, 16);
    for (i = 0; i + 1 < n; ++i) {
        size_t j;
        for (j = 0; j < 16; ++j) {
            x[j] ^= msg[i * 16 + j];
        }
        aes128_encrypt(key, x, x);
    }
    for (i = 0; i < 16; ++i) {
        x[i] ^= m_last[i];
    }
    aes128_encrypt(key, x, mac);
}

/* f4(U, V, X, Z) = AES-CMAC_X(U || V || Z). U and V are the 32-byte
   x-coordinates of the two public keys, X a 16-byte nonce (the CMAC key),
   Z a single octet (0 for Just Works). All inputs air order, output air
   order. Used for the LESC Pairing Confirm value. */
static void smp_f4(const uint8_t u[32], const uint8_t v[32], const uint8_t x[16],
        uint8_t z, uint8_t out_air[16]) {
    uint8_t m[65];
    uint8_t key[16];
    uint8_t mac[16];

    smp_rev(u, m, 32);
    smp_rev(v, m + 32, 32);
    m[64] = z;
    smp_rev(x, key, 16);
    aes128_cmac(key, m, sizeof(m), mac);
    smp_rev(mac, out_air, 16);
}

/* f5(W, N1, N2, A1, A2) -> MacKey || LTK. W is the 32-byte DHKey, N1/N2 the
   two 16-byte nonces, A1/A2 the two 6-byte addresses plus their type
   octets, all air order. MacKey (Counter=0) and LTK (Counter=1) come out in
   air order, so the LTK drops straight into LE_Start_Encryption. */
static void smp_f5(const uint8_t w[32], const uint8_t n1[16], const uint8_t n2[16],
        uint8_t a1type, const uint8_t a1[6], uint8_t a2type, const uint8_t a2[6],
        uint8_t mackey_air[16], uint8_t ltk_air[16]) {
    /* SALT = 0x6C888391AAF5A53860370BDB5A6083BE (Vol 3 Part H 2.2.7),
       written MSB-first exactly as the crypto domain expects it. */
    static const uint8_t salt[16] = {
        0x6C, 0x88, 0x83, 0x91, 0xAA, 0xF5, 0xA5, 0x38,
        0x60, 0x37, 0x0B, 0xDB, 0x5A, 0x60, 0x83, 0xBE
    };
    uint8_t w_msb[32];
    uint8_t t[16];
    uint8_t m[53];
    uint8_t mac[16];

    smp_rev(w, w_msb, 32);
    aes128_cmac(salt, w_msb, 32, t);

    /* m = Counter(1) || keyID "btle"(4) || N1(16) || N2(16) || A1(7) ||
       A2(7) || Length=256(2) = 53 octets, all MSB-first. */
    m[1] = 0x62; /* 'b' */
    m[2] = 0x74; /* 't' */
    m[3] = 0x6C; /* 'l' */
    m[4] = 0x65; /* 'e' */
    smp_rev(n1, m + 5, 16);
    smp_rev(n2, m + 21, 16);
    m[37] = (uint8_t)(a1type & 0x01);
    smp_rev(a1, m + 38, 6);
    m[44] = (uint8_t)(a2type & 0x01);
    smp_rev(a2, m + 45, 6);
    m[51] = 0x01; /* Length = 256, most significant octet */
    m[52] = 0x00;

    m[0] = 0x00; /* Counter = 0 -> MacKey */
    aes128_cmac(t, m, sizeof(m), mac);
    smp_rev(mac, mackey_air, 16);
    m[0] = 0x01; /* Counter = 1 -> LTK */
    aes128_cmac(t, m, sizeof(m), mac);
    smp_rev(mac, ltk_air, 16);
}

/* f6(W, N1, N2, R, IOcap, A1, A2) = AES-CMAC_W(N1||N2||R||IOcap||A1||A2).
   W is the MacKey, N1/N2/R 16 octets, IOcap the 3-octet [AuthReq, OOB,
   IOCap] of the side being checked, A1/A2 the 6-byte addresses with type
   octets. R is all-zero for Just Works. Output air order. */
static void smp_f6(const uint8_t w[16], const uint8_t n1[16], const uint8_t n2[16],
        const uint8_t r[16], const uint8_t iocap[3],
        uint8_t a1type, const uint8_t a1[6], uint8_t a2type, const uint8_t a2[6],
        uint8_t out_air[16]) {
    uint8_t m[65];
    uint8_t key[16];
    uint8_t mac[16];

    smp_rev(n1, m, 16);
    smp_rev(n2, m + 16, 16);
    smp_rev(r, m + 32, 16);
    m[48] = iocap[0]; /* AuthReq  (most significant octet of IOcap) */
    m[49] = iocap[1]; /* OOB flag */
    m[50] = iocap[2]; /* IO capability */
    m[51] = (uint8_t)(a1type & 0x01);
    smp_rev(a1, m + 52, 6);
    m[58] = (uint8_t)(a2type & 0x01);
    smp_rev(a2, m + 59, 6);
    smp_rev(w, key, 16);
    aes128_cmac(key, m, sizeof(m), mac);
    smp_rev(mac, out_air, 16);
}

/* ---------- random ----------
   The controller's own generator (HCI_LE_Rand) is the good source; the
   xorshift below only covers a controller that refuses it, and is seeded
   from the millisecond clock so two boots do not produce the same LTK. */
uint32_t _rng_state = 0;

static uint32_t bt_rng32(void) {
    _rng_state ^= _rng_state << 13;
    _rng_state ^= _rng_state >> 17;
    _rng_state ^= _rng_state << 5;
    return _rng_state;
}

static void bt_fill_random(uint8_t* out, size_t n) {
    size_t pos = 0;

    while (pos < n) {
        uint8_t hw[16];
        uint8_t hw_len = 0;

        if (_le_supported &&
                bt_hci_command_sync_ret(HCI_OGF_LE, HCI_OCF_LE_RAND, NULL, 0,
                    1000, hw, sizeof(hw), &hw_len) == 0 && hw_len == 16) {
            size_t take = n - pos < 16 ? n - pos : 16;
            memcpy(out + pos, hw, take);
            pos += take;
            continue;
        }
        if (_rng_state == 0) {
            _rng_state = (uint32_t)kernel_tic_ms(0) ^ 0x5bf03635u;
            if (_rng_state == 0) {
                _rng_state = 0x12345677u;
            }
        }
        out[pos] = (uint8_t)bt_rng32();
        ++pos;
    }
}

/* True once we hold a usable own address (any non-zero six octets). */
static bool bt_local_addr_valid(void) {
    int i;

    for (i = 0; i < 6; ++i) {
        if (_local_addr[i] != 0) {
            return true;
        }
    }
    return false;
}

/* Establish the address we present on air before any scan or connect. If the
   controller reports no usable public BD_ADDR (an unprogrammed CYW4345C0, or
   a flaky transport that dropped the Read_BD_ADDR reply), we must NOT let
   LE_Create_Connection go out with Own_Address_Type=public and an all-zero
   address: the controller then puts some InitA on air that we do not know,
   the peer hashes that unknown address into its c1 confirm, and every legacy
   pairing aborts with reason 0x04 (Confirm Value Failed). Synthesize a random
   static address, program it with LE_Set_Random_Address and switch our own
   address type to RANDOM, so the InitA on air and the ia we feed c1 are the
   very same value we control. */
static void bt_le_provision_own_addr(void) {
    uint8_t addr_ret[8];
    uint8_t addr_ret_len = 0;
    int rc;

    if (bt_local_addr_valid()) {
        return;
    }

    memset(addr_ret, 0, sizeof(addr_ret));
    rc = bt_hci_command_sync_ret(HCI_OGF_HOST_CTRL, HCI_OCF_READ_BD_ADDR, NULL,
            0, 1000, addr_ret, sizeof(addr_ret), &addr_ret_len);
    if (rc == 0 && addr_ret_len >= 6) {
        memcpy(_local_addr, addr_ret, 6);
    }
    if (bt_local_addr_valid()) {
        return;
    }

    /* No usable public address: build a random static one. The two most
       significant bits of the most significant octet (addr[5]; HCI stores
       octets LSB first) must be 1 for a static random address. */
    bt_fill_random(_local_addr, sizeof(_local_addr));
    _local_addr[5] = (uint8_t)((_local_addr[5] & 0x3f) | 0xc0);
    _local_addr_type = BT_LE_ADDR_TYPE_RANDOM;
    if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_RANDOM_ADDRESS,
            _local_addr, sizeof(_local_addr), 1000) != 0) {
        slog("bluetooth le_set_random_addr_failed\n");
    }
}

static bool bt_poll_until(bt_pred_fn pred, void* ctx, uint32_t timeout_ms) {
    uint64_t start_ms = kernel_tic_ms(0);

    while ((uint32_t)(kernel_tic_ms(0) - start_ms) < timeout_ms) {
        if (pred(ctx)) {
            return true;
        }
        bt_poll_once(2);
        /* BLE pairing/discovery must not stall a concurrent classic handshake. */
        l2cap_step();
    }
    return pred(ctx);
}

/* ---------- controller LE bring-up ----------
   Called from bt_configure_controller. A controller without LE answers
   LE_Set_Event_Mask with Unknown HCI Command and the classic-only path
   carries on; the Pi 5's CYW4345C0 is dual-mode, so this normally
   succeeds. */
/* ---- LE address resolution (privacy) ------------------------------------
   A peripheral that rotates a resolvable private address cannot be tracked
   by its on-air address: every rotation looks like a brand-new device, so the
   scan list fills with duplicates and the bond (keyed by address) never
   matches, forcing a full re-pair each time. The standard remedy is the
   controller's resolving list - hand it each peer's IRK + identity address
   once, enable resolution, and from then on every advertising report and
   connection complete carries the STABLE identity address instead of the
   rotating one. The device table, the bond store and c1 all key off that one
   address, so the duplicates disappear and reconnection re-encrypts.
   Everything here is fail-safe: a controller without LE privacy just refuses
   the commands and we carry on exactly as before (an unresolvable address
   likewise stays as-is, so nothing that works today regresses). */
bool _le_resolving = false;

static int bt_le_resolving_list_add(uint8_t id_addr_type,
        const uint8_t* id_addr, const uint8_t* peer_irk) {
    uint8_t p[39];

    /* Peer_Identity_Address_Type(1) Peer_Identity_Address(6) Peer_IRK(16)
       Local_IRK(16). A zero Local_IRK keeps our own static random address as
       InitA instead of asking the controller to mint an RPA for us. */
    p[0] = id_addr_type;
    memcpy(p + 1, id_addr, 6);
    memcpy(p + 7, peer_irk, 16);
    memset(p + 23, 0, 16);
    return bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_ADD_DEV_RESOLV_LIST,
            p, sizeof(p), 1000);
}

/* Rebuild the resolving list from the bonds loaded at mount and enable
   resolution. Called at the end of the LE controller bring-up, before any
   scan or connection, which is the only point LE_Set_Address_Resolution_Enable
   is guaranteed to be accepted. */
static void bt_le_resolving_setup(void) {
    uint8_t size_ret[4];
    uint8_t size_len = 0;
    uint8_t cap;
    uint8_t on = 1;
    int added = 0;
    int i;

    if (!_le_supported) {
        return;
    }
    /* LE_Read_Resolving_List_Size doubles as the "does this controller
       implement LE privacy at all" probe */
    if (bt_hci_command_sync_ret(HCI_OGF_LE, HCI_OCF_LE_READ_RESOLV_LIST_SIZE,
            NULL, 0, 1000, size_ret, sizeof(size_ret), &size_len) != 0 ||
            size_len < 1 || size_ret[0] == 0) {
        slog("bluetooth le_resolving unsupported\n");
        return;
    }
    cap = size_ret[0];
    (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_CLEAR_RESOLV_LIST,
            NULL, 0, 1000);
    for (i = 0; i < MAX_BT_KNOWN && added < (int)cap; ++i) {
        if (!_known[i].used || !_known[i].has_irk || !_known[i].has_id_addr) {
            continue;
        }
        if (bt_le_resolving_list_add(_known[i].id_addr_type, _known[i].id_addr,
                _known[i].irk) == 0) {
            ++added;
        }
    }
    if (added == 0) {
        return;
    }
    if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_ADDR_RESOLUTION_ENABLE,
            &on, 1, 1000) != 0) {
        slog("bluetooth le_resolving enable_failed added=%d\n", added);
        return;
    }
    _le_resolving = true;
}

/* Note a freshly bonded peer in the resolving list. Adding an entry is
   allowed while a connection is up; enabling resolution is not, so on the
   very first bond it stays off until the next controller init picks it up
   from the store. */
static void bt_le_resolving_note_bond(const bt_device_t* dev) {
    uint8_t on = 1;

    if (!_le_supported || dev == NULL || !dev->has_irk || !dev->has_id_addr) {
        return;
    }
    if (bt_le_resolving_list_add(dev->id_addr_type, dev->id_addr,
            dev->irk) != 0) {
        slog("bluetooth le_resolving add_failed\n");
        return;
    }
    if (!_le_resolving &&
            bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_ADDR_RESOLUTION_ENABLE,
                    &on, 1, 1000) == 0) {
        _le_resolving = true;
    }
}

/* ---------- LE Secure Connections: controller-side ECDH ------------------ */

static bool p256_done_pred(void* ctx) {
    (void)ctx;
    return _p256_done;
}

static bool dhkey_done_pred(void* ctx) {
    (void)ctx;
    return _dhkey_done;
}

/* LE Read Local P-256 Public Key Complete sub-event: [status, key(64)].
   The public key is Qx || Qy, each 32 octets little-endian, exactly the
   order the Pairing Public Key PDU and HCI_LE_Generate_DHKey expect. */
static void bt_le_handle_local_p256(const uint8_t* p, size_t len) {
    if (len >= 65 && p[0] == 0) {
        memcpy(_local_p256, p + 1, 64);
        _local_p256_valid = true;
    }
    else {
        _local_p256_valid = false;
    }
    _p256_done = true;
}

/* LE Generate DHKey Complete sub-event: [status, dhkey(32)] */
static void bt_le_handle_dhkey(const uint8_t* p, size_t len) {
    if (len >= 33 && p[0] == 0) {
        memcpy(_dhkey, p + 1, 32);
        _dhkey_valid = true;
    }
    else {
        _dhkey_valid = false;
    }
    _dhkey_done = true;
}

/* Read and cache the controller's local P-256 public key. Success is the
   proof that this controller implements LE Secure Connections; on failure
   _local_p256_valid stays false and the SMP layer never offers the SC bit,
   so every existing legacy-only peripheral keeps working unchanged. */
static int bt_le_read_local_p256(void) {
    _p256_done = false;
    _local_p256_valid = false;
    if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_READ_LOCAL_P256, NULL, 0,
            1000) != 0) {
        return -1;
    }
    if (!bt_poll_until(p256_done_pred, NULL, 2000) || !_local_p256_valid) {
        slog("bluetooth le_local_p256_failed\n");
        return -1;
    }
    return 0;
}

/* ECDH: hand the controller the peer's 64-byte public key and wait for the
   32-byte shared secret. Blocking, but a bring-up only ever pairs one peer
   (the spec forbids concurrent Generate_DHKey anyway). */
static int bt_le_generate_dhkey(const uint8_t peer_pub[64]) {
    _dhkey_done = false;
    _dhkey_valid = false;
    if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_GENERATE_DHKEY, peer_pub, 64,
            1000) != 0) {
        return -1;
    }
    if (!bt_poll_until(dhkey_done_pred, NULL, 5000) || !_dhkey_valid) {
        slog("bluetooth le_dhkey_failed\n");
        return -1;
    }
    return 0;
}

int bt_le_controller_init(void) {
    uint8_t le_mask[8];
    uint8_t buf_ret[8];
    uint8_t buf_ret_len = 0;
    uint16_t le_len = 0;
    uint16_t le_num = 0;

    _le_supported = false;
    _le_scan_enabled = false;
    bt_le_stack_reset();

    memset(le_mask, 0xff, sizeof(le_mask)); /* every LE sub-event */
    if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_EVENT_MASK, le_mask,
            sizeof(le_mask), 1000) != 0) {
        slog("bluetooth le unsupported (set_event_mask refused)\n");
        return -1;
    }

    /* Our own address (hashed by c1, and used as Own_Address_Type/address by
       LE_Set_Scan_Parameters and LE_Create_Connection) is provisioned once at
       the end of this bring-up, after LE is marked supported so the
       random-static fallback can use hardware randomness and
       LE_Set_Random_Address. */

    /* LE_Read_Buffer_Size: [len_lo, len_hi, num]. A zero length means the
       controller shares one ACL pool with BR/EDR, in which case the
       classic count already covers LE. When it reports a separate pool
       the two counts are added: Number_Of_Completed_Packets returns
       completions from both, so one combined counter stays balanced. */
    if (bt_hci_command_sync_ret(HCI_OGF_LE, HCI_OCF_LE_READ_BUFFER_SIZE, NULL, 0,
            1000, buf_ret, sizeof(buf_ret), &buf_ret_len) == 0 &&
            buf_ret_len >= 3) {
        le_len = (uint16_t)((uint16_t)buf_ret[0] | ((uint16_t)buf_ret[1] << 8));
        le_num = buf_ret[2];
    }
    if (le_len != 0 && le_num != 0) {
        _acl_credits = (uint16_t)(_acl_credits + le_num);
    }

    _le_supported = true;
    bt_le_provision_own_addr();
    /* Cache the controller's local P-256 public key. If it comes back we
       support LE Secure Connections and the SMP layer offers the SC bit,
       which is what an Xbox pad (and any SC-only peripheral) requires to
       bond; a controller without it just leaves SC off and stays legacy. */
    (void)bt_le_read_local_p256();
    /* rebuild the resolving list from the bonds loaded at mount and turn on
       address resolution before any scan, so a peer that rotates a
       resolvable private address is reported by its stable identity address
       from the very first advertising report */
    bt_le_resolving_setup();
    return 0;
}

/* ---------- LE scanning ---------- */
static int bt_le_scan_enable(bool enable, bool filter_dup) {
    uint8_t params[2];
    int ret;

    if (!_le_supported) {
        return -1;
    }
    params[0] = enable ? 0x01 : 0x00;
    params[1] = filter_dup ? 0x01 : 0x00;
    ret = bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_SCAN_ENABLE, params,
            sizeof(params), 1000);
    if (ret == 0) {
        _le_scan_enabled = enable;
    }
    else if (enable) {
        slog("bluetooth le_scan_enable_failed status=%d\n", ret);
    }
    return ret;
}

static int bt_le_scan_params(void) {
    uint8_t params[7];

    params[0] = BT_LE_SCAN_TYPE_ACTIVE;
    params[1] = (uint8_t)(BT_LE_SCAN_INTERVAL & 0xff);
    params[2] = (uint8_t)(BT_LE_SCAN_INTERVAL >> 8);
    params[3] = (uint8_t)(BT_LE_SCAN_WINDOW & 0xff);
    params[4] = (uint8_t)(BT_LE_SCAN_WINDOW >> 8);
    params[5] = _local_addr_type; /* our own address type */
    params[6] = 0x00;             /* accept advertisements from anyone */
    return bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_SCAN_PARAMS, params,
            sizeof(params), 1000);
}

/* fire-and-forget twin of bt_le_scan_enable: no Command Complete wait, so
   a devcmd handler (xbt's stop/scan/power clicks) never stalls here. The
   controller executes commands in wire order, so a following
   LE_Create_Connection / Inquiry still sees scanning off. */
static int bt_le_scan_enable_async(bool enable, bool filter_dup) {
    uint8_t params[2];
    int ret;

    if (!_le_supported) {
        return -1;
    }
    params[0] = enable ? 0x01 : 0x00;
    params[1] = filter_dup ? 0x01 : 0x00;
    ret = bt_hci_send_command(HCI_OGF_LE, HCI_OCF_LE_SET_SCAN_ENABLE, params,
            sizeof(params));
    if (ret == 0) {
        _le_scan_enabled = enable;
    }
    return ret;
}

/* LE 5.0 extended scanning. Scanning_PHYs = LE 1M only (bit 0): every HID
   peripheral advertises on 1M, and adding the Coded PHY would double the
   per-PHY parameter block for no benefit here. One Scan_Type/Interval/Window
   triple follows for that single PHY. Filter policy 0x00 = accept all. */
static int bt_le_ext_scan_params(void) {
    uint8_t params[8];

    params[0] = _local_addr_type;      /* Own_Address_Type */
    params[1] = 0x00;                  /* Scanning_Filter_Policy: accept all */
    params[2] = 0x01;                  /* Scanning_PHYs: LE 1M */
    params[3] = BT_LE_SCAN_TYPE_ACTIVE; /* Scan_Type[1M]: active (get SCAN_RSP) */
    params[4] = (uint8_t)(BT_LE_SCAN_INTERVAL & 0xff);
    params[5] = (uint8_t)(BT_LE_SCAN_INTERVAL >> 8);
    params[6] = (uint8_t)(BT_LE_SCAN_WINDOW & 0xff);
    params[7] = (uint8_t)(BT_LE_SCAN_WINDOW >> 8);
    return bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_EXT_SCAN_PARAMS,
            params, sizeof(params), 1000);
}

/* Duration = 0 (no limit) and Period = 0 (scan continuously) keep the radio
   on until we explicitly disable it, matching the legacy scan's behaviour
   across a discovery slice. */
static int bt_le_ext_scan_enable(bool enable, bool filter_dup) {
    uint8_t params[6];
    int ret;

    params[0] = enable ? 0x01 : 0x00;
    params[1] = filter_dup ? 0x01 : 0x00;
    params[2] = 0x00; params[3] = 0x00; /* Duration: 0 = until disabled */
    params[4] = 0x00; params[5] = 0x00; /* Period: 0 = continuous */
    ret = bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_SET_EXT_SCAN_ENABLE,
            params, sizeof(params), 1000);
    if (ret == 0) {
        _le_scan_enabled = enable;
    }
    return ret;
}

/* fire-and-forget twin of bt_le_ext_scan_enable: see bt_le_scan_enable_async */
static int bt_le_ext_scan_enable_async(bool enable, bool filter_dup) {
    uint8_t params[6];
    int ret;

    params[0] = enable ? 0x01 : 0x00;
    params[1] = filter_dup ? 0x01 : 0x00;
    params[2] = 0x00; params[3] = 0x00; /* Duration: 0 = until disabled */
    params[4] = 0x00; params[5] = 0x00; /* Period: 0 = continuous */
    ret = bt_hci_send_command(HCI_OGF_LE, HCI_OCF_LE_SET_EXT_SCAN_ENABLE,
            params, sizeof(params));
    if (ret == 0) {
        _le_scan_enabled = enable;
    }
    return ret;
}

/* ---------- GAP advertising data ----------
   An advertising report is a run of length-type-value fields. The three
   we care about are the name (what the user picks from), the 16-bit
   service UUID list (0x1812 is the HID Service) and the appearance
   (category 15 is a HID device, 961 keyboard / 962 mouse). */
static void bt_le_parse_ad(const uint8_t* ad, size_t len, uint16_t* appearance,
        bool* adv_hid, char* name, size_t name_sz) {
    size_t i = 0;

    while (i + 1 < len) {
        uint8_t field_len = ad[i];
        uint8_t type;
        size_t data_len;
        size_t d;

        if (field_len == 0 || i + 1 + (size_t)field_len > len) {
            break;
        }
        type = ad[i + 1];
        data_len = (size_t)field_len - 1;

        if ((type == AD_TYPE_NAME_SHORT || type == AD_TYPE_NAME_COMPLETE) &&
                name != NULL && name_sz > 1 && name[0] == 0) {
            size_t copy = data_len >= name_sz ? name_sz - 1 : data_len;

            for (d = 0; d < copy; ++d) {
                unsigned char ch = ad[i + 2 + d];
                name[d] = isprint(ch) ? (char)ch : '.';
            }
            name[copy] = 0;
            bt_trim_name(name);
        }
        else if (type == AD_TYPE_APPEARANCE && data_len >= 2 &&
                appearance != NULL) {
            *appearance = (uint16_t)((uint16_t)ad[i + 2] |
                    ((uint16_t)ad[i + 3] << 8));
            if (adv_hid != NULL &&
                    (*appearance >> 6) == AD_APPEARANCE_CATEGORY_HID) {
                *adv_hid = true;
            }
        }
        else if ((type == AD_TYPE_UUID16_INCOMPLETE ||
                type == AD_TYPE_UUID16_COMPLETE) && adv_hid != NULL) {
            for (d = 0; d + 1 < data_len; d += 2) {
                uint16_t uuid = (uint16_t)((uint16_t)ad[i + 2 + d] |
                        ((uint16_t)ad[i + 2 + d + 1] << 8));
                if (uuid == GATT_SVC_HID) {
                    *adv_hid = true;
                }
            }
        }
        i += (size_t)field_len + 1;
    }
}

/* The table holds 32 entries and a busy RF neighbourhood advertises far
   more, so an LE slot gets recycled: the oldest entry that is not
   connected, is not a classic device, carries no bond and has no name
   goes first. Only when nothing qualifies is the name requirement
   dropped. */
static bt_device_t* bt_le_alloc_device(const uint8_t* addr) {
    bt_device_t* dev = bt_find_device(addr, true);
    int pass;

    if (dev != NULL) {
        return dev;
    }
    for (pass = 0; pass < 2; ++pass) {
        bt_device_t* victim = NULL;
        uint64_t oldest = 0;
        int i;

        for (i = 0; i < MAX_BT_DEVICES; ++i) {
            bt_device_t* d = &_devices[i];

            if (!d->used || d->connected || d->classic || !d->le) {
                continue;
            }
            if (d->has_ltk || d->has_link_key) {
                continue;
            }
            if (pass == 0 && d->name[0] != 0) {
                continue;
            }
            if (victim == NULL || d->last_seen_ms < oldest) {
                victim = d;
                oldest = d->last_seen_ms;
            }
        }
        if (victim == NULL) {
            continue;
        }
        memset(victim, 0, sizeof(*victim));
        victim->used = true;
        victim->le = true;
        memcpy(victim->addr, addr, 6);
        victim->rssi = 127;
        return victim;
    }
    return NULL;
}

/* Queue an LE bring-up for bt_le_step. Returns false when one is already
   running or queued - the caller reports connect_busy rather than
   silently replacing a request that is mid-flight. */
static bool bt_le_queue_request(const bt_device_t* dev, bool pair) {
    int slot;
    int i;

    if (dev == NULL || _le_req_active) {
        slog("bluetooth le_queue_reject reason=%s\n",
                dev == NULL ? "null_dev" : "req_active");
        return false;
    }
    /* already linked on some session: nothing new to bring up */
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.handle_valid &&
                bt_addr_equal(_les[i].le.addr, dev->addr)) {
            slog("bluetooth le_queue_reject reason=already_linked slot=%d\n", i);
            return false;
        }
    }
    /* need a free slot to connect into */
    slot = le_session_free();
    if (slot < 0) {
        slog("bluetooth le_queue_reject reason=no_free_slot\n");
        return false;
    }
    memcpy(_le_req_addr, dev->addr, 6);
    _le_req_pair = pair;
    _le_req_slot = slot;
    _le_req_ms = kernel_tic_ms(0);
    _le_req_active = true;
    return true;
}

/* Find an already-bonded LE device carrying this name, other than `exclude`.
   Used to recognise a rotating-address (NRPA) peripheral that reappeared under
   a fresh address during a scan, so the new entry can be folded back onto the
   bonded one instead of piling up a duplicate. Skips a device that is the
   target of an in-flight LE bring-up. */
static bt_device_t* bt_find_le_bonded_by_name(const char* name,
        const bt_device_t* exclude) {
    int i;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        bt_device_t* d = &_devices[i];
        int s;
        if (d == exclude || !d->used || !d->le || d->connected ||
                !d->has_ltk || d->name[0] == 0) {
            continue;
        }
        /* skip anything already linked on any session */
        for (s = 0; s < MAX_LE_SESSIONS; ++s) {
            if (_les[s].le.handle_valid && bt_addr_equal(_les[s].le.addr, d->addr)) {
                break;
            }
        }
        if (s < MAX_LE_SESSIONS) {
            continue;
        }
        if (strcmp(d->name, name) == 0) {
            return d;
        }
    }
    return NULL;
}

/* Shared admission logic for one advertising report, whatever transport it
   arrived on (legacy LE Advertising Report or LE Extended Advertising
   Report). ext_pdu marks a report that came from a non-legacy BLE 5.0
   extended PDU; such a peer can only be connected with LE_Extended_Create_
   Connection, so the flag is recorded on the device for the connect path. */
/* LE carries no Class-of-Device, but xbt's "type" column is derived from the
   CoD major class, so an LE HID peripheral would otherwise read "Unknown".
   Synthesize a Peripheral-major (0x05) CoD with the keyboard/pointing minor
   bits once we know what the device actually is. */
static uint32_t bt_le_synth_cod(bool is_mouse, bool is_kbd, bool is_gamepad) {
    if (is_gamepad) {
        return 0x002508; /* peripheral: gamepad */
    }
    if (is_mouse && is_kbd) {
        return 0x0025C0; /* peripheral: keyboard + pointing combo */
    }
    if (is_mouse) {
        return 0x002580; /* peripheral: pointing device */
    }
    if (is_kbd) {
        return 0x002540; /* peripheral: keyboard */
    }
    return 0x002500; /* peripheral: unspecified HID */
}

static void bt_le_admit_adv(const uint8_t* addr, uint8_t addr_type,
        const uint8_t* data, uint8_t data_len, int8_t rssi, bool ext_pdu) {
    uint16_t appearance = 0;
    bool adv_hid = false;
    char name[64];
    bt_device_t* dev;
    bool fresh;
    bool updated = false;

    name[0] = 0;
    bt_le_parse_ad(data, data_len, &appearance, &adv_hid, name, sizeof(name));

    /* A nameless report claiming neither the HID Service nor a HID
       appearance is a beacon or a phone. Admitting those would evict
       real peripherals from the table within seconds, so they are
       dropped unless we already know the address. */
    dev = bt_find_device(addr, false);
    fresh = dev == NULL;
    if (fresh) {
        if (!adv_hid && name[0] == 0) {
            return;
        }
        dev = bt_le_alloc_device(addr);
        if (dev == NULL) {
            return;
        }
    }
    dev->le = true;
    dev->addr_type = addr_type;
    dev->rssi = rssi;
    dev->last_seen_ms = kernel_tic_ms(0);
    if (ext_pdu) {
        dev->ext_adv = true;
    }
    if (appearance != 0) {
        dev->appearance = appearance;
        if (dev->class_of_device == 0 &&
                (appearance >> 6) == AD_APPEARANCE_CATEGORY_HID) {
            uint8_t sub = (uint8_t)(appearance & 0x3f);
            dev->class_of_device = bt_le_synth_cod(sub == 2, sub == 1, false);
        }
    }
    if (adv_hid) {
        dev->adv_hid = true;
    }
    if (name[0] != 0 && dev->name[0] == 0) {
        strncpy(dev->name, name, sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = 0;
        bt_trim_name(dev->name);
        updated = true;
    }
    /* A bonded NRPA peripheral reappears under a fresh address every scan. Now
       that its name is known, fold this new entry back onto the bonded one:
       move the bond onto the address currently on air and drop the duplicate,
       so xbt keeps showing a single device instead of one per rotation. */
    if (updated && dev->name[0] != 0 && !dev->connected && !dev->has_ltk) {
        bt_device_t* b = bt_find_le_bonded_by_name(dev->name, dev);
        if (b != NULL) {
            bt_known_t* k;
            memcpy(b->addr, dev->addr, 6);
            b->addr_type = dev->addr_type;
            b->rssi = dev->rssi;
            b->last_seen_ms = dev->last_seen_ms;
            if (ext_pdu) {
                b->ext_adv = true;
            }
            k = bt_known_find_le_by_name(b->name);
            if (k != NULL) {
                memcpy(k->addr, b->addr, 6);
                k->addr_type = b->addr_type;
            }
            memset(dev, 0, sizeof(*dev));
            bt_emit_device_line("device", b);
            return;
        }
    }
    /* a device we already hold - seeded from the bond store at boot, or
       seen in an ADV_IND before its SCAN_RSP - still has to be
       re-announced once the name turns up, or xbt keeps showing the
       bare address */
    if (fresh || updated) {
        bt_emit_device_line("device", dev);
    }

    /* a bonded peripheral that just showed up reconnects by itself;
       has_ltk already means we paired with it once, so it is a HID
       device whatever this particular advertisement happens to carry */
    if (_le_autoconnect && dev->has_ltk && !dev->connected) {
        char addr_str[24];

        if (bt_le_queue_request(dev, false)) {
            bt_addr_to_str(dev->addr, addr_str, sizeof(addr_str));
            bt_emit("le_autoconnect %s\n", addr_str);
            _le_autoconnect = false;
        }
    }
}

/* LE Advertising Report: Num_Reports(1), then per report Event_Type(1)
   Address_Type(1) Address(6) Data_Length(1) Data(n) RSSI(1). */
static void bt_le_handle_adv_report(const uint8_t* p, size_t len) {
    uint8_t n;
    size_t off = 1;
    uint8_t i;

    if (len < 1) {
        return;
    }
    n = p[0];
    for (i = 0; i < n; ++i) {
        uint8_t addr[6];
        uint8_t addr_type;
        uint8_t data_len;
        const uint8_t* data;
        int8_t rssi;

        if (off + 9 > len) {
            return;
        }
        addr_type = p[off + 1];
        memcpy(addr, p + off + 2, 6);
        data_len = p[off + 8];
        if (off + 10 + (size_t)data_len > len) {
            return;
        }
        data = p + off + 9;
        rssi = (int8_t)p[off + 9 + data_len];
        off += 10 + (size_t)data_len;

        /* a legacy report is never an extended PDU */
        bt_le_admit_adv(addr, addr_type, data, data_len, rssi, false);
    }
}

/* LE Extended Advertising Report (subevent 0x0d): Num_Reports(1), then per
   report a fixed 24-octet header followed by Data:
     Event_Type(2) Address_Type(1) Address(6) Primary_PHY(1) Secondary_PHY(1)
     Advertising_SID(1) TX_Power(1) RSSI(1) Periodic_Adv_Interval(2)
     Direct_Address_Type(1) Direct_Address(6) Data_Length(1) Data(n)
   Event_Type bit 4 marks a legacy PDU (a pre-5.0 advertiser relayed through
   the extended report); such a peer stays reachable with the legacy create
   connection, so only a clear bit 4 flags dev->ext_adv. */
static void bt_le_handle_ext_adv_report(const uint8_t* p, size_t len) {
    uint8_t n;
    size_t off = 1;
    uint8_t i;

    if (len < 1) {
        return;
    }
    n = p[0];
    for (i = 0; i < n; ++i) {
        uint8_t addr[6];
        uint8_t addr_type;
        uint8_t data_len;
        const uint8_t* data;
        int8_t rssi;
        uint16_t evt_type;
        bool legacy_pdu;

        if (off + 24 > len) {
            return;
        }
        evt_type = (uint16_t)((uint16_t)p[off] | ((uint16_t)p[off + 1] << 8));
        addr_type = p[off + 2];
        memcpy(addr, p + off + 3, 6);
        rssi = (int8_t)p[off + 13];
        data_len = p[off + 23];
        if (off + 24 + (size_t)data_len > len) {
            return;
        }
        data = p + off + 24;
        off += 24 + (size_t)data_len;

        /* 0xFF = anonymous advertisement: nothing to key a device on */
        if (addr_type == 0xFF) {
            continue;
        }
        legacy_pdu = (evt_type & 0x0010) != 0;
        bt_le_admit_adv(addr, addr_type, data, data_len, rssi, !legacy_pdu);
    }
}

static uint16_t att_le16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ---------- LE meta events ----------
   LE Connection Complete is 18 octets, LE Enhanced Connection Complete 30
   (it adds the two resolvable private addresses actually used on air).
   We never fill the controller's resolving list, so no address
   resolution happens and Peer_Address is the on-air address SMP's c1 has
   to hash - the RPAs are logged and otherwise ignored. */
static void bt_le_handle_conn_complete(const uint8_t* p, size_t len,
        bool enhanced) {
    uint16_t handle;
    uint16_t interval;
    bt_device_t* dev;
    char addr_str[24];
    int saved = _le_cur;
    int i;

    if (len < (enhanced ? 30u : 18u)) {
        return;
    }
    handle = (uint16_t)(att_le16(p + 1) & 0x0fff);
    interval = att_le16(enhanced ? p + 23 : p + 11);

    /* the completion belongs to whichever session is mid-connect; at most one
       session is in LE_ST_CONNECTING because bring-ups are serialized */
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_CONNECTING) {
            _le_cur = i;
            break;
        }
    }

    if (p[0] != 0) {
        _le.state = LE_ST_FAILED;
        _le.deadline_ms = kernel_tic_ms(0) + 3000;
        slog("bluetooth le_conn_failed status=0x%02x\n", p[0]);
        _le_cur = saved;
        return;
    }

    _le.handle = handle;
    _le.handle_valid = true;
    _le.addr_type = p[4];
    memcpy(_le.addr, p + 5, 6);
    _le.encrypted = false;
    if (_le.state != LE_ST_FAILED) {
        _le.state = LE_ST_LINK_UP;
    }

    bt_addr_to_str(_le.addr, addr_str, sizeof(addr_str));

    dev = bt_find_device(_le.addr, true);
    if (dev != NULL) {
        dev->le = true;
        dev->addr_type = _le.addr_type;
        dev->connected = true;
        dev->handle = handle;
        dev->last_seen_ms = kernel_tic_ms(0);
        bt_emit("le_connected %s handle=0x%04X interval=%u\n",
            addr_str, handle, interval);
    }

    _le_cur = saved;
}

static void bt_le_handle_conn_update(const uint8_t* p, size_t len) {
    if (len < 9) {
        return;
    }
    if (p[0] != 0) {
        slog("bluetooth le_conn_update_failed status=0x%02x handle=0x%04x\n",
            p[0], att_le16(p + 1) & 0x0fff);
    }
}

/* LE Long Term Key Request: the peer's Link Layer wants to re-encrypt and
   names the bond by EDIV + Rand. Answer with the stored LTK when it
   matches, otherwise refuse so SMP runs instead. */
static void bt_le_handle_ltk_request(const uint8_t* p, size_t len) {
    uint8_t params[18];
    uint16_t handle;
    uint16_t ediv;
    bt_device_t* dev;

    if (len < 12) {
        return;
    }
    handle = (uint16_t)(att_le16(p) & 0x0fff);
    ediv = att_le16(p + 10);
    params[0] = (uint8_t)(handle & 0xff);
    params[1] = (uint8_t)(handle >> 8);

    /* answer on behalf of the session that owns this link, so _le.addr and
       _le.encrypted below refer to the right device */
    int si = le_session_by_handle(handle);
    int saved = _le_cur;
    if (si >= 0) {
        _le_cur = si;
    }

    /* the handle is the reliable key here: _le.addr is only guaranteed to
       be filled while a bring-up we started is running */
    dev = bt_find_device_by_handle(handle);
    if (dev == NULL) {
        dev = bt_find_device(_le.addr, false);
    }
    if (dev != NULL && dev->has_ltk && dev->ediv == ediv &&
            memcmp(dev->ltk_rand, p + 2, 8) == 0) {
        memcpy(params + 2, dev->ltk, 16);
        _le.encrypted = false;
        (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_LTK_REQ_REPLY, params,
                sizeof(params), 1000);
        _le_cur = saved;
        return;
    }

    /* A peer that rotates an unresolvable private address (NRPA, top two bits
       00) reconnects from a fresh address that carries no LTK, and an NRPA
       cannot be resolved by an IRK, so the address-keyed lookup above misses.
       The key material itself is stable, though: recognise the stored bond by
       its EDIV + randomizer, answer with its LTK (so the peer encrypts instead
       of re-pairing), then rebind the bond onto the address we are talking to
       now and drop the stale device entry seeded from the previous rotation -
       otherwise one mouse piles up a new list entry every time it rotates. */
    {
        bt_known_t* k = bt_known_find_by_ltk(ediv, p + 2);
        if (k != NULL) {
            bt_device_t* cur = (dev != NULL) ? dev
                    : bt_find_device(_le.addr, false);
            memcpy(params + 2, k->ltk, 16);
            _le.encrypted = false;
            (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_LTK_REQ_REPLY,
                    params, sizeof(params), 1000);
            if (cur != NULL && !bt_addr_equal(cur->addr, k->addr)) {
                bt_device_t* stale = bt_find_device(k->addr, false);
                if (stale != NULL && !stale->connected) {
                    stale->used = false;
                }
                memcpy(k->addr, cur->addr, 6);
                k->addr_type = cur->addr_type;
                cur->le = true;
                cur->has_ltk = true;
                memcpy(cur->ltk, k->ltk, 16);
                cur->ediv = k->ediv;
                memcpy(cur->ltk_rand, k->ltk_rand, 8);
                if (cur->name[0] == 0 && k->name[0] != 0) {
                    strncpy(cur->name, k->name, sizeof(cur->name) - 1);
                    cur->name[sizeof(cur->name) - 1] = 0;
                }
                bt_known_save();
            }
            _le_cur = saved;
            return;
        }
    }

    (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_LTK_REQ_NEG_REPLY, params,
            2, 1000);
    slog("bluetooth le_ltk_neg_reply handle=0x%04x ediv=0x%04x\n", handle, ediv);
    _le_cur = saved;
}

void bt_handle_le_meta(const uint8_t* payload, size_t len) {
    if (len < 1) {
        return;
    }
    switch (payload[0]) {
    case LE_EVT_CONN_COMPLETE:
        bt_le_handle_conn_complete(payload + 1, len - 1, false);
        break;
    case LE_EVT_ENHANCED_CONN_COMPLETE:
        bt_le_handle_conn_complete(payload + 1, len - 1, true);
        break;
    case LE_EVT_ADV_REPORT:
        bt_le_handle_adv_report(payload + 1, len - 1);
        break;
    case LE_EVT_EXT_ADV_REPORT:
        bt_le_handle_ext_adv_report(payload + 1, len - 1);
        break;
    case LE_EVT_CONN_UPDATE:
        bt_le_handle_conn_update(payload + 1, len - 1);
        break;
    case LE_EVT_LTK_REQUEST:
        bt_le_handle_ltk_request(payload + 1, len - 1);
        break;
    case LE_EVT_READ_LOCAL_P256:
        bt_le_handle_local_p256(payload + 1, len - 1);
        break;
    case LE_EVT_GENERATE_DHKEY:
        bt_le_handle_dhkey(payload + 1, len - 1);
        break;
    /* remote feature reads and periodic-advertising reports are not part
       of the bring-up; the extended advertising report (0x0d) is handled
       above whenever the controller accepted extended scanning */
    default:
        break;
    }
}

void bt_handle_encryption_change(const uint8_t* payload, size_t len) {
    uint16_t handle;
    bt_device_t* dev;
    int si;

    if (len < 4) {
        return;
    }
    handle = (uint16_t)(att_le16(payload + 1) & 0x0fff);

    /* LE link: route to the session that owns this handle so the _le/_smp
       macros land on the right one, whichever link just encrypted */
    si = le_session_by_handle(handle);
    if (si >= 0) {
        int saved = _le_cur;
        _le_cur = si;
        if (payload[0] == 0 && payload[3] != 0) {
            _le.encrypted = true;
            _smp.enc_changed = true;
        }
        else {
            _le.encrypted = false;
            _smp.enc_changed = true;
            slog("bluetooth le_encrypt_failed handle=0x%04x status=0x%02x\n",
                handle, payload[0]);
        }
        _le_cur = saved;
        return;
    }

    /* classic (BR/EDR) link: HID bring-up was deferred until the link is
       secured. Whether encryption ended up enabled or was refused, the link
       is authenticated now, so open the L2CAP HID channels - unless HOGP is
       already streaming reports from the same peripheral over LE, in which
       case classic channels would be refused and the radio contention would
       kill the LE link. */
    if (_sec_encrypt_handle == handle) {
        _sec_encrypt_handle = 0;
    }
    dev = bt_find_device_by_handle(handle);
    if (dev != NULL && dev->hid_after_sec) {
        dev->hid_after_sec = false;
        if (!bt_hogp_blocks_classic(dev)) {
            bt_hid_start(handle, dev->addr);
        }
    }
}

/* ---------- ATT client (fixed CID 0x0004) ---------- */
static int att_send(uint16_t handle, const uint8_t* pdu, uint16_t len) {
    return l2cap_send_pdu(handle, L2CAP_CID_ATT, pdu, len);
}

static bool att_rsp_pred(void* ctx) {
    (void)ctx;
    return _att.rsp_ready;
}

/* Exactly one request is outstanding at a time: the matching response, an
   Error Response or the deadline ends the wait. The caller reads _att.rsp
   / _att.rsp_len straight after a zero return. */
static int att_request(uint16_t handle, const uint8_t* pdu, uint16_t len,
        uint8_t expect_op, uint32_t timeout_ms) {
    if (len == 0 || pdu == NULL) {
        return -1;
    }
    memset(&_att, 0, sizeof(_att));
    _att.busy = true;
    _att.req_opcode = pdu[0];
    if (att_send(handle, pdu, len) != 0) {
        _att.busy = false;
        slog("bluetooth att_send_failed req=0x%02x\n", pdu[0]);
        return -1;
    }
    if (!bt_poll_until(att_rsp_pred, NULL, timeout_ms)) {
        _att.busy = false;
        slog("bluetooth att_timeout req=0x%02x\n", pdu[0]);
        return -1;
    }
    _att.busy = false;
    if (_att.err) {
        slog("bluetooth att_error req=0x%02x attr=0x%04x code=0x%02x\n",
            pdu[0], _att.err_handle, _att.err_code);
        return -1;
    }
    if (expect_op != 0 && _att.rsp_opcode != expect_op) {
        slog("bluetooth att_unexpected req=0x%02x rsp=0x%02x\n",
            pdu[0], _att.rsp_opcode);
        return -1;
    }
    return 0;
}

static hogp_attr_t* hogp_find(uint16_t uuid) {
    int i;

    for (i = 0; i < _hogp.n_attrs; ++i) {
        if (_hogp.attrs[i].uuid == uuid) {
            return &_hogp.attrs[i];
        }
    }
    return NULL;
}

/* One input report arrived as a notification or indication. Which
   characteristic it came from decides everything: under Boot Protocol the
   layout is fixed by the HID spec, under Report Protocol the Report Map
   bit layout in h->mouse does the decoding. The bytes handed to the
   subscribers are the same ones a USB or classic-Bluetooth HID device
   produces, because they come out of libhid's normalizer.

   `si` is the session that owns the link the notification arrived on; its
   HOGP state (attributes, mouse parser) is used instead of the global
   _hogp macro, since a second BLE device can stream while _le_cur points
   at the session currently being set up. */
static void bt_le_handle_notify(int si, uint16_t value_handle,
        const uint8_t* value, size_t len) {
    hogp_state_t* h = &_les[si].hogp;
    const hogp_attr_t* a = NULL;
    uint8_t evt[HID_MAX_EVENT_SIZE];
    int i;

    for (i = 0; i < h->n_attrs; ++i) {
        if (h->attrs[i].value_handle == value_handle) {
            a = &h->attrs[i];
            break;
        }
    }

    if (a == NULL || len == 0) {
        return;
    }

    if (a->uuid == GATT_CHR_BOOT_KBD_INPUT) {
        if (len < HID_KEYBOARD_REPORT_SIZE) {
            return;
        }
        memset(evt, 0, sizeof(evt));
        memcpy(evt, value, HID_KEYBOARD_REPORT_SIZE);
        bt_hid_dispatch_keyboard(evt);
        return;
    }
    if (a->uuid == GATT_CHR_BOOT_MOUSE_INPUT) {
        if (len < HID_MOUSE_REPORT_SIZE) {
            return;
        }
        memset(evt, 0, sizeof(evt));
        evt[0] = value[0]; /* buttons */
        evt[1] = value[1]; /* dx */
        evt[2] = value[2]; /* dy */
        if (len >= 4) {
            evt[3] = value[3]; /* wheel */
        }
        bt_hid_dispatch_mouse(evt);
        return;
    }
    if (a->uuid == GATT_CHR_REPORT) {
        /* A LE gamepad's Report characteristic carries the descriptor-driven
           joystick layout. HOGP omits the Report ID octet from the value (it
           lives in the Report Reference descriptor) while the parser's bit
           offsets include it, so rebuild [id || body] before normalizing -
           exactly like the mouse path below. The normalized js_evt_t is
           forwarded under the reserved 0x05 tag that hid_joystickd's BT mode
           passes straight through. */
        if (h->is_gamepad && h->joystick_ok &&
                (!a->has_report_ref || h->joystick_report_id == 0 ||
                 a->report_id == h->joystick_report_id)) {
            uint8_t jbuf[65];
            const uint8_t* jp = value;
            int jlen = (int)len;
            js_evt_t je;
            uint8_t frame[1 + sizeof(js_evt_t)];

            if (h->joystick.has_report_id &&
                    jlen == h->joystick.report_bytes - 1 &&
                    jlen + 1 <= (int)sizeof(jbuf)) {
                jbuf[0] = h->joystick.report_id;
                memcpy(jbuf + 1, value, (size_t)jlen);
                jp = jbuf;
                jlen += 1;
            }
            if (joystick_normalize_report(&h->joystick, jp, jlen, &je) == 0) {
                frame[0] = 0x05; /* normalized js_evt_t follows */
                memcpy(frame + 1, &je, sizeof(js_evt_t));
                bt_hid_dispatch_joystick(frame, sizeof(frame));
                return;
            }
            /* normalize bailed: fall through to the heuristics below */
        }

        /* Classify the Report characteristic by the Report Map's own
           collection IDs, exactly like usbhostd routes a composite device:
           a keyboard body shorter than 8 octets (the WiWU sends 6) is
           indistinguishable from a mouse by length, and the old length
           heuristic dispatched every such keystroke as a pointer move. */
        bool is_mouse = h->mouse_ok && a->has_report_ref &&
                a->report_id == h->mouse.report_id;
        bool is_kbd = a->has_report_ref && h->kbd_report_id != 0 &&
                a->report_id == h->kbd_report_id;
        uint8_t rbuf[65];
        const uint8_t* rp = value;
        int rlen = (int)len;

        if (is_mouse) {
            /* A HOGP Report characteristic value OMITS the Report ID octet
               (the id lives in the Report Reference descriptor), but the
               parser's bit offsets include it (report_bits[id] starts at 8)
               and mouse_normalize_report validates report[0] == report_id and
               len >= report_bytes. Prepend the id so a report-id mouse decodes
               through the real layout. */
            if (h->mouse.has_report_id &&
                    rlen == h->mouse.report_bytes - 1 &&
                    rlen + 1 <= (int)sizeof(rbuf)) {
                rbuf[0] = h->mouse.report_id;
                memcpy(rbuf + 1, value, (size_t)rlen);
                rp = rbuf;
                rlen += 1;
            }
            if (mouse_normalize_report(&h->mouse, rp, rlen, evt) ==
                    HID_POINTER_EVENT_SIZE) {
                bt_hid_dispatch_mouse(evt);
                return;
            }
            /* normalize bailed: fall through to the length heuristic below */
        }

        if (is_kbd) {
            /* The value omits the Report ID, so the body is already the boot
               keyboard layout [modifiers, reserved, key...]; copy it into the
               fixed 8-octet event, zero-padding a short body. Some peripherals
               include the ID octet anyway - strip a leading byte equal to the
               known Report ID so a 9-octet value still aligns. */
            const uint8_t* kp = value;
            size_t klen = len;
            if (klen == HID_KEYBOARD_REPORT_SIZE + 1 && kp[0] == a->report_id) {
                kp++;
                klen--;
            }
            memset(evt, 0, sizeof(evt));
            memcpy(evt, kp, klen < HID_KEYBOARD_REPORT_SIZE ?
                    klen : HID_KEYBOARD_REPORT_SIZE);
            bt_hid_dispatch_keyboard(evt);
            return;
        }

        /* Neither a known mouse nor a known keyboard collection (or the Report
           Reference was never discovered): fall back to the length heuristic,
           exactly like the classic HIDP path in l2cap_chan_data, including the
           defensive 9-octet Report-ID strip - otherwise an un-stripped 0x01
           Report ID is misread as LCTRL and every letter turns into a control
           code while digits/symbols pass through. */
        {
            const uint8_t* kdata = value;
            size_t klen = len;
            if (klen == HID_KEYBOARD_REPORT_SIZE + 1 &&
                    (!a->has_report_ref || a->report_id == 0 ||
                     kdata[0] == a->report_id)) {
                kdata++;
                klen--;
            }
            bt_hid_handle_report(kdata, klen);
        }
    }
}

static void att_handle_rx(uint16_t handle, const uint8_t* pdu, size_t len) {
    uint16_t n;

    if (len < 1) {
        return;
    }

    /* Notifications and indications can arrive on ANY active LE link,
       including one that is not the session currently being set up, so they
       are routed purely by connection handle and never touch _le_cur. */
    if (pdu[0] == ATT_OP_HANDLE_NOTIFY || pdu[0] == ATT_OP_HANDLE_IND) {
        int si;
        if (len < 3) {
            return;
        }
        if (pdu[0] == ATT_OP_HANDLE_IND) {
            /* an unconfirmed indication stalls the peer's whole queue */
            uint8_t conf = ATT_OP_IND_CONFIRM;
            (void)att_send(handle, &conf, 1);
        }
        si = le_session_by_handle(handle);
        if (si >= 0) {
            bt_le_handle_notify(si, att_le16(pdu + 1), pdu + 3, len - 3);
        }
        return;
    }

    /* everything else is a response to a request the current session sent,
       so it must belong to that session's link */
    if (!_le.handle_valid || handle != _le.handle) {
        return;
    }

    switch (pdu[0]) {
    case ATT_OP_ERROR_RSP:
        if (len < 5) {
            return;
        }
        _att.err = true;
        _att.err_handle = att_le16(pdu + 2);
        _att.err_code = pdu[4];
        _att.rsp_ready = true;
        return;
    default:
        break;
    }

    n = (uint16_t)(len > sizeof(_att.rsp) ? sizeof(_att.rsp) : len);
    memcpy(_att.rsp, pdu, n);
    _att.rsp_len = n;
    _att.rsp_opcode = pdu[0];
    _att.err = false;
    if (pdu[0] == ATT_OP_MTU_RSP && len >= 3) {
        /* ATT_MTU is the smaller of the two sides' RX capabilities */
        uint16_t m = att_le16(pdu + 1);
        if (m >= L2CAP_LE_MTU_DEFAULT && m < _hogp.mtu) {
            _hogp.mtu = m;
        }
    }
    _att.rsp_ready = true;
}

/* ---------- GATT discovery ---------- */
static int gatt_exchange_mtu(uint16_t handle) {
    uint8_t pdu[3];

    _hogp.mtu = BT_LE_ATT_MTU_PREFERRED;
    pdu[0] = ATT_OP_MTU_REQ;
    pdu[1] = (uint8_t)(BT_LE_ATT_MTU_PREFERRED & 0xff);
    pdu[2] = (uint8_t)(BT_LE_ATT_MTU_PREFERRED >> 8);
    if (att_request(handle, pdu, sizeof(pdu), ATT_OP_MTU_RSP,
            BT_LE_ATT_TIMEOUT_MS) != 0) {
        _hogp.mtu = L2CAP_LE_MTU_DEFAULT;
        return -1;
    }
    return 0;
}

/* A value longer than ATT_MTU-1 arrives as a Read Response followed by
   Read Blob Responses, each starting one octet further in; a response
   shorter than a full chunk ends the value. */
static int gatt_read_value(uint16_t handle, uint16_t attr, uint8_t* out,
        uint16_t cap, uint16_t* got) {
    uint16_t pos = 0;
    uint16_t chunk = _hogp.mtu > 1 ? (uint16_t)(_hogp.mtu - 1) : 22;
    uint8_t pdu[5];
    bool first = true;

    if (got != NULL) {
        *got = 0;
    }
    for (;;) {
        uint16_t raw;
        uint16_t n;

        if (first) {
            pdu[0] = ATT_OP_READ_REQ;
            pdu[1] = (uint8_t)(attr & 0xff);
            pdu[2] = (uint8_t)(attr >> 8);
            if (att_request(handle, pdu, 3, ATT_OP_READ_RSP,
                    BT_LE_ATT_TIMEOUT_MS) != 0) {
                break;
            }
            first = false;
        }
        else {
            pdu[0] = ATT_OP_READ_BLOB_REQ;
            pdu[1] = (uint8_t)(attr & 0xff);
            pdu[2] = (uint8_t)(attr >> 8);
            pdu[3] = (uint8_t)(pos & 0xff);
            pdu[4] = (uint8_t)(pos >> 8);
            /* Attribute Not Long / Attribute Not Found both mean "that was
               the whole value" rather than a failure */
            if (att_request(handle, pdu, 5, ATT_OP_READ_BLOB_RSP,
                    BT_LE_ATT_TIMEOUT_MS) != 0) {
                break;
            }
        }
        raw = _att.rsp_len > 1 ? (uint16_t)(_att.rsp_len - 1) : 0;
        if (raw == 0) {
            break;
        }
        n = raw;
        if ((uint16_t)(pos + n) > cap) {
            n = (uint16_t)(cap - pos);
        }
        if (n > 0) {
            memcpy(out + pos, _att.rsp + 1, n);
            pos = (uint16_t)(pos + n);
        }
        if (raw < chunk || pos >= cap) {
            break;
        }
    }
    if (got != NULL) {
        *got = pos;
    }
    return pos > 0 ? 0 : -1;
}

static int gatt_write_value(uint16_t handle, uint16_t attr, const uint8_t* v,
        uint16_t n) {
    uint8_t pdu[3 + 64];

    if (n > 64) {
        return -1;
    }
    pdu[0] = ATT_OP_WRITE_REQ;
    pdu[1] = (uint8_t)(attr & 0xff);
    pdu[2] = (uint8_t)(attr >> 8);
    if (n > 0) {
        memcpy(pdu + 3, v, n);
    }
    return att_request(handle, pdu, (uint16_t)(3 + n), ATT_OP_WRITE_RSP,
            BT_LE_ATT_TIMEOUT_MS);
}

static int gatt_write_cccd(uint16_t handle, uint16_t cccd, uint16_t value) {
    uint8_t v[2];

    v[0] = (uint8_t)(value & 0xff);
    v[1] = (uint8_t)(value >> 8);
    return gatt_write_value(handle, cccd, v, 2);
}

/* Read By Group Type over the whole attribute range, looking for the
   0x1812 primary service. An Attribute Not Found error is the normal end
   of the walk, so the loop breaks rather than reporting failure. */
static int gatt_find_hid_service(uint16_t handle) {
    uint8_t pdu[7];
    uint16_t start = 0x0001;

    _hogp.svc_start = 0;
    _hogp.svc_end = 0;

    while (start != 0 && start < 0xffff) {
        uint8_t item;
        uint16_t next = 0;
        size_t n;
        size_t i;

        pdu[0] = ATT_OP_READ_BY_GROUP_REQ;
        pdu[1] = (uint8_t)(start & 0xff);
        pdu[2] = (uint8_t)(start >> 8);
        pdu[3] = 0xff;
        pdu[4] = 0xff;
        pdu[5] = (uint8_t)(GATT_UUID_PRIMARY_SERVICE & 0xff);
        pdu[6] = (uint8_t)(GATT_UUID_PRIMARY_SERVICE >> 8);
        if (att_request(handle, pdu, sizeof(pdu), ATT_OP_READ_BY_GROUP_RSP,
                BT_LE_ATT_TIMEOUT_MS) != 0) {
            break;
        }
        if (_att.rsp_len < 3) {
            break;
        }
        item = _att.rsp[1];
        /* 6 = 16-bit UUID, 20 = 128-bit; the HID Service is a 16-bit one */
        if (item != 6) {
            break;
        }
        n = (_att.rsp_len - 2) / item;
        for (i = 0; i < n; ++i) {
            const uint8_t* e = _att.rsp + 2 + i * item;
            uint16_t group_end = att_le16(e + 2);

            next = (uint16_t)(group_end + 1);
            if (att_le16(e + 4) == GATT_SVC_HID) {
                _hogp.svc_start = att_le16(e);
                _hogp.svc_end = group_end;
                return 0;
            }
        }
        if (next == 0 || next <= start) {
            break;
        }
        start = next;
    }
    return _hogp.svc_start != 0 ? 0 : -1;
}

/* Read By Type for the 0x2803 characteristic declarations inside the HID
   Service. Each item is [decl handle, props, value handle, uuid]. */
static int gatt_discover_chars(uint16_t handle) {
    uint8_t pdu[7];
    uint16_t start = _hogp.svc_start;

    _hogp.n_attrs = 0;
    if (start == 0 || _hogp.svc_end == 0) {
        return -1;
    }
    while (start != 0 && start <= _hogp.svc_end) {
        uint8_t item;
        uint16_t next = 0;
        size_t n;
        size_t i;

        pdu[0] = ATT_OP_READ_BY_TYPE_REQ;
        pdu[1] = (uint8_t)(start & 0xff);
        pdu[2] = (uint8_t)(start >> 8);
        pdu[3] = (uint8_t)(_hogp.svc_end & 0xff);
        pdu[4] = (uint8_t)(_hogp.svc_end >> 8);
        pdu[5] = (uint8_t)(GATT_UUID_CHARACTERISTIC & 0xff);
        pdu[6] = (uint8_t)(GATT_UUID_CHARACTERISTIC >> 8);
        if (att_request(handle, pdu, sizeof(pdu), ATT_OP_READ_BY_TYPE_RSP,
                BT_LE_ATT_TIMEOUT_MS) != 0) {
            break;
        }
        if (_att.rsp_len < 3) {
            break;
        }
        item = _att.rsp[1];
        if (item != 7) {
            break; /* a 128-bit UUID declaration is nothing we handle */
        }
        n = (_att.rsp_len - 2) / item;
        for (i = 0; i < n; ++i) {
            const uint8_t* e = _att.rsp + 2 + i * item;

            next = (uint16_t)(att_le16(e) + 1);
            if (_hogp.n_attrs >= BT_MAX_HID_ATTRS) {
                continue;
            }
            {
                hogp_attr_t* a = &_hogp.attrs[_hogp.n_attrs++];

                memset(a, 0, sizeof(*a));
                a->decl_handle = att_le16(e);
                a->props = e[2];
                a->value_handle = att_le16(e + 3);
                a->uuid = att_le16(e + 5);
            }
        }
        if (next == 0 || next <= start) {
            break;
        }
        start = next;
    }
    return _hogp.n_attrs > 0 ? 0 : -1;
}

/* Find Information over the descriptors of one characteristic: the range
   runs from just past its value to just before the next declaration. The
   CCCD is what makes notifications arrive, the Report Reference is what
   ties a Report characteristic to a report id and direction. */
static void gatt_discover_char_descriptors(uint16_t handle, hogp_attr_t* a) {
    uint16_t start = (uint16_t)(a->value_handle + 1);
    uint16_t end = _hogp.svc_end;
    int j;

    if (a->value_handle == 0 || start == 0) {
        return;
    }
    for (j = 0; j < _hogp.n_attrs; ++j) {
        uint16_t d = _hogp.attrs[j].decl_handle;

        if (d > a->decl_handle && (uint16_t)(d - 1) < end) {
            end = (uint16_t)(d - 1);
        }
    }
    if (start > end) {
        return;
    }

    while (start != 0 && start <= end) {
        uint8_t pdu[5];
        uint8_t fmt;
        size_t item;
        size_t n;
        size_t k;
        uint16_t next = 0;

        pdu[0] = ATT_OP_FIND_INFO_REQ;
        pdu[1] = (uint8_t)(start & 0xff);
        pdu[2] = (uint8_t)(start >> 8);
        pdu[3] = (uint8_t)(end & 0xff);
        pdu[4] = (uint8_t)(end >> 8);
        if (att_request(handle, pdu, sizeof(pdu), ATT_OP_FIND_INFO_RSP,
                BT_LE_ATT_TIMEOUT_MS) != 0) {
            return;
        }
        if (_att.rsp_len < 2) {
            return;
        }
        fmt = _att.rsp[1];
        item = fmt == 0x01 ? 4 : 18; /* 0x01 = 16-bit UUIDs, 0x02 = 128-bit */
        n = (_att.rsp_len - 2) / item;
        for (k = 0; k < n; ++k) {
            const uint8_t* e = _att.rsp + 2 + k * item;
            uint16_t dh = att_le16(e);
            uint16_t uuid = fmt == 0x01 ? att_le16(e + 2) : 0;

            next = (uint16_t)(dh + 1);
            if (uuid == GATT_UUID_CLIENT_CHAR_CFG) {
                a->cccd_handle = dh;
            }
            else if (uuid == GATT_UUID_REPORT_REFERENCE) {
                uint8_t ref[4];
                uint16_t got = 0;

                if (gatt_read_value(handle, dh, ref, sizeof(ref), &got) == 0 &&
                        got >= 2) {
                    a->has_report_ref = true;
                    a->report_id = ref[0];
                    a->report_type = ref[1];
                }
            }
        }
        if (next == 0 || next <= start) {
            return;
        }
        start = next;
    }
}

static int gatt_discover_descriptors(uint16_t handle) {
    int i;

    for (i = 0; i < _hogp.n_attrs; ++i) {
        gatt_discover_char_descriptors(handle, &_hogp.attrs[i]);
    }
    return 0;
}

/* ---- LE bring-up wait predicates ---------------------------------------
   A bring-up blocks on controller events, so every predicate also
   releases on LE_ST_FAILED: the failure path must never leave a caller
   waiting out a full timeout. */

static bool le_link_up_pred(void* ctx) {
    (void)ctx;
    return _le.handle_valid || _le.state == LE_ST_FAILED;
}

static bool le_encrypted_pred(void* ctx) {
    (void)ctx;
    /* enc_changed releases the wait on a refusal too, so a rejected key
       costs one event round trip instead of the whole timeout */
    return _le.encrypted || _smp.enc_changed || _le.state == LE_ST_FAILED;
}

/* ---- discovery slicing --------------------------------------------------
   bt_start_scan only arms a session; these primitives move the radio
   between LE scanning and classic inquiry and bt_le_step calls them when
   a slice expires. None of them touch _scanning or the session deadline,
   so a bring-up can suspend discovery and let the same session resume
   afterwards without the two mechanisms fighting over the controller. */

static int bt_le_slice_start(void) {
    if (!_le_supported) {
        return -1;
    }
    /* Prefer extended scanning: it reports both legacy ADV_IND and BLE 5.0
       ADV_EXT_IND, so a keyboard that only sends extended PDUs becomes
       visible. Probe support once - a controller without LE 5.0 extended
       scanning refuses Set_Extended_Scan_Parameters and we fall back to the
       legacy path that already works for mice. */
    if (_le_ext_scan_supp != 0) {
        if (bt_le_ext_scan_params() == 0) {
            _le_ext_scan_supp = 1;
            if (bt_le_ext_scan_enable(true, true) == 0) {
                _le_scan_extended = true;
                return 0;
            }
            /* params accepted but enable failed: fall through to legacy */
        }
        else {
            _le_ext_scan_supp = 0;
        }
    }
    if (bt_le_scan_params() != 0) {
        return -1;
    }
    _le_scan_extended = false;
    return bt_le_scan_enable(true, true);
}

/* Inquiry_Length is counted in 1.28s units, so a shorter slice rounds up;
   the ceiling keeps a long scan session from turning into an inquiry the
   controller cannot be told to stop in time. */
static int bt_classic_inquiry_start(int slice_ms) {
    uint8_t params[5];
    int units;
    int ret;

    if (slice_ms < 1280) {
        slice_ms = 1280;
    }
    units = (slice_ms + 1279) / 1280;
    if (units > 0x30) {
        units = 0x30;
    }
    params[0] = 0x33; /* GIAC LAP 33:8b:9e, the "find everything" code */
    params[1] = 0x8b;
    params[2] = 0x9e;
    params[3] = (uint8_t)units;
    params[4] = 0x00; /* unlimited responses */

    ret = bt_hci_command_sync(HCI_OGF_LINK_CTRL, HCI_OCF_INQUIRY, params,
            sizeof(params), 1500);
    if (ret != 0) {
        slog("bluetooth inquiry_failed status=%d\n", ret);
        return -1;
    }
    _inquiry_running = true;
    return 0;
}

static void bt_scan_slice_stop(void) {
    /* all fire-and-forget: a slice stop is on xbt's click path (stop before
       connect/pair, scan, power off), and a synchronous wait here froze the
       caller for up to a second per outstanding command */
    if (_scan_slice == BT_SCAN_SLICE_LE && _le_scan_enabled) {
        if (_le_scan_extended) {
            (void)bt_le_ext_scan_enable_async(false, false);
        }
        else {
            (void)bt_le_scan_enable_async(false, false);
        }
    }
    if (_scan_slice == BT_SCAN_SLICE_CLASSIC && _inquiry_running) {
        (void)bt_hci_send_command(HCI_OGF_LINK_CTRL, HCI_OCF_INQUIRY_CANCEL,
                NULL, 0);
    }
    _inquiry_running = false;
    _le_scan_enabled = false;
}

/* LE_Create_Connection and scanning are mutually exclusive on these
   controllers, so a bring-up takes the radio for itself. _scanning stays
   set: the session resumes once the link is up or the attempt failed. */
static void bt_scan_suspend(void) {
    bt_scan_slice_stop();
    _scan_slice = BT_SCAN_SLICE_NONE;
    _scan_slice_end_ms = 0;
}

static bool bt_scan_enter_le(uint64_t now) {
    if (bt_le_slice_start() != 0) {
        return false;
    }
    _scan_slice = BT_SCAN_SLICE_LE;
    _scan_slice_end_ms = now + BT_LE_SCAN_SLICE_MS;
    return true;
}

static bool bt_scan_enter_classic(uint64_t now) {
    if (bt_classic_inquiry_start(BT_CLASSIC_SCAN_SLICE_MS) != 0) {
        return false;
    }
    _scan_slice = BT_SCAN_SLICE_CLASSIC;
    _scan_slice_end_ms = now + BT_CLASSIC_SCAN_SLICE_MS;
    return true;
}

/* ---- LE Security Manager ------------------------------------------------ */

static int smp_send(uint16_t handle, const uint8_t* pdu, uint16_t len) {
    return l2cap_send_pdu(handle, L2CAP_CID_SMP, pdu, len);
}

static void smp_reset(void) {
    memset(&_smp, 0, sizeof(_smp));
}

static bool smp_pres_pred(void* ctx) {
    (void)ctx;
    return _smp.got_pres || _smp.failed;
}

static bool smp_confirm_pred(void* ctx) {
    (void)ctx;
    return _smp.got_sconfirm || _smp.failed;
}

static bool smp_random_pred(void* ctx) {
    (void)ctx;
    return _smp.got_srand || _smp.failed;
}

static bool smp_peer_pub_pred(void* ctx) {
    (void)ctx;
    return _smp.got_peer_pub || _smp.failed;
}

static bool smp_dhkey_check_pred(void* ctx) {
    (void)ctx;
    return _smp.got_dhkey_check || _smp.failed;
}

static bool smp_keys_pred(void* ctx) {
    (void)ctx;
    return (_smp.got_peer_ltk && _smp.got_peer_ident) || _smp.failed;
}

/* Identity Information (IRK) then Identity Address Information arrive after
   the LTK/Master Identification when the peer honoured the IdKey request. */
static bool smp_ident_pred(void* ctx) {
    (void)ctx;
    return (_smp.got_peer_irk && _smp.got_peer_id_addr) || _smp.failed;
}

static bool smp_security_request_pred(void* ctx) {
    (void)ctx;
    return _smp.security_request_seen;
}

/* Pairing Request from the central. NoInputNoOutput with the MITM bit
   clear makes Just Works the negotiated association model. When the
   controller gave us a local P-256 public key we also offer the SC bit: a
   Secure-Connections-capable peer (an Xbox pad mandates it) then answers
   with SC set and smp_run takes the LESC path, while a legacy-only peer
   clears SC and falls back to the c1/s1 flow. RespKeyDist asks for the LTK
   (reconnection re-encrypts with it) and the IRK + identity address (the
   resolving list uses them to collapse a rotating private address to one
   stable identity). We still distribute nothing ourselves. */
static int smp_start_pairing(uint16_t handle) {
    uint8_t pdu[7];

    pdu[0] = SMP_CMD_PAIRING_REQUEST;
    pdu[1] = SMP_IO_NO_INPUT_NO_OUTPUT;
    pdu[2] = 0x00; /* OOB data not available */
    pdu[3] = SMP_AUTHREQ_BONDING;
    _smp.sc_local = _local_p256_valid;
    if (_smp.sc_local) {
        pdu[3] |= SMP_AUTHREQ_SC;
    }
    pdu[4] = 0x10; /* maximum encryption key size */
    pdu[5] = 0x00; /* initiator key distribution: none */
    pdu[6] = SMP_DIST_ENCKEY | SMP_DIST_IDKEY;

    memcpy(_smp.preq, pdu, sizeof(pdu));
    _smp.active = true;
    return smp_send(handle, pdu, sizeof(pdu));
}

static void smp_handle_rx(uint16_t handle, const uint8_t* pdu, size_t len) {
    /* SMP belongs to whichever session owns this handle. During a blocking
       bring-up _le_cur already points there; when a PDU arrives for the other
       link, temporarily switch so the _smp/_le macros hit the right session
       and restore afterwards. */
    int si = le_session_by_handle(handle);
    int saved = _le_cur;

    if (len < 1 || si < 0) {
        return;
    }
    _le_cur = si;

    switch (pdu[0]) {
    case SMP_CMD_PAIRING_RESPONSE:
        if (len >= 7 && !_smp.got_pres) {
            memcpy(_smp.pres, pdu, 7);
            _smp.got_pres = true;
        }
        break;
    case SMP_CMD_PAIRING_CONFIRM:
        if (len >= 17 && !_smp.got_sconfirm) {
            memcpy(_smp.sconfirm, pdu + 1, 16);
            _smp.got_sconfirm = true;
        }
        break;
    case SMP_CMD_PAIRING_RANDOM:
        if (len >= 17 && !_smp.got_srand) {
            memcpy(_smp.srand, pdu + 1, 16);
            _smp.got_srand = true;
        }
        break;
    case SMP_CMD_PAIRING_PUBLIC_KEY:
        if (len >= 65 && !_smp.got_peer_pub) {
            memcpy(_smp.peer_pub, pdu + 1, 64);
            _smp.got_peer_pub = true;
        }
        break;
    case SMP_CMD_PAIRING_DHKEY_CHECK:
        if (len >= 17 && !_smp.got_dhkey_check) {
            memcpy(_smp.peer_dhkey_check, pdu + 1, 16);
            _smp.got_dhkey_check = true;
        }
        break;
    case SMP_CMD_PAIRING_FAILED:
        _smp.failed = true;
        _smp.fail_reason = len >= 2 ? pdu[1] : 0;
        /* release every outstanding wait at once, whichever one is in it */
        _smp.got_pres = true;
        _smp.got_sconfirm = true;
        _smp.got_srand = true;
        _smp.got_peer_pub = true;
        _smp.got_dhkey_check = true;
        slog("bluetooth smp_failed reason=0x%02x\n", _smp.fail_reason);
        break;
    case SMP_CMD_ENCRYPTION_INFO:
        if (len >= 17) {
            memcpy(_smp.peer_ltk, pdu + 1, 16);
            _smp.got_peer_ltk = true;
        }
        break;
    case SMP_CMD_MASTER_IDENT:
        if (len >= 11) {
            _smp.peer_ediv = att_le16(pdu + 1);
            memcpy(_smp.peer_rand, pdu + 3, 8);
            _smp.got_peer_ident = true;
        }
        break;
    case SMP_CMD_IDENTITY_INFO:
        if (len >= 17) {
            memcpy(_smp.peer_irk, pdu + 1, 16);
            _smp.got_peer_irk = true;
        }
        break;
    case SMP_CMD_IDENTITY_ADDR_INFO:
        if (len >= 8) {
            _smp.peer_id_addr_type = pdu[1];
            memcpy(_smp.peer_id_addr, pdu + 2, 6);
            _smp.got_peer_id_addr = true;
        }
        break;
    case SMP_CMD_SECURITY_REQUEST:
        _smp.security_request_seen = true;
        _smp.security_request_auth = len >= 2 ? pdu[1] : 0;
        break;
    default:
        /* Signing Info is a legal phase-3 PDU we never asked for;
           ignoring beats failing. */
        break;
    }

    _le_cur = saved;
}

/* LE_Start_Encryption. Random_Number and EDIV are zero whenever the key
   in hand is an STK rather than a distributed LTK (Vol 3 Part H 2.4.4.1). */
static int smp_start_encryption(uint16_t handle, const uint8_t* ltk,
        uint16_t ediv, const uint8_t* rand8) {
    uint8_t params[28];
    int i;

    params[0] = (uint8_t)(handle & 0xff);
    params[1] = (uint8_t)(handle >> 8);
    for (i = 0; i < 8; ++i) {
        params[2 + i] = rand8 != NULL ? rand8[i] : 0;
    }
    params[10] = (uint8_t)(ediv & 0xff);
    params[11] = (uint8_t)(ediv >> 8);
    memcpy(params + 12, ltk, 16);

    _le.encrypted = false;
    _le.state = LE_ST_ENCRYPTING;
    _smp.enc_changed = false; /* this attempt's Encryption Change is pending */
    return bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_START_ENCRYPTION,
            params, sizeof(params), 2000);
}

/* Phase 3 key collection, shared by the legacy and LESC runs. On the legacy
   path the peripheral also distributes its LTK (take_peer_ltk); under LE
   Secure Connections the LTK is the f5 output and is never sent over SMP, so
   only the IRK + identity address are collected. Rekeying the stored bond
   onto the identity address is what keeps one rotating private address from
   turning into one bt.json entry per rotation. */
static void smp_collect_keys(bt_device_t* dev, bool take_peer_ltk) {
    if (take_peer_ltk && (_smp.pres[6] & SMP_DIST_ENCKEY) != 0 && dev != NULL &&
            bt_poll_until(smp_keys_pred, NULL, 3000) &&
            _smp.got_peer_ltk && _smp.got_peer_ident) {
        memcpy(dev->ltk, _smp.peer_ltk, 16);
        dev->ediv = _smp.peer_ediv;
        memcpy(dev->ltk_rand, _smp.peer_rand, 8);
        dev->has_ltk = true;
    }

    /* The peer's IRK + identity address (when it honoured the IdKey request)
       let the controller's resolving list turn its rotating private address
       back into one stable identity. */
    if ((_smp.pres[6] & SMP_DIST_IDKEY) != 0 && dev != NULL) {
        (void)bt_poll_until(smp_ident_pred, NULL, 2000);
    }
    if (dev != NULL && _smp.got_peer_irk && _smp.got_peer_id_addr) {
        memcpy(dev->irk, _smp.peer_irk, 16);
        dev->has_irk = true;
        memcpy(dev->id_addr, _smp.peer_id_addr, 6);
        dev->id_addr_type = _smp.peer_id_addr_type;
        dev->has_id_addr = true;
        memcpy(dev->addr, dev->id_addr, 6);
        dev->addr_type = dev->id_addr_type;
        memcpy(_le.addr, dev->id_addr, 6);
        _le.addr_type = dev->id_addr_type;
        bt_le_resolving_note_bond(dev);
    }
}

/* LE Secure Connections pairing, Just Works association model. We are the
   initiator (we sent the Pairing Request), so we drive the P-256 public-key
   exchange, run the ECDH on the controller, check the confirm/random pair
   and the DHKey Check round, and derive MacKey + LTK from f5. Called from
   smp_run once the Pairing Response has arrived and both sides set SC.
   Returns 0 on a valid bond. The argument order of every f4/f5/f6 call
   mirrors the initiator branch of the reference SMP stack: A1 is always our
   address and A2 the peer's, with the nonces Na=ours and Nb=peer. */
static int smp_run_lesc(uint16_t handle, bt_device_t* dev,
        const char* addr_str) {
    uint8_t pdu[65];
    uint8_t zero[16];
    uint8_t iocap[3];
    uint8_t cb_check[16];
    uint8_t mackey[16];
    uint8_t ltk[16];
    uint8_t ea[16];
    uint8_t eb_expected[16];

    memset(zero, 0, sizeof(zero));

    /* 1. Send our P-256 public key (Qx || Qy), the initiator goes first. */
    pdu[0] = SMP_CMD_PAIRING_PUBLIC_KEY;
    memcpy(pdu + 1, _local_p256, 64);
    if (smp_send(handle, pdu, 65) != 0) {
        return -1;
    }

    /* 2. Wait for the peer's public key, then let the controller do ECDH. */
    if (!bt_poll_until(smp_peer_pub_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_public_key %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }
    if (bt_le_generate_dhkey(_smp.peer_pub) != 0) {
        slog("bluetooth smp_dhkey_failed %s\n", addr_str);
        return -1;
    }

    /* 3. The responder answers with its Pairing Confirm (Cb) right after its
          public key; we then send our Pairing Random (Na). */
    if (!bt_poll_until(smp_confirm_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_confirm %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }
    pdu[0] = SMP_CMD_PAIRING_RANDOM;
    memcpy(pdu + 1, _smp.mrand, 16);
    if (smp_send(handle, pdu, 17) != 0) {
        return -1;
    }

    /* 4. The peer's Pairing Random (Nb). */
    if (!bt_poll_until(smp_random_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_random %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }

    /* 5. Cb = f4(PKbx, PKax, Nb, 0): PKb the responder (peer), PKa us; each
          takes only the 32-byte x coordinate, which is the first half of the
          64-byte public key. */
    smp_f4(_smp.peer_pub, _local_p256, _smp.srand, 0, cb_check);
    if (memcmp(cb_check, _smp.sconfirm, 16) != 0) {
        uint8_t fail[2];

        slog("bluetooth smp_confirm_mismatch %s\n", addr_str);
        fail[0] = SMP_CMD_PAIRING_FAILED;
        fail[1] = SMP_REASON_CONFIRM_VALUE_FAILED;
        (void)smp_send(handle, fail, sizeof(fail));
        return -1;
    }

    /* 6. MacKey || LTK = f5(DHKey, Na, Nb, A1=us, A2=peer). */
    smp_f5(_dhkey, _smp.mrand, _smp.srand, _local_addr_type, _local_addr,
            _le.addr_type, _le.addr, mackey, ltk);

    /* 7. Ea = f6(MacKey, Na, Nb, R=0, IOcap(ours), A1=us, A2=peer); send it.
          IOcap is the 3-octet [AuthReq, OOB, IOCap] of our Pairing Request. */
    iocap[0] = _smp.preq[3];
    iocap[1] = _smp.preq[2];
    iocap[2] = _smp.preq[1];
    smp_f6(mackey, _smp.mrand, _smp.srand, zero, iocap,
            _local_addr_type, _local_addr, _le.addr_type, _le.addr, ea);
    pdu[0] = SMP_CMD_PAIRING_DHKEY_CHECK;
    memcpy(pdu + 1, ea, 16);
    if (smp_send(handle, pdu, 17) != 0) {
        return -1;
    }

    /* 8. The peer's DHKey Check (Eb). */
    if (!bt_poll_until(smp_dhkey_check_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_dhkey_check %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }

    /* 9. Eb = f6(MacKey, Nb, Na, R=0, IOcap(peer), A1=peer, A2=us): the
          responder swaps the nonces, its own IOcap and the two addresses. */
    iocap[0] = _smp.pres[3];
    iocap[1] = _smp.pres[2];
    iocap[2] = _smp.pres[1];
    smp_f6(mackey, _smp.srand, _smp.mrand, zero, iocap,
            _le.addr_type, _le.addr, _local_addr_type, _local_addr, eb_expected);
    if (memcmp(eb_expected, _smp.peer_dhkey_check, 16) != 0) {
        uint8_t fail[2];

        slog("bluetooth smp_dhkey_check_mismatch %s\n", addr_str);
        fail[0] = SMP_CMD_PAIRING_FAILED;
        fail[1] = SMP_REASON_DHKEY_CHECK_FAILED;
        (void)smp_send(handle, fail, sizeof(fail));
        return -1;
    }

    /* 10. The f5 LTK encrypts the link; EDIV and Rand stay zero for a fresh
           LESC bond (Vol 3 Part H 2.4.4.1). */
    memcpy(_smp.ltk, ltk, 16);
    if (dev != NULL) {
        memcpy(dev->ltk, ltk, 16);
        dev->ediv = 0;
        memset(dev->ltk_rand, 0, 8);
        dev->has_ltk = true;
    }
    if (smp_start_encryption(handle, ltk, 0, NULL) != 0) {
        slog("bluetooth smp_start_encryption_failed %s\n", addr_str);
        return -1;
    }
    if (!bt_poll_until(le_encrypted_pred, NULL, 5000) || !_le.encrypted) {
        slog("bluetooth smp_no_encryption %s\n", addr_str);
        return -1;
    }

    /* 11. Phase 3: the peer may still distribute its IRK + identity address. */
    smp_collect_keys(dev, false);
    return 0;
}

/* Legacy pairing with Just Works: TK is all zeroes, each confirm value is
   c1 over one side's random, and the key that encrypts the link is
   s1(randoms). The whole exchange runs as a bounded blocking sequence,
   the same shape as the classic SSP path it sits next to. */
static int smp_run(uint16_t handle, bt_device_t* dev) {
    uint8_t tk[16];
    uint8_t pdu[17];
    uint8_t confirm[16];
    uint8_t check[16];
    uint8_t stk[16];
    char addr_str[24];

    bt_addr_to_str(_le.addr, addr_str, sizeof(addr_str));
    smp_reset();
    _smp.active = true;
    memset(tk, 0, sizeof(tk));
    bt_fill_random(_smp.mrand, sizeof(_smp.mrand));
    _le.state = LE_ST_PAIRING;

    if (smp_start_pairing(handle) != 0) {
        slog("bluetooth smp_send_failed %s\n", addr_str);
        return -1;
    }
    if (!bt_poll_until(smp_pres_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_pairing_response %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }

    /* Both sides advertised LE Secure Connections: take the LESC path. A
       Secure-Connections-only peer (an Xbox pad mandates it) never treats a
       legacy Just Works bond as valid, so the legacy flow leaves it stuck in
       pairing mode streaming no input. When the peer cleared SC we fall
       through to the c1/s1 flow below unchanged. */
    if ((_smp.preq[3] & SMP_AUTHREQ_SC) != 0 &&
            (_smp.pres[3] & SMP_AUTHREQ_SC) != 0) {
        return smp_run_lesc(handle, dev, addr_str);
    }

    smp_c1(tk, _smp.mrand, _smp.preq, _smp.pres, _local_addr_type,
            _local_addr, _le.addr_type, _le.addr, confirm);
    memcpy(_smp.mconfirm, confirm, 16);
    pdu[0] = SMP_CMD_PAIRING_CONFIRM;
    memcpy(pdu + 1, confirm, 16);
    if (smp_send(handle, pdu, sizeof(pdu)) != 0) {
        return -1;
    }
    if (!bt_poll_until(smp_confirm_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_confirm %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }

    pdu[0] = SMP_CMD_PAIRING_RANDOM;
    memcpy(pdu + 1, _smp.mrand, 16);
    if (smp_send(handle, pdu, sizeof(pdu)) != 0) {
        return -1;
    }
    if (!bt_poll_until(smp_random_pred, NULL, BT_LE_SMP_TIMEOUT_MS)) {
        slog("bluetooth smp_no_random %s\n", addr_str);
        return -1;
    }
    if (_smp.failed) {
        return -1;
    }

    smp_c1(tk, _smp.srand, _smp.preq, _smp.pres, _local_addr_type,
            _local_addr, _le.addr_type, _le.addr, check);
    if (memcmp(check, _smp.sconfirm, 16) != 0) {
        uint8_t fail[2];

        slog("bluetooth smp_confirm_mismatch %s\n", addr_str);
        fail[0] = SMP_CMD_PAIRING_FAILED;
        fail[1] = SMP_REASON_CONFIRM_VALUE_FAILED;
        (void)smp_send(handle, fail, sizeof(fail));
        return -1;
    }

    /* s1 takes the responder's random first (Vol 3 Part H 2.3.5.5) */
    smp_s1(tk, _smp.srand, _smp.mrand, stk);
    if (smp_start_encryption(handle, stk, 0, NULL) != 0) {
        slog("bluetooth smp_start_encryption_failed %s\n", addr_str);
        return -1;
    }
    if (!bt_poll_until(le_encrypted_pred, NULL, 5000) || !_le.encrypted) {
        slog("bluetooth smp_no_encryption %s\n", addr_str);
        return -1;
    }

    /* Phase 3: the peripheral distributes its LTK (re-encryption on the next
       power-on) plus its IRK + identity address. */
    smp_collect_keys(dev, true);
    return 0;
}

/* ---- HID over GATT bring-up -------------------------------------------- */

static int hogp_subscribe_attr(uint16_t handle, hogp_attr_t* a) {
    uint16_t value;

    if (a->cccd_handle == 0 ||
            (a->props & (GATT_CHR_PROP_NOTIFY | GATT_CHR_PROP_INDICATE)) == 0) {
        return 0;
    }
    value = (a->props & GATT_CHR_PROP_NOTIFY) != 0 ? GATT_CCCD_NOTIFY
                                                   : GATT_CCCD_INDICATE;
    if (gatt_write_cccd(handle, a->cccd_handle, value) != 0) {
        slog("bluetooth le_subscribe_failed uuid=0x%04x cccd=0x%04x\n",
                a->uuid, a->cccd_handle);
        return 0;
    }
    return 1;
}

static int hogp_subscribe(uint16_t handle, uint16_t uuid) {
    hogp_attr_t* a = hogp_find(uuid);

    if (a == NULL) {
        return 0;
    }
    return hogp_subscribe_attr(handle, a);
}

static int hogp_read_report_map(uint16_t handle) {
    hogp_attr_t* a = hogp_find(GATT_CHR_REPORT_MAP);
    uint16_t got = 0;

    if (a == NULL || (a->props & GATT_CHR_PROP_READ) == 0) {
        return -1;
    }
    if (gatt_read_value(handle, a->value_handle, _hogp.report_map,
            sizeof(_hogp.report_map), &got) != 0) {
        return -1;
    }
    _hogp.report_map_len = got;
    _hogp.report_map_complete = true;
    return 0;
}

/* Boot Protocol first: a mouse or keyboard that accepts it delivers
   reports in a layout the HID spec fixes, so no descriptor has to be
   parsed at all. Report Protocol is the fallback for peripherals that
   refuse the write or expose no boot characteristics. */
static int hogp_bringup(uint16_t handle) {
    hogp_attr_t* pm;
    uint8_t mode;
    int i;

    memset(&_hogp, 0, sizeof(_hogp));
    _hogp.active = true;
    _hogp.mtu = L2CAP_LE_MTU_DEFAULT;

    if (gatt_exchange_mtu(handle) != 0) {
        /* 23 octets still fits every request we send, so carry on */
        slog("bluetooth le_mtu_exchange_failed mtu=%u\n", _hogp.mtu);
    }
    if (gatt_find_hid_service(handle) != 0) {
        slog("bluetooth le_no_hid_service\n");
        return -1;
    }
    if (gatt_discover_chars(handle) != 0) {
        slog("bluetooth le_no_characteristics\n");
        return -1;
    }
    (void)gatt_discover_descriptors(handle);

    pm = hogp_find(GATT_CHR_PROTOCOL_MODE);
    if (pm != NULL && (pm->props & GATT_CHR_PROP_WRITE) != 0) {
        mode = GATT_PROTOCOL_MODE_BOOT;
        if (gatt_write_value(handle, pm->value_handle, &mode, 1) == 0) {
            _hogp.boot_mode_ok = true;
        }
    }
    if (_hogp.boot_mode_ok) {
        _hogp.n_subscribed = hogp_subscribe(handle, GATT_CHR_BOOT_MOUSE_INPUT) +
                hogp_subscribe(handle, GATT_CHR_BOOT_KBD_INPUT);
        if (_hogp.n_subscribed > 0) {
            return 0;
        }
    }

    if (pm != NULL && (pm->props & GATT_CHR_PROP_WRITE) != 0) {
        mode = GATT_PROTOCOL_MODE_REPORT;
        (void)gatt_write_value(handle, pm->value_handle, &mode, 1);
    }
    if (hogp_read_report_map(handle) != 0) {
        slog("bluetooth le_no_report_map\n");
        return -1;
    }
    if (hid_parse_mouse_report(_hogp.report_map, (int)_hogp.report_map_len,
            &_hogp.mouse) == 0 &&
            mouse_parser_sane(&_hogp.mouse, 64, false)) {
        _hogp.mouse_ok = true;
    }
    /* classify the Report characteristics by the Report Map's own collection
       IDs: a keyboard body shorter than 8 octets is indistinguishable from a
       mouse by length alone, so the notify path routes on these IDs. */
    _hogp.kbd_report_id =
            hid_find_kbd_report_id(_hogp.report_map, (int)_hogp.report_map_len);
    /* A LE gamepad (an Xbox/8BitDo pad) exposes a Generic Desktop Joystick or
       Game Pad collection in its Report Map instead of a mouse or keyboard.
       When it parses as one - and the map was not already claimed as a mouse
       or keyboard - latch the descriptor-driven parser so the notify path can
       normalize its Report characteristic into a js_evt_t for hid_joystickd,
       exactly like usbhostd does for a USB pad. */
    if (!_hogp.mouse_ok && _hogp.kbd_report_id == 0 &&
            hid_probe_joystick_report(_hogp.report_map,
                (int)_hogp.report_map_len, &_hogp.joystick)) {
        _hogp.is_gamepad = true;
        _hogp.joystick_ok = true;
        _hogp.joystick_report_id = _hogp.joystick.report_id;
        slog("bluetooth le_gamepad report_id=%u\n", _hogp.joystick_report_id);
    }
    /* the Report Map is the authoritative classification for a connected LE
       HID peripheral (appearance is optional and often absent); give the
       device a Peripheral-class CoD so xbt's type column is not Unknown */
    {
        bt_device_t* d = bt_find_device_by_handle(handle);
        if (d != NULL && d->class_of_device == 0) {
            d->class_of_device = bt_le_synth_cod(_hogp.mouse_ok,
                    _hogp.kbd_report_id != 0, _hogp.is_gamepad);
        }
    }
    for (i = 0; i < _hogp.n_attrs; ++i) {
        hogp_attr_t* a = &_hogp.attrs[i];

        if (a->uuid != GATT_CHR_REPORT) {
            continue;
        }
        /* report_type 0 means no Report Reference descriptor was there,
           which on an input-only peripheral still means input */
        if (a->report_type != 0 && a->report_type != 1) {
            continue;
        }
        _hogp.n_subscribed += hogp_subscribe_attr(handle, a);
    }
    return _hogp.n_subscribed > 0 ? 0 : -1;
}

/* ---- LE connection, teardown and request queueing ---------------------- */

static void bt_le_fail(const char* reason) {
    char addr_str[24];

    bt_addr_to_str(_le.addr, addr_str, sizeof(addr_str));
    _le.state = LE_ST_FAILED;
    _le.deadline_ms = kernel_tic_ms(0) + 3000;
    bt_emit("connect_fail %s reason=%s\n", addr_str, reason);
    slog("bluetooth le_connect_failed %s reason=%s\n", addr_str, reason);
    if (_le.handle_valid) {
        uint8_t params[3];

        params[0] = (uint8_t)(_le.handle & 0xff);
        params[1] = (uint8_t)(_le.handle >> 8);
        params[2] = 0x13; /* remote user terminated connection */
        (void)bt_hci_command_sync(HCI_OGF_LINK_CTRL, HCI_OCF_DISCONNECT,
                params, sizeof(params), 2000);
    }
}

void bt_le_stack_reset(void) {
    memset(&_le, 0, sizeof(_le));
    memset(&_att, 0, sizeof(_att));
    memset(&_smp, 0, sizeof(_smp));
    memset(&_hogp, 0, sizeof(_hogp));
    _le.state = LE_ST_IDLE;
}

/* An LE HID link always ends up paired - input reports only flow over an
   encrypted ATT bearer - so the caller's pair flag changes nothing here,
   and a bonded peripheral is re-encrypted instead of going through SMP
   again. */
/* LE_Extended_Create_Connection for a single initiating PHY (LE 1M). The
   header is Initiator_Filter_Policy, Own_Address_Type, Peer_Address_Type,
   Peer_Address(6), Initiating_PHYs; then per selected PHY a 16-octet block
   Scan_Interval, Scan_Window, Conn_Interval_Min, Conn_Interval_Max,
   Max_Latency, Supervision_Timeout, Min_CE_Length, Max_CE_Length. A peer
   that only sends ADV_EXT_IND is unreachable with the legacy command, which
   scans for ADV_IND alone. */
static int bt_le_ext_create_connection(void) {
    uint8_t p[26];
    int i;

    p[0] = 0x00; /* Initiator_Filter_Policy: peer address, not the accept list */
    p[1] = _local_addr_type; /* Own_Address_Type */
    p[2] = _le.addr_type;    /* Peer_Address_Type */
    for (i = 0; i < 6; ++i) {
        p[3 + i] = _le.addr[i];
    }
    p[9] = 0x01; /* Initiating_PHYs: LE 1M */
    p[10] = (uint8_t)(BT_LE_SCAN_INTERVAL & 0xff);
    p[11] = (uint8_t)(BT_LE_SCAN_INTERVAL >> 8);
    p[12] = (uint8_t)(BT_LE_SCAN_WINDOW & 0xff);
    p[13] = (uint8_t)(BT_LE_SCAN_WINDOW >> 8);
    p[14] = (uint8_t)(BT_LE_CONN_ITV_MIN & 0xff);
    p[15] = (uint8_t)(BT_LE_CONN_ITV_MIN >> 8);
    p[16] = (uint8_t)(BT_LE_CONN_ITV_MAX & 0xff);
    p[17] = (uint8_t)(BT_LE_CONN_ITV_MAX >> 8);
    p[18] = (uint8_t)(BT_LE_CONN_LATENCY & 0xff);
    p[19] = (uint8_t)(BT_LE_CONN_LATENCY >> 8);
    p[20] = (uint8_t)(BT_LE_CONN_TIMEOUT & 0xff);
    p[21] = (uint8_t)(BT_LE_CONN_TIMEOUT >> 8);
    p[22] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[23] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);
    p[24] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[25] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);
    return bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_EXT_CREATE_CONNECTION, p,
            sizeof(p), 2000);
}

/* Force the live link to a fast connection interval now that reports are
   flowing. LE_Create_Connection only sets the INITIAL interval; this host
   does not auto-apply even an accepted L2CAP param update, and an idle mouse
   keeps asking to stretch the link for power saving, so a HOGP link tends to
   sit at 30ms (~33Hz) and feel sluggish. An explicit LE_Connection_Update to
   the input floor forces 7.5-15ms (66-133Hz); an interval-limited mouse (it
   requested exactly this) starts reporting faster immediately. The outcome
   arrives asynchronously as an LE Connection Update Complete event;
   bt_le_handle_conn_update reports failures. */
static void bt_le_conn_update_fast(uint16_t handle) {
    uint8_t p[14];

    if (handle == 0) {
        return;
    }
    p[0] = (uint8_t)(handle & 0xff);
    p[1] = (uint8_t)(handle >> 8);
    p[2] = (uint8_t)(BT_LE_CONN_ITV_MIN & 0xff);
    p[3] = (uint8_t)(BT_LE_CONN_ITV_MIN >> 8);
    p[4] = (uint8_t)(BT_LE_INPUT_ITV_MAX & 0xff);
    p[5] = (uint8_t)(BT_LE_INPUT_ITV_MAX >> 8);
    p[6] = (uint8_t)(BT_LE_CONN_LATENCY & 0xff);
    p[7] = (uint8_t)(BT_LE_CONN_LATENCY >> 8);
    p[8] = (uint8_t)(BT_LE_CONN_TIMEOUT & 0xff);
    p[9] = (uint8_t)(BT_LE_CONN_TIMEOUT >> 8);
    p[10] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[11] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);
    p[12] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[13] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);
    (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_CONNECTION_UPDATE,
            p, sizeof(p), 1000);
}

int bt_le_connect(bt_device_t* dev, bool pair,
        char* ret_text, size_t ret_text_sz) {
    uint8_t p[25];
    char addr_str[24];
    bool ext_conn;
    int i;

    (void)pair;
    ext_conn = dev->ext_adv; /* capture before dev is re-resolved below */
    bt_le_stack_reset();
    memcpy(_le.addr, dev->addr, 6);
    _le.addr_type = dev->addr_type;
    _le.state = LE_ST_CONNECTING;
    bt_addr_to_str(_le.addr, addr_str, sizeof(addr_str));
    bt_scan_suspend();
    bt_emit("le_connect_begin %s\n", addr_str);

    p[0] = (uint8_t)(BT_LE_SCAN_INTERVAL & 0xff);
    p[1] = (uint8_t)(BT_LE_SCAN_INTERVAL >> 8);
    p[2] = (uint8_t)(BT_LE_SCAN_WINDOW & 0xff);
    p[3] = (uint8_t)(BT_LE_SCAN_WINDOW >> 8);
    p[4] = 0x00; /* filter policy: the peer address, not the white list */
    p[5] = _le.addr_type;
    for (i = 0; i < 6; ++i) {
        p[6 + i] = _le.addr[i];
    }
    p[12] = _local_addr_type;
    p[13] = (uint8_t)(BT_LE_CONN_ITV_MIN & 0xff);
    p[14] = (uint8_t)(BT_LE_CONN_ITV_MIN >> 8);
    p[15] = (uint8_t)(BT_LE_CONN_ITV_MAX & 0xff);
    p[16] = (uint8_t)(BT_LE_CONN_ITV_MAX >> 8);
    p[17] = (uint8_t)(BT_LE_CONN_LATENCY & 0xff);
    p[18] = (uint8_t)(BT_LE_CONN_LATENCY >> 8);
    p[19] = (uint8_t)(BT_LE_CONN_TIMEOUT & 0xff);
    p[20] = (uint8_t)(BT_LE_CONN_TIMEOUT >> 8);
    p[21] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[22] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);
    p[23] = (uint8_t)(BT_LE_CONN_CE_LEN & 0xff);
    p[24] = (uint8_t)(BT_LE_CONN_CE_LEN >> 8);

    /* An extended-advertising peer needs LE_Extended_Create_Connection; a
       legacy one keeps the plain command. If the extended command is refused
       (a controller that accepted extended scanning should not, but stay
       defensive) fall back to the legacy create connection. */
    if (ext_conn && bt_le_ext_create_connection() == 0) {
        /* accepted */
    }
    else if (bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_CREATE_CONNECTION, p,
            sizeof(p), 2000) != 0) {
        bt_le_stack_reset();
        bt_emit("connect_fail %s reason=le_create_conn\n", addr_str);
        slog("bluetooth le_create_conn_failed %s\n", addr_str);
        return -1;
    }
    if (!bt_poll_until(le_link_up_pred, NULL, BT_LE_CONNECT_TIMEOUT_MS) ||
            !_le.handle_valid) {
        /* A Connection Complete carrying a non-zero status already put us
           in LE_ST_FAILED, and that is a different story from a radio that
           never answered: the peripheral is awake but refusing us. The
           state has to be read before the reset wipes it. */
        bool refused = _le.state == LE_ST_FAILED;

        (void)bt_hci_command_sync(HCI_OGF_LE, HCI_OCF_LE_CREATE_CONN_CANCEL,
                NULL, 0, 1000);
        bt_le_stack_reset();
        bt_emit("connect_fail %s reason=%s\n", addr_str,
                refused ? "le_conn_refused" : "le_timeout");
        slog("bluetooth le_connect_%s %s\n", refused ? "refused" : "timeout",
                addr_str);
        return -1;
    }

    dev = bt_find_device(_le.addr, true);
    if (dev == NULL) {
        bt_le_fail("no_device_slot");
        return -1;
    }

    smp_reset();
    _smp.active = true;
    /* a peripheral usually asks for pairing itself; give it the chance so
       our Pairing Request does not cross with its Security Request */
    (void)bt_poll_until(smp_security_request_pred, NULL, 300);

    if (dev->has_ltk) {
        if (smp_start_encryption(_le.handle, dev->ltk, dev->ediv,
                dev->ltk_rand) != 0 ||
                !bt_poll_until(le_encrypted_pred, NULL, 4000) ||
                !_le.encrypted) {
            /* rejected or expired key: drop it and pair from scratch */
            slog("bluetooth le_reencrypt_failed %s\n", addr_str);
            dev->has_ltk = false;
            _le.encrypted = false;
        }
    }
    if (!_le.encrypted && smp_run(_le.handle, dev) != 0) {
        bt_le_fail("pairing");
        return -1;
    }

    _le.state = LE_ST_DISCOVERING;
    if (hogp_bringup(_le.handle) != 0) {
        bt_le_fail("hogp");
        return -1;
    }

    _le.state = LE_ST_READY;
    bt_known_touch_from_device(dev);
    bt_emit("hid_up %s handle=0x%04X le=1 boot=%d reports=%d\n", addr_str,
            _le.handle, _hogp.boot_mode_ok ? 1 : 0, _hogp.n_subscribed);
    /* reports start flowing now: pull the link to the fast input interval
       before the mouse gets a chance to stretch it for power saving */
    bt_le_conn_update_fast(_le.handle);

    /* Suppress a duplicate transport only for a device already served by
       HOGP. A separate classic keyboard must remain connected. */
    if (_hid.active) {
        bt_device_t* cdev = bt_find_device(_hid.addr, false);
        if (cdev != NULL && bt_hogp_blocks_classic(cdev)) {
            uint16_t chandle = _hid.acl_handle;
            char caddr[24];
            bt_addr_to_str(cdev->addr, caddr, sizeof(caddr));
            bt_hid_stop();
            /* Drop only a genuine classic transport. Were a classic session
               ever aliased onto this live LE handle, disconnecting it would
               tear down the very HOGP link just brought up (the pad drops at
               READY and reconnect-loops), so an LE-session handle is left
               connected; bt_hid_start already refuses to bind one. */
            if (chandle != 0 && le_session_by_handle(chandle) < 0) {
                slog("bluetooth hid_classic_drop %s reason=hogp_contention\n", caddr);
                bt_hci_disconnect(chandle);
            }
        }
    }
    if (ret_text != NULL && ret_text_sz > 0) {
        snprintf(ret_text, ret_text_sz, "connect_ok %s handle=0x%04X le=1\n",
                addr_str, _le.handle);
    }
    return 0;
}

void bt_le_link_closed(uint16_t handle, uint8_t reason) {
    int si = le_session_by_handle(handle);
    bt_device_t* dev;

    if (si < 0) {
        return;
    }
    dev = bt_find_device(_les[si].le.addr, false);
    if (dev != NULL) {
        dev->connected = false;
        dev->handle = 0;
    }
    bt_emit("le_disconnected handle=0x%04X reason=%u\n", handle, reason);
    slog("bluetooth le_link_closed handle=0x%04x reason=%u state=%d\n",
            handle, reason, (int)_les[si].le.state);
    /* FAILED rather than a full reset: a bring-up in flight is blocked on
       a predicate and has to be released now, and bt_le_step clears the
       session once its retry deadline passes. */
    _les[si].le.handle_valid = false;
    _les[si].le.encrypted = false;
    _les[si].le.state = LE_ST_FAILED;
    _les[si].le.deadline_ms = kernel_tic_ms(0) + 3000;
}

/* The LE fixed channels never go through L2CAP signalling, so they are
   routed straight out of l2cap_dispatch. LE signalling carries exactly one
   command a peripheral may send us, a connection parameter update request.
   An idle mouse uses it to stretch the live link to a long power-saving
   interval (30-50ms => 20-33Hz), which is the sluggish-cursor symptom, so
   the old always-accept policy is replaced with an input floor: accept only
   when the peer's fastest acceptable interval is still quick enough for a
   pointing device, otherwise reject so the link stays at the fast interval
   bt_le_connect negotiated. A rejected peer may re-ask occasionally; that
   costs a few tiny signalling PDUs and never adds input latency. */
void bt_le_l2cap_rx(uint16_t handle, uint16_t cid,
        const uint8_t* data, size_t len) {
    if (cid == L2CAP_CID_ATT) {
        att_handle_rx(handle, data, len);
        return;
    }
    if (cid == L2CAP_CID_SMP) {
        smp_handle_rx(handle, data, len);
        return;
    }
    if (cid == L2CAP_CID_LE_SIGNAL && len >= 4 &&
            data[0] == L2CAP_SIG_LE_CONN_PARAM_UPDATE_REQ) {
        uint8_t cmd[6];
        bool accept = true;

        /* request body: interval_min(2) interval_max(2) latency(2) timeout(2)
           after the 4-byte code/id/length signalling header. */
        if (len >= 12) {
            uint16_t itv_min = att_le16(data + 4);
            accept = (itv_min <= BT_LE_INPUT_ITV_MAX);
        }

        cmd[0] = L2CAP_SIG_LE_CONN_PARAM_UPDATE_RSP;
        cmd[1] = data[1]; /* identifier, echoed back */
        cmd[2] = 0x02;    /* data length */
        cmd[3] = 0x00;
        cmd[4] = accept ? 0x00 : 0x01; /* result: 0 = accept, 1 = reject */
        cmd[5] = 0x00;
        (void)l2cap_send_pdu(handle, L2CAP_CID_LE_SIGNAL, cmd, sizeof(cmd));
    }
}

/* The command-handler entry point. A bring-up blocks for seconds, so it is
   queued and answered later over the event stream: the caller gets
   connect_begin at once and sees hid_up or connect_fail in the events. */
int bt_le_request(bt_device_t* dev, bool pair,
        char* ret_text, size_t ret_text_sz) {
    char addr_str[24];
    int i;

    bt_addr_to_str(dev->addr, addr_str, sizeof(addr_str));
    /* already connected on some session: answer success with its handle */
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_READY &&
                bt_addr_equal(_les[i].le.addr, dev->addr)) {
            if (ret_text != NULL && ret_text_sz > 0) {
                snprintf(ret_text, ret_text_sz,
                        "connect_ok %s handle=0x%04X le=1\n",
                        addr_str, _les[i].le.handle);
            }
            return 0;
        }
    }
    if (!_le_supported) {
        if (ret_text != NULL && ret_text_sz > 0) {
            snprintf(ret_text, ret_text_sz,
                    "connect_fail %s reason=le_unsupported\n", addr_str);
        }
        return -1;
    }
    if (!bt_le_queue_request(dev, pair)) {
        if (ret_text != NULL && ret_text_sz > 0) {
            snprintf(ret_text, ret_text_sz, "connect_busy %s\n", addr_str);
        }
        return -1;
    }
    bt_emit("connect_begin %s le=1\n", addr_str);
    if (ret_text != NULL && ret_text_sz > 0) {
        snprintf(ret_text, ret_text_sz, "connect_begin %s le=1\n", addr_str);
    }
    return 0;
}

/* True while an LE bring-up owns the radio exclusively: bt_le_connect is
   blocked in bt_poll_until on the daemon's main context, driving one session
   through CONNECTING -> LINK_UP -> PAIRING -> ENCRYPTING -> DISCOVERING.
   During that window no scan slice may be armed. xbt fires a "scan 10"
   dev.cmd once a second; that handler runs on the kernel IPC interrupt and
   preempts the blocked bring-up, and bt_scan_enter_le -> bt_le_slice_start
   issues synchronous scan HCI (bt_le_[ext_]scan_params/enable all call
   bt_hci_command_sync). Two failures follow: (1) the radio is toggled between
   initiating and scanning, so the pending LE_Create_Connection is starved or
   aborted -> le_connect_timeout (then a late status=0x02 from the
   create-conn-cancel race) and the pad drops back to advertising (keeps
   blinking); (2) bt_wait_for_opcode keeps a single global _wait_cmd slot, so
   the nested command-sync memsets it and clobbers whichever synchronous
   command the bring-up is itself waiting on. Refusing the scan during a
   bring-up lets the connection establish; xbt retries next second and the
   scan proceeds once the link is up or the attempt has failed. */
static bool le_bringup_busy(void) {
    int i;

    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        le_state_t s = _les[i].le.state;
        if (s >= LE_ST_CONNECTING && s <= LE_ST_DISCOVERING) {
            return true;
        }
    }
    return false;
}

/* One tick of LE work: failure retry deadlines, queued bring-ups, and the
   discovery slice alternation. Everything that may block for seconds lives
   here rather than in a command handler, which has to stay short enough
   for a click in xbt to feel instant.

   from_loop distinguishes the two callers. Only bt_loop (the daemon's main
   context) may run a queued bring-up: a dev.cmd handler runs on the kernel's
   IPC interrupt, and while it is inside that interrupt every OTHER client's
   ipc_call to /dev/bt0 comes back IPC_ERROR_RETRY and parks until it unwinds
   (kernel ipc.c proc_ipc_call). A bring-up blocks for tens of seconds (LESC
   public-key/DHKey/confirm rounds plus HOGP discovery), so running it from a
   handler -- bt_start_scan reaches here via the "scan" verb, which xbt fires
   once a second -- freezes the whole xbt UI for its duration. bt_start_scan
   therefore passes false: the scan slice still arms at once, but the queued
   bring-up is left for the next bt_loop tick, exactly where a blocking wait
   can be preempted by an incoming IPC. */
void bt_le_step(bool from_loop) {
    uint64_t now = kernel_tic_ms(0);
    bool want_le;
    int i;

    /* retry deadlines: clear any session that failed long enough ago, so its
       slot becomes free for the next candidate without disturbing the other */
    for (i = 0; i < MAX_LE_SESSIONS; ++i) {
        if (_les[i].le.state == LE_ST_FAILED && _les[i].le.deadline_ms != 0 &&
                now >= _les[i].le.deadline_ms) {
            int saved = _le_cur;
            slog("bluetooth le_reset_after_failure slot=%d\n", i);
            _le_cur = i;
            bt_le_stack_reset();
            _le_cur = saved;
        }
    }

    if (from_loop && _le_req_active) {
        bt_device_t* dev = bt_find_device(_le_req_addr, false);
        bool was_autoconnect = _le_autoconnect;
        int saved = _le_cur;

        _le_req_active = false;
        _le_autoconnect = false;
        if (dev == NULL) {
            char addr_str[24];

            bt_addr_to_str(_le_req_addr, addr_str, sizeof(addr_str));
            bt_emit("connect_fail %s reason=unknown_device\n", addr_str);
            return;
        }
        _le_cur = _le_req_slot;   /* bring the target session up */
        bt_scan_suspend();
        (void)bt_le_connect(dev, _le_req_pair, NULL, 0);
        _le_cur = saved;          /* back to the primary setup session */
        if (was_autoconnect) {
            /* the session was armed for autoconnect, so hand the radio to
               the next known device now that this one is up or failed */
            bt_autoconnect_known();
        }
        return;
    }

    /* A bring-up already in flight owns the radio: never arm, flip or stop a
       scan slice underneath it (see le_bringup_busy). This is the choke point
       that actually issues scan HCI, so guarding here protects every caller -
       in particular a re-entrant bt_le_step(false) from bt_start_scan running
       on the "scan" IPC interrupt while bt_le_connect blocks on the main
       context. */
    if (le_bringup_busy()) {
        return;
    }

    /* Radio-contention guard (input latency outranks discovery): a scan
       slice owns the whole radio for BT_LE_SCAN_SLICE_MS /
       BT_CLASSIC_SCAN_SLICE_MS (2.5s each), and a classic BR/EDR inquiry in
       particular starves a live LE connection - the BLE mouse's reports
       cannot get through, so the cursor freezes for seconds at a time. With
       two session slots, one connected mouse still leaves a "free" slot, so
       without this guard scanning keeps alternating LE/inquiry slices right
       alongside the active mouse and produces exactly that multi-second lag.
       While any HID device is streaming, suspend scanning and dedicate the
       radio to it. bt_scan_suspend leaves _scanning set, so an in-progress
       scan session resumes on its own once no HID link is live (e.g. the
       mouse disconnects), keeping autoconnect/reconnect working. */
    if (bt_hid_live() && !_le_autoconnect) {
        if (_scan_slice != BT_SCAN_SLICE_NONE) {
            bt_scan_suspend();
        }
        return;
    }

    /* the radio is only busy when every session slot is taken; with a free
       slot scanning may continue so a second LE device can be discovered */
    if (le_session_free() < 0) {
        return;
    }

    if (!_scanning) {
        _le_autoconnect = false;
        return;
    }
    if (now >= _scan_total_end_ms) {
        bt_scan_slice_stop();
        _scan_slice = BT_SCAN_SLICE_NONE;
        _scan_slice_end_ms = 0;
        _scanning = false;
        _le_autoconnect = false;
        bt_emit("scan_done status=0\n");
        return;
    }
    if (_scan_slice != BT_SCAN_SLICE_NONE && now < _scan_slice_end_ms) {
        return;
    }

    /* the slice expired (or none was armed yet): flip to the other radio
       mode, and only fall back to the one that just ran if the flip
       itself fails. bt_scan_slice_stop leaves _scan_slice alone so the
       choice below still sees which mode is being given up. */
    bt_scan_slice_stop();
    if (_le_autoconnect) {
        /* Known BLE peripherals cannot be found by classic inquiry. Keep
           background reconnect LE-only so it cannot page-scan over HID. */
        if (!bt_scan_enter_le(now)) {
            _scan_slice = BT_SCAN_SLICE_NONE;
            _scan_slice_end_ms = now;
        }
        return;
    }
    want_le = _scan_slice == BT_SCAN_SLICE_NONE ? _le_supported
                                                : _scan_slice != BT_SCAN_SLICE_LE;
    if (want_le ? bt_scan_enter_le(now) : bt_scan_enter_classic(now)) {
        return;
    }
    if (want_le ? bt_scan_enter_classic(now) : bt_scan_enter_le(now)) {
        return;
    }
    _scan_slice = BT_SCAN_SLICE_NONE;
    _scan_slice_end_ms = now; /* both radios refused: retry next tick */
}

/* A scan session alternates LE and classic slices until the requested
   time is up, and bt_le_step drives the alternation. Opening with the LE
   slice is what makes BLE-only mice and keyboards show up at all: they
   never answer a BR/EDR inquiry. */
int bt_start_scan(int seconds) {
    if (!_ready) {
        return -1;
    }
    /* Refuse outright while a bring-up owns the radio (see le_bringup_busy):
       arming a scan here would issue synchronous scan HCI from this dev.cmd
       handler that both aborts the pending LE_Create_Connection and clobbers
       the bring-up's own _wait_cmd. xbt re-fires "scan" every second, so the
       discovery simply resumes once the connection is up or has failed. */
    if (le_bringup_busy()) {
        return -1;
    }
    if (seconds <= 0) {
        seconds = 10;
    }
    if (seconds > 120) {
        seconds = 120;
    }

    bt_scan_suspend();
    _scan_total_end_ms = kernel_tic_ms(0) + (uint64_t)seconds * 1000u;
    _scanning = true;
    bt_le_step(false); /* enter the first slice right away; never run a
                          queued bring-up from here -- this can be reached
                          from the "scan" dev.cmd handler (interrupt ctx),
                          where a blocking bring-up would freeze xbt */

    if (_scan_slice == BT_SCAN_SLICE_NONE && le_session_free() >= 0 &&
            !_le_req_active) {
        _scanning = false;
        return -1;
    }
    return 0;
}

int bt_stop_scan(void) {
    if (!_scanning) {
        return 0;
    }
    bt_scan_slice_stop();
    _scan_slice = BT_SCAN_SLICE_NONE;
    _scan_slice_end_ms = 0;
    _scan_total_end_ms = 0;
    _scanning = false;
    _le_autoconnect = false;
    return 0;
}
