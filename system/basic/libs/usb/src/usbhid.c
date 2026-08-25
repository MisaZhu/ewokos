/*
 * usbhid.c: hardware-independent USB HID report-descriptor parsing.
 *
 * Extracted verbatim from the usbhostd implementations (raspi5 xHCI
 * variant plus the raspix keyboard report-ID lookup): device type
 * detection, composite report-ID discovery, touch/mouse bit-layout
 * extraction and report normalization into /dev/hid0 event formats.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <usb/usb_defs.h>
#include <usb/usbhid.h>

static void clear_local_usages(uint32_t* usages, int* usage_count, bool* usage_range_valid) {
    (void)usages;
    *usage_count = 0;
    *usage_range_valid = false;
}

static uint32_t hid_usage_for_index(const uint32_t* usages, int usage_count,
        bool usage_range_valid, uint32_t usage_min, uint32_t usage_max, int idx) {
    if (usage_count > 0) {
        if (idx < usage_count) {
            return usages[idx];
        }
        return usages[usage_count - 1];
    }
    if (usage_range_valid) {
        uint32_t usage = usage_min + (uint32_t)idx;
        if (usage > usage_max) {
            usage = usage_max;
        }
        return usage;
    }
    return 0xFFFFFFFFu;
}

static int32_t hid_sign_extend(uint32_t value, int bits) {
    if (bits <= 0 || bits >= 32) {
        return (int32_t)value;
    }
    if ((value & (1u << (bits - 1))) != 0) {
        value |= ~((1u << bits) - 1u);
    }
    return (int32_t)value;
}

hid_dev_type_t hid_detect_device_type(const uint8_t* desc, int len) {
    uint32_t usage_page = 0;
    uint32_t usages[USB_MAX_USAGE_LIST];
    int usage_count = 0;
    uint32_t usage_min = 0;
    uint32_t usage_max = 0;
    bool usage_range_valid = false;
    bool found_keyboard = false;
    bool found_mouse = false;
    bool found_touch = false;

    for (int off = 0; off < len; ) {
        uint8_t prefix = desc[off++];
        uint32_t value = 0;
        int size_code, size, type, tag;

        if (prefix == 0xFE) {
            if (off + 2 > len) break;
            size = desc[off];
            off += 2 + size;
            continue;
        }

        size_code = prefix & 0x3;
        size = (size_code == 3) ? 4 : size_code;
        type = (prefix >> 2) & 0x3;
        tag = (prefix >> 4) & 0xF;
        if (off + size > len) break;
        for (int i = 0; i < size; ++i) {
            value |= (uint32_t)desc[off + i] << (i * 8);
        }
        off += size;

        if (type == 1) {
            if (tag == 0) {
                usage_page = value;
            }
        }
        else if (type == 2) {
            switch (tag) {
            case 0:
                if (usage_count < USB_MAX_USAGE_LIST) {
                    usages[usage_count++] = value;
                }
                break;
            case 1:
                usage_min = value;
                usage_range_valid = true;
                break;
            case 2:
                usage_max = value;
                usage_range_valid = true;
                break;
            }
        }
        else if (type == 0) {
            if (tag == 10) {
                uint8_t collection_type = (uint8_t)value;
                if (collection_type == 1 && usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP) {
                    uint32_t usage = hid_usage_for_index(usages, usage_count,
                            usage_range_valid, usage_min, usage_max, 0);
                    if (usage == HID_USAGE_KEYBOARD) {
                        found_keyboard = true;
                    }
                    else if (usage == HID_USAGE_MOUSE || usage == HID_USAGE_POINTER) {
                        found_mouse = true;
                    }
                }
                else if (collection_type == 1 && usage_page == HID_USAGE_PAGE_DIGITIZER) {
                    uint32_t usage = hid_usage_for_index(usages, usage_count,
                            usage_range_valid, usage_min, usage_max, 0);
                    if (usage == HID_USAGE_TOUCH_SCREEN || usage == HID_USAGE_TOUCH_PAD ||
                            usage == HID_USAGE_FINGER) {
                        found_touch = true;
                    }
                }
            }
            usage_count = 0;
            usage_range_valid = false;
        }
    }
    /*
     * Some USB touch panels expose both Generic Desktop mouse/pointer and
     * Digitizer touch collections in one report descriptor. Prefer the
     * explicit digitizer collection, otherwise hid_touchd never gets data.
     */
    if (found_touch) {
        return HID_DEV_TYPE_TOUCH;
    }
    if (found_keyboard) {
        return HID_DEV_TYPE_KEYBOARD;
    }
    if (found_mouse) {
        return HID_DEV_TYPE_MOUSE;
    }
    return HID_DEV_TYPE_UNKNOWN;
}

