/* btd_known.c - device table, persistent known-device store and
            address / EIR parsing helpers.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

bt_device_t _devices[MAX_BT_DEVICES];

bt_known_t _known[MAX_BT_KNOWN];

bt_pending_t _pending;

uint8_t hci_opcode_lo(uint16_t opcode) {
    return (uint8_t)(opcode & 0xff);
}

uint8_t hci_opcode_hi(uint16_t opcode) {
    return (uint8_t)((opcode >> 8) & 0xff);
}

bool bt_addr_equal(const uint8_t* a, const uint8_t* b) {
    return memcmp(a, b, 6) == 0;
}

void bt_addr_to_str(const uint8_t* addr, char* out, size_t size) {
    snprintf(out, size, "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

bool bt_parse_addr(const char* str, uint8_t* addr) {
    unsigned int b[6];

    if (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
            &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
        return false;
    }

    addr[0] = (uint8_t)b[5];
    addr[1] = (uint8_t)b[4];
    addr[2] = (uint8_t)b[3];
    addr[3] = (uint8_t)b[2];
    addr[4] = (uint8_t)b[1];
    addr[5] = (uint8_t)b[0];
    return true;
}

int bt_pending_matches_addr(const uint8_t* addr) {
    return _pending.type != BT_PENDING_NONE && bt_addr_equal(_pending.addr, addr);
}

void bt_clear_pending(void) {
    memset(&_pending, 0, sizeof(_pending));
}

bt_device_t* bt_find_device(const uint8_t* addr, bool create) {
    int i;
    bt_device_t* free_slot = NULL;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (_devices[i].used) {
            if (bt_addr_equal(_devices[i].addr, addr)) {
                return &_devices[i];
            }
        }
        else if (free_slot == NULL) {
            free_slot = &_devices[i];
        }
    }

    if (!create || free_slot == NULL) {
        return NULL;
    }

    memset(free_slot, 0, sizeof(*free_slot));
    free_slot->used = true;
    memcpy(free_slot->addr, addr, 6);
    free_slot->rssi = 127;
    return free_slot;
}

bt_device_t* bt_find_device_by_handle(uint16_t handle) {
    int i;

    for (i = 0; i < MAX_BT_DEVICES; ++i) {
        if (_devices[i].used && _devices[i].connected && _devices[i].handle == handle) {
            return &_devices[i];
        }
    }
    return NULL;
}

void bt_trim_name(char* name) {
    int len;

    if (name == NULL) {
        return;
    }

    len = (int)strlen(name);
    while (len > 0) {
        unsigned char ch = (unsigned char)name[len - 1];
        if (ch == '\0' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            name[len - 1] = 0;
            --len;
            continue;
        }
        break;
    }
}

/* ---- persistent known-device store (/etc/bt/bt.json) -----------------
   small json doc, one device object per line:
   {"addr":"AA:BB:CC:DD:EE:FF","name":"...","paired":1,"key":"32hex"}
   parsed tolerantly (hand edits survive): fields are looked up between
   each "addr" occurrence and the closing '}' of its object. */

bt_known_t* bt_known_find(const uint8_t* addr) {
    int i;

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        if (_known[i].used && bt_addr_equal(_known[i].addr, addr)) {
            return &_known[i];
        }
    }
    return NULL;
}

bt_known_t* bt_known_find_by_ltk(uint16_t ediv, const uint8_t* rand8) {
    int i;

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        if (_known[i].used && _known[i].le && _known[i].has_ltk &&
                _known[i].ediv == ediv &&
                memcmp(_known[i].ltk_rand, rand8, 8) == 0) {
            return &_known[i];
        }
    }
    return NULL;
}

/* The only stable handle on a peer that rotates an unresolvable private
   address (NRPA): its name. The address changes every rotation and an NRPA
   carries no IRK-resolvable identity, so a bonded LE device is recognised by
   name to keep exactly one bond per physical device. */
bt_known_t* bt_known_find_le_by_name(const char* name) {
    int i;

    if (name == NULL || name[0] == 0) {
        return NULL;
    }
    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        if (_known[i].used && _known[i].le && _known[i].name[0] != 0 &&
                strcmp(_known[i].name, name) == 0) {
            return &_known[i];
        }
    }
    return NULL;
}

