/* Host test for the multi-contact touch screen descriptor parser and
 * normalizer: repeated Finger collections, Report Count runs, legacy
 * single-contact panels, Contact Count handling and malformed input.
 * No hardware. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <hid/hid_report.h>
#include <hid/hid_touch.h>

/* Two repeated Finger collections (tip 1 bit + id 7 bits + X/Y 16 bits
   each), report id 3, a Contact Count Maximum and a Contact Count. */
static const uint8_t desc_repeated[] = {
    0x05, 0x0D, 0x09, 0x04, 0xA1, 0x01,
    0x85, 0x03,
    /* finger 0 */
    0x09, 0x22, 0xA1, 0x02,
    0x09, 0x42, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x51, 0x25, 0x0A, 0x75, 0x07, 0x95, 0x01, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x30, 0x26, 0xFF, 0x0F, 0x75, 0x10, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x31, 0x81, 0x02,
    0xC0,
    /* finger 1 */
    0x05, 0x0D, 0x09, 0x22, 0xA1, 0x02,
    0x09, 0x42, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x51, 0x25, 0x0A, 0x75, 0x07, 0x95, 0x01, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x30, 0x26, 0xFF, 0x0F, 0x75, 0x10, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x31, 0x81, 0x02,
    0xC0,
    /* contact count maximum (input here so the parser sees it) + count */
    0x05, 0x0D, 0x09, 0x55, 0x25, 0x02, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x54, 0x81, 0x02,
    0xC0
};

/* One Finger collection with Report Count 5 per field: a run of 5 tip
   bits, 3 constant pad bits, then 5 ids / 5 X / 5 Y, report id 3. */
static const uint8_t desc_runs[] = {
    0x05, 0x0D, 0x09, 0x04, 0xA1, 0x01,
    0x85, 0x03,
    0x09, 0x22, 0xA1, 0x02,
    0x09, 0x42, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x05, 0x81, 0x02,
    0x75, 0x03, 0x95, 0x01, 0x81, 0x03,
    0x09, 0x51, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x05, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x30, 0x26, 0xFF, 0x0F, 0x75, 0x10, 0x95, 0x05, 0x81, 0x02,
    0x09, 0x31, 0x81, 0x02,
    0xC0,
    0x05, 0x0D, 0x09, 0x54, 0x25, 0x05, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02,
    0xC0
};

/* Legacy single-contact panel: no Finger collection, no report id. */
static const uint8_t desc_legacy[] = {
    0x05, 0x0D, 0x09, 0x04, 0xA1, 0x01,
    0x09, 0x42, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
    0x75, 0x07, 0x95, 0x01, 0x81, 0x03,
    0x05, 0x01, 0x09, 0x30, 0x26, 0xFF, 0x0F, 0x75, 0x10, 0x95, 0x01, 0x81, 0x02,
    0x09, 0x31, 0x81, 0x02,
    0xC0
};

static void check_contact(const touch_mt_evt_t* f, unsigned i,
        uint8_t id, uint8_t tip, uint16_t x, uint16_t y) {
    assert(f->contacts[i].id == id);
    assert(f->contacts[i].tip == tip);
    assert(f->contacts[i].x == x);
    assert(f->contacts[i].y == y);
}