/* Detect a composite interface: kbd+mouse collections share one interrupt
   endpoint, distinguished by report IDs. Returns 0 with both IDs filled
   when the descriptor holds a keyboard AND a mouse/pointer application
   collection each with its own report ID. */
int hid_parse_report_ids(const uint8_t* desc, int len,
        uint8_t* kbd_id, uint8_t* mouse_id) {
    uint32_t usage_page = 0;
    uint32_t usages[USB_MAX_USAGE_LIST];
    int usage_count = 0;
    int depth = 0;
    hid_dev_type_t cur_app = HID_DEV_TYPE_UNKNOWN;
    bool kbd_found = false, mouse_found = false;

    for (int off = 0; off < len; ) {
        uint8_t prefix = desc[off++];
        uint32_t value = 0;
        int size_code, size, type, tag;

        if (prefix == 0xFE) {
            if (off + 2 > len) break;
            size = desc[off];
            off += 2 + size;
            continue;
        }
        size_code = prefix & 0x3;
        size = (size_code == 3) ? 4 : size_code;
        type = (prefix >> 2) & 0x3;
        tag = (prefix >> 4) & 0xF;
        if (off + size > len) break;
        for (int i = 0; i < size; ++i) {
            value |= (uint32_t)desc[off + i] << (i * 8);
        }
        off += size;

        if (type == 1) { /* global */
            if (tag == 0) {
                usage_page = value;
            }
            else if (tag == 8) { /* Report ID */
                /* only IDs declared inside the collection count: a global
                   Report ID from a preceding collection (consumer/joystick)
                   must not leak into the next one */
                if (cur_app == HID_DEV_TYPE_KEYBOARD && !kbd_found) {
                    *kbd_id = (uint8_t)value;
                    kbd_found = true;
                }
                else if (cur_app == HID_DEV_TYPE_MOUSE && !mouse_found) {
                    *mouse_id = (uint8_t)value;
                    mouse_found = true;
                }
            }
        }
        else if (type == 2) { /* local */
            if (tag == 0 && usage_count < USB_MAX_USAGE_LIST) {
                usages[usage_count++] = value;
            }
        }
        else if (type == 0) { /* main */
            if (tag == 10) { /* Collection */
                if (depth == 0 && (uint8_t)value == 1 &&
                        usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP && usage_count > 0) {
                    if (usages[0] == HID_USAGE_KEYBOARD) {
                        cur_app = HID_DEV_TYPE_KEYBOARD;
                    }
                    else if (usages[0] == HID_USAGE_MOUSE || usages[0] == HID_USAGE_POINTER) {
                        cur_app = HID_DEV_TYPE_MOUSE;
                    }
                    else {
                        cur_app = HID_DEV_TYPE_UNKNOWN;
                    }
                }
                depth++;
            }
            else if (tag == 12) { /* End Collection */
                if (depth > 0) {
                    depth--;
                }
                if (depth == 0) {
                    cur_app = HID_DEV_TYPE_UNKNOWN;
                }
            }
            usage_count = 0;
        }
    }
    if (kbd_found && mouse_found && *kbd_id != *mouse_id) {
        return 0;
    }
    return -1;
}

/* Find the Report ID used by the keyboard application collection, if any.
   Many external keyboards multiplex kbd+consumer reports behind IDs; their
   interrupt data then carries a leading ID byte which must be stripped
   before the plain [mod,res,key[6]] decode.  Returns 0 when the keyboard
   collection has no Report ID (plain boot layout). */
