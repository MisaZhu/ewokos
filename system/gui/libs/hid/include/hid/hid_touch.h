/*
 * hid_touch.h: multi-contact USB touchscreen model.
 *
 * A HID Digitizer touch screen (usage 0x04) reports up to N simultaneous
 * contacts in one report - each with a stable Contact ID, Tip Switch and
 * X/Y - plus a report-level Contact Count. The legacy single-contact touch
 * event (7 bytes, one tip/x/y) cannot represent that, so report id
 * HID_REPORT_ID_TOUCH now carries this fixed-size frame instead: one frame
 * per report, every slot's tip state included, so the consumer can diff
 * presses and releases without remembering the device's slot layout.
 *
 * Single-contact panels (and absolute mice presented as touch) are wrapped
 * into the same frame with one slot, so hid_touchd has a single input
 * format and always emits multi-touch edge frames on /dev/touch0.
 *
 * The frame is fixed-size (HID_TOUCH_EVENT_SIZE) so the subscriber queue
 * (hid_srv) and the consumer daemon (hid_touchd) can batch-drain with a
 * constant stride, exactly like the touchpad frames.
 */
#ifndef __HID_TOUCH_H__
#define __HID_TOUCH_H__

#include <stdint.h>
#include <stdbool.h>
#include <hid/hid_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The /dev/touch0 multi-touch protocol carries contact ids 0..9. */
#define HID_TOUCH_MAX_CONTACTS 10

/* One contact slot inside a touch frame. x/y are the device's logical
   coordinates clamped to 16 bits, exactly what the legacy touch event
   carried; the consumer maps them to the screen through its own config. */
typedef struct __attribute__((packed)) {
    uint8_t id;   /* device contact id (stable while the finger is down) */
    uint8_t tip;  /* 1 = finger touching the panel */
    uint16_t x;
    uint16_t y;
} touch_contact_t;

/* One normalized touch report. contact_count is the number of valid
   entries in contacts[] (the report's Contact Count, or the slot count
   when the descriptor has none); unused slots are zeroed. A slot with
   tip 0 inside the valid range is a contact the device still lists but
   that is not touching - the consumer treats it as released. */
typedef struct __attribute__((packed)) {
    uint8_t contact_count;
    uint8_t reserved;
    uint16_t x_max;   /* device logical X range (0 when unknown) */
    uint16_t y_max;
    touch_contact_t contacts[HID_TOUCH_MAX_CONTACTS];
} touch_mt_evt_t;

#define HID_TOUCH_EVENT_SIZE (6 + HID_TOUCH_MAX_CONTACTS * 6) /* 66 */

/* Descriptor-driven bit layout of a multi-contact touch screen report.
   Two descriptor styles are covered:
    - repeated Finger collections: contact i's fields live slot_bits bits
      after contact 0's (the PTP layout);
    - one Finger collection with Report Count N per field: each usage is a
      run of N fields, so a field's stride is its own report_size.
   Per-field strides carry whichever style was parsed. */
typedef struct {
    bool valid;
    bool has_report_id;
    uint8_t report_id;
    uint8_t report_bytes;
    int max_contacts;              /* slots in the report (1..MAX) */
    uint32_t x_max;                /* X logical max (0 when unknown) */
    uint32_t y_max;
    int slot_bits;                 /* repeated-collection stride, else 0 */
    /* contact 0 field bit offsets/sizes/strides, absolute in the report */
    int tip_bit;   int tip_size;   int tip_stride;
    int id_bit;    int id_size;    int id_stride;   /* id_bit -1: absent */
    int x_bit;     int x_size;     int x_stride;
    int y_bit;     int y_size;     int y_stride;
    /* report-level Contact Count (may be absent) */
    int count_bit; int count_size;
} touch_mt_parser_t;

#ifdef __cplusplus
}
#endif

#endif /* __HID_TOUCH_H__ */
