/*
 * hid_report.c: transport-independent HID report-descriptor parsing.
 *
 * A HID report descriptor means the same thing whichever transport
 * carried it, so this is shared by every HID host: usbhostd parses the
 * descriptor fetched with GET_DESCRIPTOR(report) over the control
 * endpoint, btd parses the HOGP Report Map read over GATT. It covers
 * device type detection, composite report-ID discovery, touch/mouse
 * bit-layout extraction and report normalization into the fixed
 * subscriber event formats. No transport code lives here.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <hid/hid_defs.h>
#include <hid/hid_report.h>

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
    uint32_t usages[HID_MAX_USAGE_LIST];
    int usage_count = 0;
    uint32_t usage_min = 0;
    uint32_t usage_max = 0;
    bool usage_range_valid = false;
    bool found_keyboard = false;
    bool found_mouse = false;
    bool found_touch = false;
    bool found_joystick = false;

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
                if (usage_count < HID_MAX_USAGE_LIST) {
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
                    else if (usage == HID_USAGE_JOYSTICK || usage == HID_USAGE_GAMEPAD) {
                        found_joystick = true;
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
    if (found_joystick) {
        return HID_DEV_TYPE_JOYSTICK;
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
    uint32_t usages[HID_MAX_USAGE_LIST];
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
            if (tag == 0 && usage_count < HID_MAX_USAGE_LIST) {
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
    uint32_t usages[HID_MAX_USAGE_LIST];
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
            if (tag == 0 && usage_count < HID_MAX_USAGE_LIST) {
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
    uint32_t usages[HID_MAX_USAGE_LIST];
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
                if (usage_count < HID_MAX_USAGE_LIST) {
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
    if (out->report_bytes == 0 || out->report_bytes > HID_MAX_REPORT) {
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
    uint32_t usages[HID_MAX_USAGE_LIST];
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
                if (usage_count < HID_MAX_USAGE_LIST) {
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
                /* bit 2 = Relative: set on a mouse/touchpad X/Y, clear on an
                   absolute pointing device (touchscreen/tablet). This is the
                   only reliable relative-vs-absolute discriminator -- axis
                   width is not (high-resolution mice use 16-bit deltas). */
                bool relative = (value & 0x4u) != 0;

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
                            if (relative) {
                                out->axis_relative = true;
                            }
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
                            if (relative) {
                                out->axis_relative = true;
                            }
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
    if (out->report_bytes == 0 || out->report_bytes > HID_MAX_REPORT) {
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

/* True when two bit fields [a_bit, a_bit+a_size) and [b_bit, b_bit+b_size)
   share any bit. A field with a negative bit position or non-positive size
   is "absent" and never overlaps anything. */
static bool bit_ranges_overlap(int a_bit, int a_size, int b_bit, int b_size) {
    if (a_bit < 0 || b_bit < 0 || a_size <= 0 || b_size <= 0) {
        return false;
    }
    return a_bit < b_bit + b_size && b_bit < a_bit + a_size;
}

/* A wrong parser result (garbled descriptor read, unusual descriptor) makes
   normalize decode X/Y from the wrong bits -- typically X still looks fine
   while Y reads a padding/wheel field and stays 0 or barely moves.  The same
   instability can leave X/Y correct while a BUTTON points at a padding or
   axis bit: the cursor then tracks perfectly but clicks fire on the wrong
   bit, fire spuriously, or never fire.  Only trust the parser when the whole
   layout (axes AND buttons) is plausible.  strict is used for boot
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
    /*
     * Every declared button must be a real 1..8-bit field inside the report
     * and must not collide with the axes, the wheel, or another button. An
     * overlap means the descriptor was misparsed (buttons landing on X/Y or
     * padding bits) -- exactly the "movement fine, clicks wrong" failure.
     */
    for (int i = 0; i < 3; ++i) {
        int b_bit = p->button_bit[i];
        int b_size = p->button_size[i];
        if (b_bit < 0) {
            continue; /* button not present in this descriptor */
        }
        if (b_size <= 0 || b_size > 8) {
            return false;
        }
        if ((uint32_t)b_bit + (uint32_t)b_size > total) {
            return false;
        }
        if (bit_ranges_overlap(b_bit, b_size, p->x_bit, p->x_size) ||
                bit_ranges_overlap(b_bit, b_size, p->y_bit, p->y_size) ||
                bit_ranges_overlap(b_bit, b_size, p->wheel_bit, p->wheel_size)) {
            return false;
        }
        for (int j = i + 1; j < 3; ++j) {
            if (bit_ranges_overlap(b_bit, b_size, p->button_bit[j], p->button_size[j])) {
                return false;
            }
        }
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
        /*
         * A boot interface must expose at least the left button. If the
         * parser lost it we can still fall back to the boot layout, where
         * byte 0 is guaranteed to hold the buttons, so reject it here rather
         * than run with dead clicks. (For report-protocol-only mice we keep
         * a button-less-but-otherwise-valid parser: rejecting it would drop
         * to a raw fallback that also breaks the working X/Y.)
         */
        if (p->button_bit[0] < 0) {
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

    memset(out, 0, HID_POINTER_EVENT_SIZE);
    out[0] = buttons;
    out[1] = (uint8_t)hid_clamp_s8(x);
    out[2] = (uint8_t)hid_clamp_s8(y);
    out[3] = (uint8_t)hid_clamp_s8(wheel);
    return HID_POINTER_EVENT_SIZE;
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
    return HID_POINTER_EVENT_SIZE;
}

bool mouse_parser_is_absolute(const mouse_parser_t* m) {
    if (!m->valid) {
        return false;
    }
    /*
     * Trust the descriptor's Relative bit first: a device that declares its
     * X/Y as Relative is a genuine relative mouse/touchpad no matter how wide
     * the axes are. High-resolution mice report 16-bit signed deltas, which
     * the old width-only heuristic mistook for absolute coordinates -- it then
     * fed those deltas to the touch path as positions near (0,0), dragging the
     * cursor to the top-left corner. Only when the axes are NOT relative fall
     * back to the width heuristic (both axes > 8 bits) to catch absolute
     * pointing devices whose descriptor omits a usable Rel/Abs distinction.
     */
    if (m->axis_relative) {
        return false;
    }
    return m->x_size > 8 && m->y_size > 8;
}

int mouse_normalize_as_touch(const mouse_parser_t* m,
        const uint8_t* report, int len, uint8_t* out) {
    uint32_t x;
    uint32_t y;
    uint8_t pressed;

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

    /* button 1 (left) acts as tip-switch for absolute pointing devices */
    pressed = 0;
    if (m->button_bit[0] >= 0 &&
            bit_extract_le(report, m->button_bit[0], m->button_size[0]) != 0) {
        pressed = 1;
    }

    x = bit_extract_le(report, m->x_bit, m->x_size);
    y = bit_extract_le(report, m->y_bit, m->y_size);
    if (x > 0xFFFFu) {
        x = 0xFFFFu;
    }
    if (y > 0xFFFFu) {
        y = 0xFFFFu;
    }

    out[0] = pressed;
    out[1] = (uint8_t)(x & 0xFFu);
    out[2] = (uint8_t)((x >> 8) & 0xFFu);
    out[3] = (uint8_t)(y & 0xFFu);
    out[4] = (uint8_t)((y >> 8) & 0xFFu);
    out[5] = 0;
    out[6] = 0;
    return HID_POINTER_EVENT_SIZE;
}

/* ---------------- generic joystick/gamepad ---------------- */

/* Extract the bit layout of a Joystick/Gamepad application collection. This
   is the descriptor-driven counterpart of hid_parse_mouse_report: instead of
   a per-device byte table it records where each field lives (buttons run,
   hat switch, the six Generic-Desktop axes and up to two sliders) together
   with the logical range declared for it, so joystick_normalize_report can
   scale any conforming gamepad. It covers the uConsole's USBComposite
   joystick (32 buttons + 4-bit hat + X/Y/Rx/Ry + 2 sliders, all 10-bit) and
   ordinary standard-HID USB gamepads alike. */
int hid_parse_joystick_report(const uint8_t* desc, int len, joystick_parser_t* out) {
    uint32_t usages[HID_MAX_USAGE_LIST];
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
    int js_collection_depth = -1;
    bool js_active = false;
    int slider_idx = 0;
    int32_t logical_min = 0;
    int32_t logical_max = 0;

    memset(report_bits, 0, sizeof(report_bits));
    memset(out, 0, sizeof(*out));
    out->button_bit = -1;
    out->hat_bit = -1;

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
            switch (tag) {
            case 0:
                usage_page = value;
                break;
            case 1:
                logical_min = hid_sign_extend(value, size * 8);
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
        else if (type == 2) { /* local */
            switch (tag) {
            case 0:
                if (usage_count < HID_MAX_USAGE_LIST) {
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
        else if (type == 0) { /* main */
            switch (tag) {
            case 8: { /* Input */
                bool constant = (value & 0x1u) != 0;
                bool variable = (value & 0x2u) != 0;

                if (js_active && !constant && variable && report_size > 0) {
                    for (uint32_t idx = 0; idx < report_count; ++idx) {
                        uint32_t usage = hid_usage_for_index(usages, usage_count,
                                usage_range_valid, usage_min, usage_max, (int)idx);
                        int bit = (int)report_bits[current_report_id] +
                                (int)(idx * report_size);

                        if (usage_page == HID_USAGE_PAGE_BUTTON) {
                            /* a contiguous run of 1-bit-per-usage buttons; keep
                               the first and derive the rest by index */
                            if (out->button_bit < 0) {
                                out->button_bit = bit;
                                out->button_size = (int)report_size;
                                out->button_count = (int)report_count;
                                out->button_usage_min = usage;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                            break; /* whole run captured at once */
                        }
                        if (usage_page == HID_USAGE_PAGE_SIMULATION &&
                                (usage == HID_USAGE_BRAKE ||
                                 usage == HID_USAGE_ACCELERATOR)) {
                            /* Xbox pads expose the analog triggers as
                               Simulation-page Brake (LT) and Accelerator (RT),
                               each a separate 10-bit Input item, rather than
                               as Generic-Desktop sliders. Capture them into
                               the trigger slots or the pad decodes with
                               LT/RT permanently 0. */
                            int si = (usage == HID_USAGE_BRAKE) ? 0 : 1;
                            if (!out->slider[si].present) {
                                out->slider[si].present = true;
                                out->slider[si].bit = bit;
                                out->slider[si].size = (int)report_size;
                                out->slider[si].logical_min = logical_min;
                                out->slider[si].logical_max = logical_max;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                            continue;
                        }
                        if (usage_page != HID_USAGE_PAGE_GENERIC_DESKTOP) {
                            continue;
                        }
                        if (usage == HID_USAGE_HAT_SWITCH) {
                            if (out->hat_bit < 0) {
                                out->hat_bit = bit;
                                out->hat_size = (int)report_size;
                                out->hat_min = logical_min;
                                out->hat_max = logical_max;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                        else if (usage >= HID_USAGE_X && usage <= HID_USAGE_RZ) {
                            int ax = (int)(usage - HID_USAGE_X); /* 0..5 */
                            if (ax >= 0 && ax < JS_AXIS_COUNT && !out->axis[ax].present) {
                                out->axis[ax].present = true;
                                out->axis[ax].bit = bit;
                                out->axis[ax].size = (int)report_size;
                                out->axis[ax].logical_min = logical_min;
                                out->axis[ax].logical_max = logical_max;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                            }
                        }
                        else if (usage == HID_USAGE_SLIDER) {
                            if (slider_idx < 2) {
                                out->slider[slider_idx].present = true;
                                out->slider[slider_idx].bit = bit;
                                out->slider[slider_idx].size = (int)report_size;
                                out->slider[slider_idx].logical_min = logical_min;
                                out->slider[slider_idx].logical_max = logical_max;
                                out->has_report_id = current_report_id != 0;
                                out->report_id = current_report_id;
                                slider_idx++;
                            }
                        }
                    }
                }
                report_bits[current_report_id] += report_size * report_count;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 10: { /* Collection */
                uint32_t usage = hid_usage_for_index(usages, usage_count,
                        usage_range_valid, usage_min, usage_max, 0);
                uint8_t collection_type = (uint8_t)value;

                if (!js_active && collection_type == 1 &&
                        usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP &&
                        (usage == HID_USAGE_JOYSTICK || usage == HID_USAGE_GAMEPAD)) {
                    js_collection_depth = collection_depth + 1;
                    js_active = true;
                }
                collection_depth++;
                clear_local_usages(usages, &usage_count, &usage_range_valid);
                break;
            }
            case 12: /* End Collection */
                if (collection_depth == js_collection_depth) {
                    js_active = false;
                    js_collection_depth = -1;
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

    /* a usable gamepad needs at least one button or one analog axis */
    bool any_axis = false;
    for (int i = 0; i < JS_AXIS_COUNT; ++i) {
        if (out->axis[i].present) {
            any_axis = true;
            break;
        }
    }
    if (out->button_bit < 0 && !any_axis) {
        return -1;
    }

    out->valid = true;
    out->report_bytes = (uint8_t)((report_bits[out->report_id] + 7u) / 8u);
    if (out->report_bytes == 0 || out->report_bytes > HID_MAX_REPORT) {
        return -1;
    }
    return 0;
}

bool hid_probe_joystick_report(const uint8_t* desc, int len, joystick_parser_t* out) {
    joystick_parser_t parser;

    if (hid_parse_joystick_report(desc, len, &parser) != 0) {
        return false;
    }
    if (out != NULL) {
        *out = parser;
    }
    return true;
}

/* map an HID Button usage (1-based) onto the physical js_evt_t button bits.
   HID button usages carry no inherent A/B/X/Y meaning, so this fixes one
   project-wide convention for the face-button diamond (1=X 2=A 3=B 4=Y),
   then 5=LB 6=RB 7=LT 8=RT 9=Back/Select 10=Start 11=L3 12=R3 13=Home. */
static uint32_t js_button_from_usage(uint32_t usage) {
    switch (usage) {
    case 1:  return JS_BTN_X;
    case 2:  return JS_BTN_A;
    case 3:  return JS_BTN_B;
    case 4:  return JS_BTN_Y;
    case 5:  return JS_BTN_LB;
    case 6:  return JS_BTN_RB;
    case 7:  return JS_BTN_LT;
    case 8:  return JS_BTN_RT;
    case 9:  return JS_BTN_SELECT;
    case 10: return JS_BTN_START;
    case 11: return JS_BTN_LS;
    case 12: return JS_BTN_RS;
    case 13: return JS_BTN_HOME;
    default: return 0;
    }
}

/* read one analog field, sign-extending only when its logical range is
   signed (an unsigned 0..1023 axis must NOT be treated as a 10-bit int) */
static int32_t js_axis_raw(const js_axis_t* a, const uint8_t* report) {
    uint32_t v = bit_extract_le(report, a->bit, a->size);
    if (a->logical_min < 0) {
        return hid_sign_extend(v, a->size);
    }
    return (int32_t)v;
}

/* scale a raw axis value from its logical range onto out_lo..out_hi */
static int32_t js_axis_scale(int32_t raw, const js_axis_t* a,
        int32_t out_lo, int32_t out_hi) {
    int32_t lo = a->logical_min;
    int32_t hi = a->logical_max;
    int64_t num;
    int64_t den;

    if (hi <= lo) {
        return out_lo;
    }
    if (raw < lo) {
        raw = lo;
    }
    if (raw > hi) {
        raw = hi;
    }
    num = (int64_t)(raw - lo) * (int64_t)(out_hi - out_lo);
    den = (int64_t)(hi - lo);
    return (int32_t)(out_lo + num / den);
}

static int16_t js_axis_to_stick(const js_axis_t* a, const uint8_t* report) {
    int32_t v = js_axis_scale(js_axis_raw(a, report), a, -32768, 32767);
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    return (int16_t)v;
}

static uint16_t js_axis_to_trigger(const js_axis_t* a, const uint8_t* report) {
    int32_t v = js_axis_scale(js_axis_raw(a, report), a, 0, 65535);
    if (v > 65535) v = 65535;
    if (v < 0) v = 0;
    return (uint16_t)v;
}

int joystick_normalize_report(const joystick_parser_t* j,
        const uint8_t* report, int len, js_evt_t* out) {
    const js_axis_t* rx;
    const js_axis_t* ry;

    if (!j->valid) {
        return -1;
    }
    if (j->has_report_id) {
        if (len <= 0 || report[0] != j->report_id) {
            return -1;
        }
    }
    if (len < j->report_bytes) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    out->dpad = JS_DPAD_NONE;

    /* buttons: a contiguous run of button_count fields */
    if (j->button_bit >= 0) {
        uint32_t b = 0;
        for (int i = 0; i < j->button_count && i < JS_MAX_BUTTONS; ++i) {
            int bit = j->button_bit + i * j->button_size;
            if (bit_extract_le(report, bit, j->button_size) != 0) {
                b |= js_button_from_usage(j->button_usage_min + (uint32_t)i);
            }
        }
        out->buttons = b;
    }

    /* hat switch: 0..7 compass relative to logical_min, else released */
    if (j->hat_bit >= 0) {
        int32_t v = (int32_t)bit_extract_le(report, j->hat_bit, j->hat_size);
        v -= j->hat_min;
        out->dpad = (v >= 0 && v <= 7) ? (uint8_t)v : JS_DPAD_NONE;
    }

    /* left stick = X/Y */
    if (j->axis[0].present) {
        out->lx = js_axis_to_stick(&j->axis[0], report);
    }
    if (j->axis[1].present) {
        out->ly = js_axis_to_stick(&j->axis[1], report);
    }
    /* right stick = Rx/Ry when present, else fall back to Z/Rz */
    rx = j->axis[3].present ? &j->axis[3] : (j->axis[2].present ? &j->axis[2] : NULL);
    ry = j->axis[4].present ? &j->axis[4] : (j->axis[5].present ? &j->axis[5] : NULL);
    if (rx != NULL) {
        out->rx = js_axis_to_stick(rx, report);
    }
    if (ry != NULL) {
        out->ry = js_axis_to_stick(ry, report);
    }
    /* analog triggers from the sliders when the descriptor declares them */
    if (j->slider[0].present) {
        out->lt = js_axis_to_trigger(&j->slider[0], report);
    }
    if (j->slider[1].present) {
        out->rt = js_axis_to_trigger(&j->slider[1], report);
    }

    out->connected = 1;
    return 0;
}