int main(void) {
    touch_mt_parser_t p;
    touch_mt_evt_t f;
    uint8_t report[64];

    /* repeated collections: layout, stride and slot count */
    assert(hid_parse_touch_mt_report(desc_repeated, sizeof(desc_repeated), &p) == 0);
    assert(p.valid && p.has_report_id && p.report_id == 3);
    assert(p.report_bytes == 13 && p.max_contacts == 2);
    assert(p.tip_bit == 8 && p.tip_size == 1 && p.tip_stride == 40);
    assert(p.id_bit == 9 && p.id_size == 7 && p.id_stride == 40);
    assert(p.x_bit == 16 && p.x_size == 16 && p.x_stride == 40);
    assert(p.y_bit == 32 && p.y_size == 16 && p.y_stride == 40);
    assert(p.count_bit == 96 && p.count_size == 8);
    assert(p.x_max == 4095 && p.y_max == 4095);

    /* two contacts down: id 5 at (100,200), id 9 at (300,400) */
    memset(report, 0, sizeof(report));
    report[0] = 3;
    report[1] = 1 | (5 << 1);
    report[2] = 100; report[4] = 200;
    report[6] = 1 | (9 << 1);
    report[7] = 300 & 0xFF; report[8] = 300 >> 8;
    report[9] = 400 & 0xFF; report[10] = 400 >> 8;
    report[11] = 2;                 /* contact count maximum (ignored) */
    report[12] = 2;                 /* contact count */
    assert(touch_mt_normalize_report(&p, report, 13, &f) == 0);
    assert(f.contact_count == 2 && f.x_max == 4095 && f.y_max == 4095);
    check_contact(&f, 0, 5, 1, 100, 200);
    check_contact(&f, 1, 9, 1, 300, 400);

    /* the count gates the slots: 1 lists only the first, 0 lists none,
       an over-large count clamps to the slot count */
    report[12] = 1;
    assert(touch_mt_normalize_report(&p, report, 13, &f) == 0);
    assert(f.contact_count == 1);
    check_contact(&f, 0, 5, 1, 100, 200);
    check_contact(&f, 1, 0, 0, 0, 0);
    report[12] = 0;
    assert(touch_mt_normalize_report(&p, report, 13, &f) == 0);
    assert(f.contact_count == 0);
    report[12] = 7;
    assert(touch_mt_normalize_report(&p, report, 13, &f) == 0);
    assert(f.contact_count == 2);
    /* a listed contact with tip 0 stays listed, released (the id bits
       share the tip's byte, so keep them) */
    report[12] = 2;
    report[6] = 9 << 1;
    assert(touch_mt_normalize_report(&p, report, 13, &f) == 0);
    check_contact(&f, 1, 9, 0, 300, 400);

    /* wrong report id and short reports are rejected */
    report[0] = 4;
    assert(touch_mt_normalize_report(&p, report, 13, &f) == -1);
    report[0] = 3;
    assert(touch_mt_normalize_report(&p, report, 12, &f) == -1);
    /* extra trailing bytes are harmless */
    assert(touch_mt_normalize_report(&p, report, 20, &f) == 0);

    /* Report Count runs: each field's stride is its own width */
    assert(hid_parse_touch_mt_report(desc_runs, sizeof(desc_runs), &p) == 0);
    assert(p.report_bytes == 28 && p.max_contacts == 5 && p.slot_bits == 0);
    assert(p.tip_bit == 8 && p.tip_size == 1 && p.tip_stride == 1);
    assert(p.id_bit == 16 && p.id_size == 8 && p.id_stride == 8);
    assert(p.x_bit == 56 && p.x_size == 16 && p.x_stride == 16);
    assert(p.y_bit == 136 && p.y_size == 16 && p.y_stride == 16);
    assert(p.count_bit == 216 && p.count_size == 8);

    /* contacts 0 and 2 touching, contact 1 listed with tip 0 */
    memset(report, 0, sizeof(report));
    report[0] = 3;
    report[1] = 0x05;               /* tip bits: contacts 0 and 2 */
    report[2] = 7;                  /* ids: 7, 0, 4, 0, 0 */
    report[4] = 4;
    report[7] = 111;                /* X run: contact 0 */
    report[11] = 333 & 0xFF;        /* X run: contact 2 */
    report[12] = 333 >> 8;
    report[17] = 222;               /* Y run: contact 0 */
    report[21] = 444 & 0xFF;        /* Y run: contact 2 */
    report[22] = 444 >> 8;
    report[27] = 3;                 /* contact count */
    assert(touch_mt_normalize_report(&p, report, 28, &f) == 0);
    assert(f.contact_count == 3);
    check_contact(&f, 0, 7, 1, 111, 222);
    check_contact(&f, 1, 0, 0, 0, 0);
    check_contact(&f, 2, 4, 1, 333, 444);
    check_contact(&f, 3, 0, 0, 0, 0);

    /* legacy panel: one slot, the slot index becomes the id */
    assert(hid_parse_touch_mt_report(desc_legacy, sizeof(desc_legacy), &p) == 0);
    assert(!p.has_report_id && p.report_bytes == 5 && p.max_contacts == 1);
    assert(p.tip_bit == 0 && p.x_bit == 8 && p.y_bit == 24);
    assert(p.id_bit == -1 && p.count_bit == -1);
    report[0] = 1; report[1] = 100; report[2] = 0; report[3] = 200; report[4] = 0;
    assert(touch_mt_normalize_report(&p, report, 5, &f) == 0);
    assert(f.contact_count == 1);
    check_contact(&f, 0, 0, 1, 100, 200);
    report[0] = 0;
    assert(touch_mt_normalize_report(&p, report, 5, &f) == 0);
    check_contact(&f, 0, 0, 0, 100, 200);

    /* a touch PAD collection is not a touch screen: the probe must not
       claim the pad's descriptor (hid_touchpad owns that one) */
    {
        uint8_t pad[sizeof(desc_legacy)];
        memcpy(pad, desc_legacy, sizeof(pad));
        pad[3] = 0x05;              /* Usage (Touch Pad) */
        assert(!hid_probe_touch_mt_report(pad, sizeof(pad), &p));
    }
    /* a descriptor without Y is not a touch screen either */
    {
        uint8_t noy[sizeof(desc_legacy)];
        memcpy(noy, desc_legacy, sizeof(noy));
        /* drop the Y usage item prefix (09 31): the Input that follows
           has no usage left, so Y is never captured */
        memmove(noy + 37, noy + 39, sizeof(noy) - 39);
        assert(!hid_probe_touch_mt_report(noy, sizeof(noy) - 2, &p));
    }
    /* truncated descriptors must not parse */
    assert(!hid_probe_touch_mt_report(desc_repeated, 10, &p));
    assert(!hid_probe_touch_mt_report(desc_repeated, 0, &p));

    puts("hid_touch_parse_test: all checks passed");
    return 0;
}