/* Collapse duplicate LE bonds left behind by a rotating-address peripheral
   that re-paired on each reconnect: they all share the device name, so keep
   the first and drop the rest. Returns how many were removed. The kept bond
   self-heals on the next reconnect (its address and LTK are refreshed). */
int bt_known_dedup_le(void) {
    int i, j;
    int removed = 0;

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        if (!_known[i].used || !_known[i].le || _known[i].name[0] == 0) {
            continue;
        }
        for (j = i + 1; j < MAX_BT_KNOWN; ++j) {
            if (_known[j].used && _known[j].le &&
                    strcmp(_known[j].name, _known[i].name) == 0) {
                _known[j].used = false;
                ++removed;
            }
        }
    }
    return removed;
}

static bt_known_t* bt_known_upsert(const uint8_t* addr) {
    int i;
    bt_known_t* k = bt_known_find(addr);

    if (k != NULL) {
        return k;
    }
    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        if (!_known[i].used) {
            memset(&_known[i], 0, sizeof(_known[i]));
            _known[i].used = true;
            memcpy(_known[i].addr, addr, 6);
            return &_known[i];
        }
    }
    return NULL;
}

static bool bt_json_str_field(const char* begin, const char* end,
        const char* key, char* out, size_t out_sz) {
    char pat[16];
    const char* p;
    size_t n = 0;

    if (out_sz == 0) {
        return false;
    }
    out[0] = 0;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(begin, pat);
    if (p == NULL || p >= end) {
        return false;
    }
    p += strlen(pat);
    while (p < end && (*p == ' ' || *p == ':')) {
        ++p;
    }
    if (p >= end || *p != '"') {
        return false;
    }
    ++p;
    while (p < end && *p != 0 && n + 1 < out_sz) {
        if (*p == '\\' && (p + 1) < end && (p[1] == '"' || p[1] == '\\')) {
            out[n++] = p[1];
            p += 2;
            continue;
        }
        if (*p == '"') {
            break;
        }
        out[n++] = *p++;
    }
    out[n] = 0;
    return true;
}

static bool bt_json_int_field(const char* begin, const char* end,
        const char* key, int* out) {
    char pat[16];
    const char* p;

    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(begin, pat);
    if (p == NULL || p >= end) {
        return false;
    }
    p += strlen(pat);
    while (p < end && (*p == ' ' || *p == ':')) {
        ++p;
    }
    if (p >= end || !isdigit((unsigned char)*p)) {
        return false;
    }
    *out = atoi(p);
    return true;
}

/* hex helpers sized by the caller, so the same code writes a 16-byte link
   key or LTK and an 8-byte LE diversifier */
static void bt_bytes_to_hex(const uint8_t* b, size_t n, char* out) {
    static const char hexd[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < n; ++i) {
        out[i * 2] = hexd[b[i] >> 4];
        out[i * 2 + 1] = hexd[b[i] & 0x0f];
    }
    out[n * 2] = 0;
}

static bool bt_hex_to_bytes(const char* hex, uint8_t* b, size_t n) {
    size_t i;

    if (strlen(hex) != n * 2) {
        return false;
    }
    for (i = 0; i < n * 2; ++i) {
        char c = hex[i];
        int v;
        if (c >= '0' && c <= '9') {
            v = c - '0';
        }
        else if (c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F') {
            v = c - 'A' + 10;
        }
        else {
            return false;
        }
        if ((i & 1) == 0) {
            b[i / 2] = (uint8_t)(v << 4);
        }
        else {
            b[i / 2] |= (uint8_t)v;
        }
    }
    return true;
}

static void bt_known_sanitize_name(char* name) {
    size_t i;

    for (i = 0; name[i] != 0; ++i) {
        if (!isprint((unsigned char)name[i])) {
            name[i] = '.';
        }
    }
}

