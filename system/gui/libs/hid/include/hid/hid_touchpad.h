/*
 * hid_touchpad.h: Windows Precision Touchpad (PTP) multi-contact model.
 *
 * A PTP device is a HID Digitizer touch pad (usage 0x05) whose reports
 * carry, in one digitizer report, up to N simultaneous contacts - each with
 * a stable Contact ID, Tip Switch, confidence, X/Y and width/height - plus
 * the report-level Contact Count, In Range and the physical buttons.
 * Nothing in the legacy single-contact touch model (touch_parser_t +
 * touch_normalize_report, /dev/touch0) can represent that, so the transport
 * stack uses this separate parser + frame on its own report id.
 *
 * The frame is fixed-size (HID_TOUCHPAD_EVENT_SIZE) so the subscriber queue
 * (hid_srv) and the consumer daemon (hid_touchpadd) can batch-drain with a
 * constant stride, exactly like the mouse/touch/keyboard event sizes.
 */
#ifndef __HID_TOUCHPAD_H__
#define __HID_TOUCHPAD_H__

#include <stdint.h>
#include <stdbool.h>
#include <hid/hid_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One contact slot inside a touchpad frame. Fields are filled by the
   descriptor-driven normalizer; x/y are in the device's logical units and
   width/height in the device's width-unit (PTP pads declare a Width usage
   of its own logical range). */
typedef struct __attribute__((packed)) {
    uint8_t id;         /* device contact id (stable while the finger is down) */
    uint8_t tip;        /* 1 = finger touching the pad */
    uint8_t confidence; /* 1 = device is confident this is a real finger */
    uint8_t reserved;
    uint16_t x;
    uint16_t y;
    uint8_t width;
    uint8_t height;
} touchpad_contact_t;

/* One normalized touchpad report. contact_count is the number of valid
   entries in contacts[]; unused slots are zeroed. buttons reports the
   physical buttons latched in the same digitizer report (bit0 left,
   bit1 right, bit2 middle). x_max/y_max are the device's logical ranges
   so the gesture client can scale deltas without its own config. */
typedef struct __attribute__((packed)) {
    uint8_t buttons;
    uint8_t contact_count;
    uint8_t in_range;
    uint8_t reserved;
    uint16_t x_max;   /* device logical X range (0 when unknown) */
    uint16_t y_max;
    touchpad_contact_t contacts[HID_TOUCHPAD_MAX_CONTACTS];
} touchpad_evt_t;

/* Descriptor-driven bit layout of the touchpad digitizer report. The
   per-contact fields are captured from the first contact slot; contact i's
   fields live slot_bits bits after contact 0's, because PTP devices group
   every contact's fields into its own contiguous block (Finger logical
   collection, or a plain block on legacy pads). */
typedef struct {
    bool valid;
    bool has_report_id;
    uint8_t report_id;
    uint8_t report_bytes;
    int max_contacts;             /* contact_count logical max (1..MAX) */
    uint32_t x_max;               /* X logical max (for pointer scaling) */
    uint32_t y_max;
    int slot_bits;                /* bits from one contact slot to the next */
    /* contact 0 field bit offsets/sizes, absolute within the report */
    int contact_id_bit;   int contact_id_size;
    int tip_bit;          int tip_size;
    int confidence_bit;   int confidence_size;
    int x_bit;            int x_size;
    int y_bit;            int y_size;
    int width_bit;        int width_size;
    int height_bit;       int height_size;
    /* report-level fields (may be absent on legacy pads) */
    int button_bit[3];    int button_size[3];
    int contact_count_bit; int contact_count_size;
    int in_range_bit;      int in_range_size;
} touchpad_parser_t;

#ifdef __cplusplus
}
#endif

#endif /* __HID_TOUCHPAD_H__ */