uint8_t hid_find_kbd_report_id(const uint8_t* desc, int len) {
    uint32_t usage_page = 0;
    uint32_t usages[USB_MAX_USAGE_LIST];
    int usage_count = 0;
    int depth = 0;
    bool in_kbd = false;

    for (int off = 0; off < len; ) {
        uint8_t prefix = desc[off++];
        uint32_t value = 0;
        int size_code, size, type, tag;

        if (prefix == 0xFE) {
            if (off + 2 > len) break;
            size = desc[off];
            off += 2 + size;
            continue;
        }
        size_code = prefix & 0x3;
        size = (size_code == 3) ? 4 : size_code;
        type = (prefix >> 2) & 0x3;
        tag = (prefix >> 4) & 0xF;
        if (off + size > len) break;
        for (int i = 0; i < size; ++i) {
            value |= (uint32_t)desc[off + i] << (i * 8);
        }
        off += size;

        if (type == 1) { /* global */
            if (tag == 0) {
                usage_page = value;
            }
            else if (tag == 8 && in_kbd) { /* Report ID inside kbd collection */
                return (uint8_t)value;
            }
        }
        else if (type == 2) { /* local */
            if (tag == 0 && usage_count < USB_MAX_USAGE_LIST) {
                usages[usage_count++] = value;
            }
        }
        else if (type == 0) { /* main */
            if (tag == 10) { /* Collection */
                if (depth == 0 && (uint8_t)value == 1 &&
                        usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP && usage_count > 0 &&
                        usages[0] == HID_USAGE_KEYBOARD) {
                    in_kbd = true;
                }
                depth++;
            }
            else if (tag == 12) { /* End Collection */
                if (depth > 0) {
                    depth--;
                }
                if (depth == 0) {
                    in_kbd = false;
                }
            }
            usage_count = 0;
        }
    }
    return 0;
}

