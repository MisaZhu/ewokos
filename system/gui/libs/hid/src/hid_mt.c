#include <hid/hid_mt.h>

#include <string.h>

void hid_mt_init(hid_mt_t* mt) {
    memset(mt, 0, sizeof(*mt));
}

static void apply(hid_mt_slot_t* set, const hid_mt_entry_t* ent) {
    unsigned id = ent->rec.state & 0xffU;
    unsigned act = (ent->rec.state >> 8) & 0x3U;

    if (id >= HID_MT_MAX_CONTACTS)
        return;
    set[id].active = act != HID_MT_ACT_RELEASE;
    set[id].gen = ent->gen;
    set[id].x = ent->rec.x;
    set[id].y = ent->rec.y;
}

/* Write the frame that takes `set` to `to` and advance `set` past it.
 * Returns the records written, header included; 0 when nothing changed.
 * Where both hold a contact it must be the same one (see build()). */
static unsigned build_frame(hid_mt_slot_t* set, const hid_mt_slot_t* to,
        hid_mt_entry_t* out) {
    unsigned n = 0;

    for (unsigned id = 0; id < HID_MT_MAX_CONTACTS; id++) {
        hid_mt_entry_t* ent = &out[1U + n];
        unsigned act;

        if (to[id].active) {
            if (!set[id].active)
                act = HID_MT_ACT_PRESS;
            else if (set[id].x != to[id].x || set[id].y != to[id].y)
                act = HID_MT_ACT_MOVE;
            else
                continue;
            ent->rec.x = to[id].x;
            ent->rec.y = to[id].y;
            ent->gen = to[id].gen;
        } else if (set[id].active) {
            act = HID_MT_ACT_RELEASE;
            ent->rec.x = set[id].x;
            ent->rec.y = set[id].y;
            ent->gen = set[id].gen;
        } else {
            continue;
        }
        ent->rec.state = (uint16_t)(HID_MT_FRAME | (act << 8) | id);
        n++;
    }
    if (n == 0)
        return 0;
    out[0].rec.state = HID_MT_FRAME;
    out[0].rec.x = (uint16_t)n;
    out[0].rec.y = 0;
    out[0].gen = 0;
    for (unsigned i = 0; i < n; i++)
        apply(set, &out[1U + i]);
    return 1U + n;
}

static unsigned build(hid_mt_slot_t* set, const hid_mt_slot_t* to,
        hid_mt_entry_t* out) {
    hid_mt_slot_t lifted[HID_MT_MAX_CONTACTS];
    bool reused = false;
    unsigned n = 0;

    /*
     * A slot can hold another finger than the one the reader has in it: the
     * source hands a slot over in the very frame the old contact lifts, and
     * after an overflow the lift in between may be among the dropped frames.
     * Diffed as it stands that is one contact jumping across the panel - a
     * drag or a pinch that leaps.  Lift the old contact in a frame of its
     * own first.
     */
    memcpy(lifted, to, sizeof(lifted));
    for (unsigned id = 0; id < HID_MT_MAX_CONTACTS; id++) {
        if (to[id].active && set[id].active && set[id].gen != to[id].gen) {
            lifted[id].active = false;
            reused = true;
        }
    }
    if (reused)
        n = build_frame(set, lifted, out);
    return n + build_frame(set, to, out + n);
}

unsigned hid_mt_update(hid_mt_t* mt, const hid_mt_contact_t* cur,
        hid_mt_record_t* queued) {
    hid_mt_slot_t to[HID_MT_MAX_CONTACTS];
    hid_mt_slot_t set[HID_MT_MAX_CONTACTS];
    hid_mt_entry_t recs[HID_MT_UPDATE_MAX];
    unsigned n;

    /* Name the contacts against the previous update, which `emitted` always
     * is, whatever happened to the frames that were queued for it. */
    memset(to, 0, sizeof(to));
    for (unsigned id = 0; id < HID_MT_MAX_CONTACTS; id++) {
        if (cur == NULL || !cur[id].active)
            continue;
        to[id].active = true;
        to[id].x = cur[id].x;
        to[id].y = cur[id].y;
        if (mt->emitted[id].active && !cur[id].fresh)
            to[id].gen = mt->emitted[id].gen;
        else
            to[id].gen = ++mt->last_gen;
    }

    memcpy(set, mt->emitted, sizeof(set));
    n = build(set, to, recs);
    if (mt->count + n > HID_MT_QUEUE_SIZE) {
        /*
         * The reader fell behind.  A touch stream is only useful at its
         * leading edge, so forget the frames it has not started and take it
         * straight from what it has seen to where the contacts are now.
         * Individual records cannot be dropped the way single positions
         * could: a lost press or release leaves the contact wrong downstream
         * for good.  The frame the reader is in the middle of is kept, so
         * its framing stays whole.
         */
        memcpy(set, mt->delivered, sizeof(set));
        for (unsigned i = 0; i < mt->head_left; i++)
            apply(set, &mt->queue[i]);
        mt->count = mt->head_left;
        mt->drops++;
        n = build(set, to, recs);
    }
    memcpy(&mt->queue[mt->count], recs, n * sizeof(recs[0]));
    mt->count += n;
    memcpy(mt->emitted, set, sizeof(set));
    for (unsigned i = 0; i < n; i++) {
        unsigned act = (recs[i].rec.state >> 8) & 0x3U;

        if (act == HID_MT_ACT_PRESS)
            mt->presses++;
        else if (act == HID_MT_ACT_RELEASE)
            mt->releases++;
        if (queued != NULL)
            queued[i] = recs[i].rec;
    }
    return n;
}

bool hid_mt_pop(hid_mt_t* mt, hid_mt_record_t* out) {
    hid_mt_entry_t ent;

    if (mt->count == 0)
        return false;
    ent = mt->queue[0];
    memmove(&mt->queue[0], &mt->queue[1],
            (mt->count - 1U) * sizeof(mt->queue[0]));
    mt->count--;
    if (ent.rec.state == HID_MT_FRAME) {
        mt->head_left = ent.rec.x;
    } else if (mt->head_left != 0) {
        apply(mt->delivered, &ent);
        mt->head_left--;
    }
    *out = ent.rec;
    return true;
}