int bt_known_load(void) {
    char* buf;
    char* p;
    int fd;
    int n;
    int count = 0;

    fd = open(BT_KNOWN_FILE, O_RDONLY);
    if (fd < 0) {
        return 0; /* no store yet, not an error */
    }
    buf = (char*)malloc(BT_KNOWN_MAX_FILE + 1);
    if (buf == NULL) {
        close(fd);
        return 0;
    }
    n = (int)read(fd, buf, BT_KNOWN_MAX_FILE);
    close(fd);
    if (n <= 0) {
        free(buf);
        return 0;
    }
    buf[n] = 0;

    p = buf;
    while (count < MAX_BT_KNOWN && (p = strstr(p, "\"addr\"")) != NULL) {
        char* obj_end = strchr(p, '}');
        char addr_str[24];
        char hex[40];
        uint8_t addr[6];
        bt_known_t* k;
        int paired = 0;
        int le = 0;
        int atype = 0;
        int ediv = 0;
        int idtype = 0;

        if (obj_end == NULL) {
            break;
        }
        if (bt_json_str_field(p, obj_end, "addr", addr_str, sizeof(addr_str)) &&
                bt_parse_addr(addr_str, addr) &&
                (k = bt_known_upsert(addr)) != NULL) {
            bt_json_str_field(p, obj_end, "name", k->name, sizeof(k->name));
            bt_known_sanitize_name(k->name);
            bt_trim_name(k->name);
            if (bt_json_int_field(p, obj_end, "paired", &paired) && paired) {
                k->paired = true;
            }
            if (bt_json_str_field(p, obj_end, "key", hex, sizeof(hex)) &&
                    bt_hex_to_bytes(hex, k->key, sizeof(k->key))) {
                k->has_key = true;
                k->paired = true;
            }
            if (bt_json_int_field(p, obj_end, "le", &le) && le) {
                k->le = true;
            }
            if (bt_json_int_field(p, obj_end, "atype", &atype)) {
                k->addr_type = (uint8_t)atype;
            }
            /* an LTK on its own cannot answer an LE Long Term Key Request,
               so the EDIV and the diversifier stand or fall with it; hex
               is reused safely because && evaluates left to right */
            if (bt_json_str_field(p, obj_end, "ltk", hex, sizeof(hex)) &&
                    bt_hex_to_bytes(hex, k->ltk, sizeof(k->ltk)) &&
                    bt_json_int_field(p, obj_end, "ediv", &ediv) &&
                    bt_json_str_field(p, obj_end, "lrand", hex, sizeof(hex)) &&
                    bt_hex_to_bytes(hex, k->ltk_rand, sizeof(k->ltk_rand))) {
                k->ediv = (uint16_t)ediv;
                k->has_ltk = true;
                k->paired = true;
            }
            /* the IRK + identity address let the resolving list collapse a
               rotating private address back to one stable identity at boot;
               hex/addr_str are reused safely (&& is left to right) */
            if (bt_json_str_field(p, obj_end, "irk", hex, sizeof(hex)) &&
                    bt_hex_to_bytes(hex, k->irk, sizeof(k->irk))) {
                k->has_irk = true;
            }
            if (bt_json_str_field(p, obj_end, "idaddr", addr_str,
                    sizeof(addr_str)) &&
                    bt_parse_addr(addr_str, k->id_addr)) {
                k->has_id_addr = true;
                if (bt_json_int_field(p, obj_end, "idtype", &idtype)) {
                    k->id_addr_type = (uint8_t)idtype;
                }
            }
            ++count;
        }
        p = obj_end + 1;
    }
    free(buf);
    return count;
}

static void bt_json_write_escaped(int fd, const char* str) {
    const char* p;

    for (p = str; *p != 0; ++p) {
        if (*p == '"' || *p == '\\') {
            char esc[2];
            esc[0] = '\\';
            esc[1] = *p;
            write(fd, esc, 2);
        }
        else {
            write(fd, p, 1);
        }
    }
}

