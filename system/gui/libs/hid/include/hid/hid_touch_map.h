#ifndef __HID_TOUCH_MAP_H__
#define __HID_TOUCH_MAP_H__
#include <string.h>

#include <hid/hid_touch.h>
#include <hid/hid_mt.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Device contact ids to wire slots (no hardware, host-testable).
 *
 * A touch screen report names its contacts with device-defined Contact IDs
 * that can be any value and can come back in any report slot; the /dev/touch0
 * stream names them 0..9.  A device id keeps its wire slot for as long as it
 * is down; a new id takes a free slot and is marked fresh, so a slot handed
 * to another finger lifts the old contact first.  Ids the report no longer
 * lists with the tip down release their slot.
 */
typedef struct {
    bool used[HID_TOUCH_MAX_CONTACTS];
    uint8_t id[HID_TOUCH_MAX_CONTACTS];
} hid_touch_map_t;

static inline void hid_touch_map_init(hid_touch_map_t* m) {
    memset(m, 0, sizeof(*m));
}

/* Map one normalized report frame onto the wire slot set `cur`
 * (HID_MT_MAX_CONTACTS entries, zeroed first). */
static inline void hid_touch_map_frame(hid_touch_map_t* m,
        const touch_mt_evt_t* frame, hid_mt_contact_t* cur) {
    bool matched[HID_TOUCH_MAX_CONTACTS];
    int slot_of[HID_TOUCH_MAX_CONTACTS];
    unsigned count = frame->contact_count;

    memset(cur, 0, sizeof(cur[0]) * HID_MT_MAX_CONTACTS);
    memset(matched, 0, sizeof(matched));
    if (count > HID_TOUCH_MAX_CONTACTS)
        count = HID_TOUCH_MAX_CONTACTS;

    /* continuing contacts keep their slot */
    for (unsigned i = 0; i < count; i++) {
        const touch_contact_t* c = &frame->contacts[i];

        slot_of[i] = -1;
        if (!c->tip)
            continue;
        for (unsigned s = 0; s < HID_TOUCH_MAX_CONTACTS; s++) {
            if (m->used[s] && !matched[s] && m->id[s] == c->id) {
                slot_of[i] = (int)s;
                matched[s] = true;
                break;
            }
        }
    }
    /* slots whose contact is gone lift */
    for (unsigned s = 0; s < HID_TOUCH_MAX_CONTACTS; s++) {
        if (m->used[s] && !matched[s])
            m->used[s] = false;
    }
    /* place the contacts; new ones take a free slot, fresh */
    for (unsigned i = 0; i < count; i++) {
        const touch_contact_t* c = &frame->contacts[i];
        int s = slot_of[i];
        bool fresh = false;

        if (!c->tip)
            continue;
        /* a duplicated id inside one report is one contact, not two */
        bool dup = false;
        for (unsigned j = 0; j < i; j++) {
            if (frame->contacts[j].tip && frame->contacts[j].id == c->id) {
                dup = true;
                break;
            }
        }
        if (dup)
            continue;
        if (s < 0) {
            for (unsigned f = 0; f < HID_TOUCH_MAX_CONTACTS; f++) {
                if (!m->used[f]) {
                    s = (int)f;
                    break;
                }
            }
            if (s < 0)
                continue;   /* every slot busy: excess contact dropped */
            m->used[s] = true;
            m->id[s] = c->id;
            fresh = true;
        }
        cur[s].active = true;
        cur[s].fresh = fresh;
        cur[s].x = c->x;
        cur[s].y = c->y;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __HID_TOUCH_MAP_H__ */