int hid_parse_touch_report(const uint8_t* desc, int len, touch_parser_t* out) {
    uint32_t usages[USB_MAX_USAGE_LIST];
    int usage_count = 0;
    uint32_t usage_page = 0;
    uint32_t usage_min = 0;
    uint32_t usage_max = 0;
    bool usage_range_valid = false;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    uint8_t current_report_id = 0;
    uint32_t report_bits[256];
    int collection_depth = 0;
    int touch_collection_depth = -1;
    bool touch_active = false;
    int32_t logical_max = 0;

    memset(report_bits, 0, sizeof(report_bits));
    memset(out, 0, sizeof(*out));
    out->tip_bit = -1;
    out->x_bit = -1;
    out->y_bit = -1;

    for (int off = 0; off < len; ) {
        uint8_t prefix = desc[off++];
        uint32_t value = 0;
        int size_code;
        int size;
        int type;
        int tag;

        if (prefix == 0xFE) {
            if (off + 2 > len) {
                break;
            }
            size = desc[off];
            off += 2;
            off += size;
            continue;
        }

        size_code = prefix & 0x3;
        size = (size_code == 3) ? 4 : size_code;
        type = (prefix >> 2) & 0x3;
        tag = (prefix >> 4) & 0xF;
        if (off + size > len) {
            break;
        }
        for (int i = 0; i < size; ++i) {
            value |= (uint32_t)desc[off + i] << (i * 8);
        }
        off += size;

        if (type == 1) {
            switch (tag) {
            case 0:
                usage_page = value;
                break;
            case 1:
                (void)hid_sign_extend(value, size * 8);
                break;
            case 2:
                logical_max = hid_sign_extend(value, size * 8);
                break;
            case 7:
                report_size = value;
                break;
            case 8:
                current_report_id = (uint8_t)value;
                if (report_bits[current_report_id] == 0) {
                    report_bits[current_report_id] = 8;
                }
                break;
            case 9:
                report_count = value;
                break;
            default:
                break;
            }
        }
        else if (type == 2) {
            switch (tag) {
            case 0:
                if (usage_count < USB_MAX_USAGE_LIST) {
                    usages[usage_count++] = value;
                }
                break;
            case 1:
                usage_min = value;
                usage_range_valid = true;
                break;
            case 2:
                usage_max = value;
                usage_range_valid = true;
                break;
            default:
                break;
            }
        }
        else if (type == 0) {
            switch (tag) {
            case 8: {
                bool constant = (value & 0x1u) != 0;
                bool variable = (value & 0x2u) != 0;

                if (touch_active && !constant && variable) {
                    for (uint32_t idx = 0; idx < report_count; ++idx) {
                        uint32_t usage = hid_usage_for_index(usages, usage_count,
                                usage_range_valid, usage_min, usage_max, (int)idx);
                        int bit = (int)report_bits[current_report_id] + (int)(idx * report_size);

                        if (usage_page == HID_USAGE_PAGE_DIGITIZER && usage == HID_USAGE_TIP_SWITCH) {
                            if (out->tip_bit < 0) {
                                out->tip_bit = bit;
                                out->tip_size = (int)report_size;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                        else if (usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP && usage == HID_USAGE_X) {
                            if (out->x_bit < 0) {
                                out->x_bit = bit;
                                out->x_size = (int)report_size;
                                out->x_max = logical_max > 0 ? (uint32_t)logical_max : 0;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                        else if (usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP && usage == HID_USAGE_Y) {
                            if (out->y_bit < 0) {
                                out->y_bit = bit;
                                out->y_size = (int)report_size;
                                out->y_max = logical_max > 0 ? (uint32_t)logical_max : 0;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                    }
                }
                report_bits[current_report_id] += report_size * report_count;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 10: {
                uint32_t usage = hid_usage_for_index(usages, usage_count,
                        usage_range_valid, usage_min, usage_max, 0);
                if (usage_page == HID_USAGE_PAGE_DIGITIZER &&
                        (usage == HID_USAGE_TOUCH_SCREEN ||
                         usage == HID_USAGE_TOUCH_PAD ||
                         usage == HID_USAGE_FINGER)) {
                    touch_collection_depth = collection_depth + 1;
                    touch_active = true;
                }
                collection_depth++;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 12:
                if (collection_depth == touch_collection_depth) {
                    touch_active = false;
                    touch_collection_depth = -1;
                }
                if (collection_depth > 0) {
                    collection_depth--;
                }
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            default:
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
        }
    }

    if (out->tip_bit < 0 || out->x_bit < 0 || out->y_bit < 0) {
        return -1;
    }

    out->valid = true;
    out->report_bytes = (uint8_t)((report_bits[out->report_id] + 7u) / 8u);
    if (out->report_bytes == 0 || out->report_bytes > USB_MAX_REPORT) {
        return -1;
    }
    return 0;
}

bool hid_probe_touch_report(const uint8_t* desc, int len, touch_parser_t* out) {
    touch_parser_t parser;

    if (hid_parse_touch_report(desc, len, &parser) != 0) {
        return false;
    }
    if (out != NULL) {
        *out = parser;
    }
    return true;
}

int hid_parse_mouse_report(const uint8_t* desc, int len, mouse_parser_t* out) {
    uint32_t usages[USB_MAX_USAGE_LIST];
    int usage_count = 0;
    uint32_t usage_page = 0;
    uint32_t usage_min = 0;
    uint32_t usage_max = 0;
    bool usage_range_valid = false;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    uint8_t current_report_id = 0;
    uint32_t report_bits[256];
    int collection_depth = 0;
    int mouse_collection_depth = -1;
    bool mouse_active = false;
    int selected_report_id = -1;

    memset(report_bits, 0, sizeof(report_bits));
    memset(out, 0, sizeof(*out));
    for (int i = 0; i < 3; ++i) {
        out->button_bit[i] = -1;
    }
    out->x_bit = -1;
    out->y_bit = -1;
    out->wheel_bit = -1;

    for (int off = 0; off < len; ) {
        uint8_t prefix = desc[off++];
        uint32_t value = 0;
        int size_code;
        int size;
        int type;
        int tag;

        if (prefix == 0xFE) {
            if (off + 2 > len) {
                break;
            }
            size = desc[off];
            off += 2 + size;
            continue;
        }

        size_code = prefix & 0x3;
        size = (size_code == 3) ? 4 : size_code;
        type = (prefix >> 2) & 0x3;
        tag = (prefix >> 4) & 0xF;
        if (off + size > len) {
            break;
        }
        for (int i = 0; i < size; ++i) {
            value |= (uint32_t)desc[off + i] << (i * 8);
        }
        off += size;

        if (type == 1) {
            switch (tag) {
            case 0:
                usage_page = value;
                break;
            case 7:
                report_size = value;
                break;
            case 8:
                current_report_id = (uint8_t)value;
                if (report_bits[current_report_id] == 0) {
                    report_bits[current_report_id] = 8;
                }
                break;
            case 9:
                report_count = value;
                break;
            default:
                break;
            }
        }
        else if (type == 2) {
            switch (tag) {
            case 0:
                if (usage_count < USB_MAX_USAGE_LIST) {
                    usages[usage_count++] = value;
                }
                break;
            case 1:
                usage_min = value;
                usage_range_valid = true;
                break;
            case 2:
                usage_max = value;
                usage_range_valid = true;
                break;
            default:
                break;
            }
        }
        else if (type == 0) {
            switch (tag) {
            case 8: {
                bool constant = (value & 0x1u) != 0;
                bool variable = (value & 0x2u) != 0;

                if (mouse_active && !constant && variable) {
                    bool report_match = (selected_report_id < 0) ||
                        (selected_report_id == (int)current_report_id);

                    for (uint32_t idx = 0; idx < report_count; ++idx) {
                        uint32_t usage = hid_usage_for_index(usages, usage_count,
                                usage_range_valid, usage_min, usage_max, (int)idx);
                        int bit = (int)report_bits[current_report_id] + (int)(idx * report_size);

                        if (!report_match) {
                            continue;
                        }
                        if (usage_page == HID_USAGE_PAGE_BUTTON &&
                                usage >= 1u && usage <= 3u) {
                            int btn_idx = (int)usage - 1;
                            if (out->button_bit[btn_idx] < 0) {
                                if (selected_report_id < 0) {
                                    selected_report_id = (int)current_report_id;
                                }
                                out->button_bit[btn_idx] = bit;
                                out->button_size[btn_idx] = (int)report_size;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                        else if (usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP &&
                                usage == HID_USAGE_X && out->x_bit < 0) {
                            if (selected_report_id < 0) {
                                selected_report_id = (int)current_report_id;
                            }
                            out->x_bit = bit;
                            out->x_size = (int)report_size;
                            out->has_report_id = current_report_id != 0;
                            out->report_id = current_report_id;
                        }
                        else if (usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP &&
                                usage == HID_USAGE_Y && out->y_bit < 0) {
                            if (selected_report_id < 0) {
                                selected_report_id = (int)current_report_id;
                            }
                            out->y_bit = bit;
                            out->y_size = (int)report_size;
                            out->has_report_id = current_report_id != 0;
                            out->report_id = current_report_id;
                        }
                        else if (usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP &&
                                usage == HID_USAGE_WHEEL && out->wheel_bit < 0) {
                            if (selected_report_id < 0) {
                                selected_report_id = (int)current_report_id;
                            }
                            out->wheel_bit = bit;
                            out->wheel_size = (int)report_size;
                            out->has_report_id = current_report_id != 0;
                            out->report_id = current_report_id;
                        }
                    }
                }
                report_bits[current_report_id] += report_size * report_count;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 10: {
                uint32_t usage = hid_usage_for_index(usages, usage_count,
                        usage_range_valid, usage_min, usage_max, 0);
                uint8_t collection_type = (uint8_t)value;

                if (!mouse_active && collection_type == 1 &&
                        usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP &&
                        (usage == HID_USAGE_MOUSE || usage == HID_USAGE_POINTER)) {
                    mouse_collection_depth = collection_depth + 1;
                    mouse_active = true;
                }
                collection_depth++;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 12:
                if (collection_depth == mouse_collection_depth) {
                    mouse_active = false;
                    mouse_collection_depth = -1;
                }
                if (collection_depth > 0) {
                    collection_depth--;
                }
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            default:
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
        }
    }

    if (selected_report_id < 0 || out->x_bit < 0 || out->y_bit < 0) {
        return -1;
    }

    out->valid = true;
    out->report_bytes = (uint8_t)((report_bits[out->report_id] + 7u) / 8u);
    if (out->report_bytes == 0 || out->report_bytes > USB_MAX_REPORT) {
        return -1;
    }
    return 0;
}

bool hid_probe_mouse_report(const uint8_t* desc, int len, mouse_parser_t* out) {
    mouse_parser_t parser;

    if (hid_parse_mouse_report(desc, len, &parser) != 0) {
        return false;
    }
    if (out != NULL) {
        *out = parser;
    }
    return true;
}

static uint32_t bit_extract_le(const uint8_t* buf, int bit, int bits) {
    uint32_t value = 0;
    for (int i = 0; i < bits; ++i) {
        int off = bit + i;
        if ((buf[off / 8] & (1u << (off % 8))) != 0) {
            value |= 1u << i;
        }
    }
    return value;
}

static int8_t hid_clamp_s8(int32_t value) {
    if (value > 127) {
        return 127;
    }
    if (value < -128) {
        return -128;
    }
    return (int8_t)value;
}

/* A wrong parser result (garbled descriptor read, unusual descriptor) makes
   normalize decode X/Y from the wrong bits -- typically X still looks fine
   while Y reads a padding/wheel field and stays 0 or barely moves.  Only
   trust the parser when the layout is plausible.  strict is used for boot
   interfaces where we can fall back to the guaranteed [btn,dx,dy,wheel]
   boot layout instead. */
bool mouse_parser_sane(const mouse_parser_t* p, uint16_t max_packet, bool strict) {
    uint32_t total = (uint32_t)p->report_bytes * 8u;

    if (p->x_bit < 0 || p->y_bit < 0 || p->x_size <= 0 || p->y_size <= 0) {
        return false;
    }
    if (p->x_size > 32 || p->y_size > 32) {
        return false;
    }
    if ((uint32_t)p->x_bit + (uint32_t)p->x_size > total ||
            (uint32_t)p->y_bit + (uint32_t)p->y_size > total) {
        return false;
    }
    if (p->x_bit == p->y_bit) {
        return false;
    }
    if (p->has_report_id && p->report_bytes < 2) {
        return false;
    }
    /* the whole report must arrive in one interrupt IN packet, otherwise
       the bit count was miscomputed while parsing */
    if (max_packet > 0 && p->report_bytes > max_packet) {
        return false;
    }
    if (strict) {
        if ((p->x_bit % 8) != 0 || (p->y_bit % 8) != 0) {
            return false;
        }
        if (p->x_size != 8 && p->x_size != 16) {
            return false;
        }
        if (p->y_size != 8 && p->y_size != 16) {
            return false;
        }
    }
    return true;
}

int mouse_normalize_report(const mouse_parser_t* m,
        const uint8_t* report, int len, uint8_t* out) {
    uint8_t buttons = 0;
    int32_t x;
    int32_t y;
    int32_t wheel = 0;

    if (!m->valid) {
        return -1;
    }
    if (m->has_report_id) {
        if (len <= 0 || report[0] != m->report_id) {
            return -1;
        }
    }
    if (len < m->report_bytes) {
        return -1;
    }

    for (int i = 0; i < 3; ++i) {
        if (m->button_bit[i] >= 0 &&
                bit_extract_le(report, m->button_bit[i], m->button_size[i]) != 0) {
            buttons |= (uint8_t)(1u << i);
        }
    }

    x = hid_sign_extend(bit_extract_le(report, m->x_bit, m->x_size), m->x_size);
    y = hid_sign_extend(bit_extract_le(report, m->y_bit, m->y_size), m->y_size);
    if (m->wheel_bit >= 0) {
        wheel = hid_sign_extend(bit_extract_le(report, m->wheel_bit, m->wheel_size),
                m->wheel_size);
    }

    memset(out, 0, USB_POINTER_EVENT_SIZE);
    out[0] = buttons;
    out[1] = (uint8_t)hid_clamp_s8(x);
    out[2] = (uint8_t)hid_clamp_s8(y);
    out[3] = (uint8_t)hid_clamp_s8(wheel);
    return USB_POINTER_EVENT_SIZE;
}

int touch_normalize_report(const touch_parser_t* t, uint8_t report_len,
        const uint8_t* report, int len, uint8_t* out) {
    bool pressed;
    uint32_t x;
    uint32_t y;

    if (!t->valid) {
        return -1;
    }
    if (t->has_report_id) {
        if (len <= 0 || report[0] != t->report_id) {
            return -1;
        }
    }
    if (len < report_len) {
        return -1;
    }

    pressed = bit_extract_le(report, t->tip_bit, t->tip_size) != 0;
    x = bit_extract_le(report, t->x_bit, t->x_size);
    y = bit_extract_le(report, t->y_bit, t->y_size);
    if (x > 0xFFFFu) {
        x = 0xFFFFu;
    }
    if (y > 0xFFFFu) {
        y = 0xFFFFu;
    }

    out[0] = pressed ? 1 : 0;
    out[1] = (uint8_t)(x & 0xFFu);
    out[2] = (uint8_t)((x >> 8) & 0xFFu);
    out[3] = (uint8_t)(y & 0xFFu);
    out[4] = (uint8_t)((y >> 8) & 0xFFu);
    out[5] = 0;
    out[6] = 0;
    return USB_POINTER_EVENT_SIZE;
}