int bt_known_save(void) {
    char line[384];
    char hex[40];
    char ltkhex[40];
    char lrandhex[24];
    char irkhex[40];
    char idaddr[24];
    int fd;
    int i;
    int written = 0;

    if (mkdir(BT_KNOWN_DIR, 0755) != 0 && errno != EEXIST) {
        slog("bluetooth store mkdir_failed dir=%s errno=%d\n", BT_KNOWN_DIR, errno);
    }
    fd = open(BT_KNOWN_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        slog("bluetooth store save_failed path=%s errno=%d\n", BT_KNOWN_FILE, errno);
        return -1;
    }
    write(fd, "{\n  \"devices\": [\n", 17);
    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        char addr[24];
        int len;

        if (!_known[i].used) {
            continue;
        }
        bt_addr_to_str(_known[i].addr, addr, sizeof(addr));
        if (_known[i].has_key) {
            bt_bytes_to_hex(_known[i].key, sizeof(_known[i].key), hex);
        }
        else {
            hex[0] = 0;
        }
        if (_known[i].has_ltk) {
            bt_bytes_to_hex(_known[i].ltk, sizeof(_known[i].ltk), ltkhex);
            bt_bytes_to_hex(_known[i].ltk_rand, sizeof(_known[i].ltk_rand),
                    lrandhex);
        }
        else {
            ltkhex[0] = 0;
            lrandhex[0] = 0;
        }
        if (_known[i].has_irk) {
            bt_bytes_to_hex(_known[i].irk, sizeof(_known[i].irk), irkhex);
        }
        else {
            irkhex[0] = 0;
        }
        if (_known[i].has_id_addr) {
            bt_addr_to_str(_known[i].id_addr, idaddr, sizeof(idaddr));
        }
        else {
            idaddr[0] = 0;
        }
        len = snprintf(line, sizeof(line),
            "%s    {\"addr\":\"%s\",\"name\":\"", written > 0 ? ",\n" : "", addr);
        write(fd, line, len);
        bt_json_write_escaped(fd, _known[i].name);
        len = snprintf(line, sizeof(line),
            "\",\"paired\":%d,\"key\":\"%s\",\"le\":%d,\"atype\":%u,"
            "\"ltk\":\"%s\",\"ediv\":%u,\"lrand\":\"%s\","
            "\"irk\":\"%s\",\"idaddr\":\"%s\",\"idtype\":%u}\n",
            _known[i].paired ? 1 : 0, hex, _known[i].le ? 1 : 0,
            (unsigned)_known[i].addr_type, ltkhex, (unsigned)_known[i].ediv,
            lrandhex, irkhex, idaddr, (unsigned)_known[i].id_addr_type);
        write(fd, line, len);
        ++written;
    }
    write(fd, "  ]\n}\n", 6);
    close(fd);
    return 0;
}

/* record a device we just connected/paired with (link key included when
   we have one, so the next power-on reconnects without re-pairing) */
void bt_known_touch_from_device(const bt_device_t* dev) {
    bt_known_t* k = NULL;

    /* A rotating-private-address (NRPA) LE peripheral lands on a fresh address
       every reconnect; keying its bond by address would add another entry for
       the SAME device each time - the duplicate list xbt shows. Once we know
       its name, collapse onto the existing same-name LE bond and move that
       bond onto the address we are talking to now, dropping the stale device
       entry seeded from the previous rotation. */
    if (dev->le && dev->name[0] != 0) {
        k = bt_known_find_le_by_name(dev->name);
        if (k != NULL && !bt_addr_equal(k->addr, dev->addr)) {
            bt_device_t* stale = bt_find_device(k->addr, false);
            if (stale != NULL && !stale->connected &&
                    !bt_addr_equal(stale->addr, dev->addr)) {
                stale->used = false;
            }
            memcpy(k->addr, dev->addr, 6);
            k->addr_type = dev->addr_type;
        }
    }
    if (k == NULL) {
        k = bt_known_upsert(dev->addr);
    }

    if (k == NULL) {
        return;
    }
    if (dev->name[0] != 0) {
        strncpy(k->name, dev->name, sizeof(k->name) - 1);
        k->name[sizeof(k->name) - 1] = 0;
    }
    if (dev->has_link_key) {
        k->paired = true;
        k->has_key = true;
        memcpy(k->key, dev->link_key, 16);
    }
    if (dev->le) {
        k->le = true;
        k->addr_type = dev->addr_type;
    }
    if (dev->has_ltk) {
        k->paired = true;
        k->has_ltk = true;
        memcpy(k->ltk, dev->ltk, 16);
        k->ediv = dev->ediv;
        memcpy(k->ltk_rand, dev->ltk_rand, 8);
    }
    if (dev->has_irk) {
        k->has_irk = true;
        memcpy(k->irk, dev->irk, 16);
    }
    if (dev->has_id_addr) {
        k->has_id_addr = true;
        memcpy(k->id_addr, dev->id_addr, 6);
        k->id_addr_type = dev->id_addr_type;
    }
    bt_known_save();
}

/* push stored link keys/names into the runtime cache so the controller's
   LINK_KEY_REQUEST can be answered even before any scan ran */
void bt_known_seed_devices(void) {
    int i;

    for (i = 0; i < MAX_BT_KNOWN; ++i) {
        bt_device_t* dev;

        if (!_known[i].used) {
            continue;
        }
        dev = bt_find_device(_known[i].addr, true);
        if (dev == NULL) {
            continue;
        }
        if (_known[i].name[0] != 0 && dev->name[0] == 0) {
            strncpy(dev->name, _known[i].name, sizeof(dev->name) - 1);
        }
        if (_known[i].has_key) {
            memcpy(dev->link_key, _known[i].key, 16);
            dev->has_link_key = true;
        }
        /* the address type travels with the bond: LE_Create_Connection
           takes both and picking the wrong one finds nothing */
        if (_known[i].le) {
            dev->le = true;
            dev->addr_type = _known[i].addr_type;
        }
        if (_known[i].has_ltk) {
            memcpy(dev->ltk, _known[i].ltk, 16);
            dev->ediv = _known[i].ediv;
            memcpy(dev->ltk_rand, _known[i].ltk_rand, 8);
            dev->has_ltk = true;
        }
        if (_known[i].has_irk) {
            memcpy(dev->irk, _known[i].irk, 16);
            dev->has_irk = true;
        }
        if (_known[i].has_id_addr) {
            memcpy(dev->id_addr, _known[i].id_addr, 6);
            dev->id_addr_type = _known[i].id_addr_type;
            dev->has_id_addr = true;
        }
        if (dev->page_scan_rep_mode == 0) {
            dev->page_scan_rep_mode = 1; /* R1, the common case */
        }
    }
}

void bt_parse_eir_name(const uint8_t* eir, size_t len, char* out, size_t out_sz) {
    size_t pos = 0;

    if (out_sz == 0) {
        return;
    }
    out[0] = 0;

    while (pos < len) {
        uint8_t field_len = eir[pos];
        uint8_t field_type;
        size_t copy_len;
        size_t i;

        if (field_len == 0) {
            break;
        }
        if ((pos + 1 + field_len) > len) {
            break;
        }

        field_type = eir[pos + 1];
        if (field_type == 0x08 || field_type == 0x09) {
            copy_len = field_len - 1;
            if (copy_len >= out_sz) {
                copy_len = out_sz - 1;
            }
            for (i = 0; i < copy_len; ++i) {
                unsigned char ch = eir[pos + 2 + i];
                out[i] = isprint(ch) ? (char)ch : '.';
            }
            out[copy_len] = 0;
            bt_trim_name(out);
            return;
        }
        pos += (size_t)field_len + 1;
    }
}

/* name= stays the last field on purpose: it may contain spaces, and xbt
   reads everything after it up to the end of the line. le= is what lets
   bt_moused accept a BLE-only mouse whose Class of Device is zero. */
void bt_emit_device_line(const char* prefix, const bt_device_t* dev) {
    char addr[24];

    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    bt_emit("%s %s class=0x%06X rssi=%d connected=%d paired=%d le=%d "
        "appearance=%u name=%s\n",
        prefix,
        addr,
        dev->class_of_device & 0xffffffu,
        (int)dev->rssi,
        dev->connected ? 1 : 0,
        (dev->has_link_key || dev->has_ltk) ? 1 : 0,
        dev->le ? 1 : 0,
        (unsigned)dev->appearance,
        dev->name[0] ? dev->name : "-");
}

void bt_ret_append_device_line(int dev_id, char* ret, size_t ret_sz,
        const char* prefix, const bt_device_t* dev) {
    char addr[24];

    bt_addr_to_str(dev->addr, addr, sizeof(addr));
    bt_ret_append(ret, ret_sz,
        "%d: %s %s class=0x%06X rssi=%d connected=%d paired=%d le=%d "
        "appearance=%u name=%s\n",
        dev_id,
        prefix,
        addr,
        dev->class_of_device & 0xffffffu,
        (int)dev->rssi,
        dev->connected ? 1 : 0,
        (dev->has_link_key || dev->has_ltk) ? 1 : 0,
        dev->le ? 1 : 0,
        (unsigned)dev->appearance,
        dev->name[0] ? dev->name : "-");
}
