/* Host test for the USB multi-touch path: the edge frame queue (edge
 * derivation, the slot-reuse lift, the overflow rule - a reader that falls
 * behind must still end up with the right contacts, through whole frames
 * only, and never with one finger taken for another), the device-id to
 * wire-slot mapping, and both together under a random contact stream.
 * No hardware. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <hid/hid_mt.h>
#include <hid/hid_touch.h>
#include <hid/hid_touch_map.h>

#define N             HID_MT_MAX_CONTACTS
#define REC(act, id)  (HID_MT_FRAME | ((act) << 8) | (id))

static hid_mt_t mt;
static hid_mt_contact_t cur[N];
/* Which finger the test has put in each slot. */
static unsigned finger[N];
static unsigned last_finger;

/* What a client makes of the stream, following xtouch's framing: a header
 * opens a frame, the frame takes effect once its last record arrived. */
static hid_mt_contact_t seen[N];
static hid_mt_contact_t pending[N];
static unsigned seen_presses[N];
static unsigned owed;
static unsigned frames_seen;
/* The client's view the last time it had caught up. */
static bool sync_active[N];
static unsigned sync_finger[N];
static unsigned sync_presses[N];

static void set(unsigned id, uint16_t x, uint16_t y) {
    if (!cur[id].active)
        finger[id] = ++last_finger;
    cur[id].active = true;
    cur[id].x = x;
    cur[id].y = y;
}

/* Another finger takes the slot without a free frame in between. */
static void hand_over(unsigned id, uint16_t x, uint16_t y) {
    finger[id] = ++last_finger;
    cur[id].active = true;
    cur[id].fresh = true;
    cur[id].x = x;
    cur[id].y = y;
}

static void lift(unsigned id) {
    cur[id].active = false;
}

static unsigned step(void) {
    unsigned n = hid_mt_update(&mt, cur, NULL);

    for (unsigned id = 0; id < N; id++)
        cur[id].fresh = false;
    assert(mt.count <= HID_MT_QUEUE_SIZE);
    return n;
}

static void expect(uint16_t state, uint16_t x, uint16_t y) {
    hid_mt_record_t rec;
    assert(hid_mt_pop(&mt, &rec));
    assert(rec.state == state && rec.x == x && rec.y == y);
}

static void client_read(unsigned max) {
    hid_mt_record_t rec;
    while (max-- != 0 && hid_mt_pop(&mt, &rec)) {
        unsigned id = rec.state & 0xffU;
        unsigned act = (rec.state >> 8) & 0x3U;

        if (rec.state == HID_MT_FRAME) {
            assert(owed == 0);              /* never a torn frame */
            assert(rec.x >= 1 && rec.x <= N);
            owed = rec.x;
            memcpy(pending, seen, sizeof(pending));
            continue;
        }
        assert((rec.state & HID_MT_FRAME) != 0 && owed != 0);
        assert(id < N);
        /* every edge must make sense against what the client holds */
        if (act == HID_MT_ACT_PRESS) {
            assert(!seen[id].active);
            seen_presses[id]++;
        } else {
            assert(seen[id].active);
        }
        pending[id].active = act != HID_MT_ACT_RELEASE;
        pending[id].x = rec.x;
        pending[id].y = rec.y;
        if (--owed == 0) {
            memcpy(seen, pending, sizeof(seen));
            frames_seen++;
        }
    }
}

/* The client has caught up: it holds the current contacts, and it was told
 * of every finger change - a slot that holds another finger than at the last
 * catch-up was pressed again (which the edge check above only allows after a
 * release), a slot that holds the same one was not. */
static void client_matches_cur(void) {
    assert(owed == 0 && mt.count == 0);
    for (unsigned id = 0; id < N; id++) {
        assert(seen[id].active == cur[id].active);
        if (cur[id].active) {
            assert(seen[id].x == cur[id].x && seen[id].y == cur[id].y);
            assert(mt.delivered[id].gen == mt.emitted[id].gen);
            if (sync_active[id] && sync_finger[id] == finger[id])
                assert(seen_presses[id] == sync_presses[id]);
            else
                assert(seen_presses[id] != sync_presses[id]);
        }
        sync_active[id] = cur[id].active;
        sync_finger[id] = finger[id];
        sync_presses[id] = seen_presses[id];
    }
}

static void reset(void) {
    hid_mt_init(&mt);
    memset(cur, 0, sizeof(cur));
    memset(seen, 0, sizeof(seen));
    memset(seen_presses, 0, sizeof(seen_presses));
    memset(sync_active, 0, sizeof(sync_active));
    memset(sync_finger, 0, sizeof(sync_finger));
    memset(sync_presses, 0, sizeof(sync_presses));
    owed = 0;
    frames_seen = 0;
}

static uint32_t rnd_state;

static unsigned rnd(unsigned n) {
    /* xorshift32: cheap and well mixed for the randomized streams below */
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 17;
    rnd_state ^= rnd_state << 5;
    return rnd_state % n;
}

/* ---------------- device-id to wire-slot mapping ---------------- */

static hid_touch_map_t map;

static void frame_one(touch_mt_evt_t* f, uint8_t id, uint16_t x, uint16_t y) {
    memset(f, 0, sizeof(*f));
    f->contact_count = 1;
    f->contacts[0].id = id;
    f->contacts[0].tip = 1;
    f->contacts[0].x = x;
    f->contacts[0].y = y;
}

static void map_test(void) {
    touch_mt_evt_t f;

    /* a new device id takes slot 0, fresh; while it stays down it keeps
       the slot, however the report orders its contacts */
    reset();
    hid_touch_map_init(&map);
    frame_one(&f, 5, 10, 20);
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[0].active && cur[0].fresh && cur[0].x == 10 && cur[0].y == 20);
    assert(!cur[1].active);
    frame_one(&f, 5, 11, 20);
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[0].active && !cur[0].fresh && cur[0].x == 11);

    /* a second id takes slot 1; swapping the report order swaps nothing
       on the wire */
    f.contact_count = 2;
    f.contacts[1].id = 9;
    f.contacts[1].tip = 1;
    f.contacts[1].x = 30;
    f.contacts[1].y = 40;
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[1].active && cur[1].fresh);
    assert(map.id[0] == 5 && map.id[1] == 9);
    f.contacts[0].id = 9;
    f.contacts[1].id = 5;
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[0].active && !cur[0].fresh && map.id[0] == 5);
    assert(cur[1].active && !cur[1].fresh && map.id[1] == 9);

    /* a listed contact with tip 0 releases its slot (after the swap,
       report slot 1 carries id 5, which lives on wire slot 0) */
    f.contacts[1].tip = 0;
    hid_touch_map_frame(&map, &f, cur);
    assert(!cur[0].active && cur[1].active && !map.used[0]);

    /* the id comes back: a new contact, fresh again */
    f.contacts[1].tip = 1;
    f.contacts[1].x = 77;
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[0].active && cur[0].fresh && cur[0].x == 77);

    /* an id simply missing from the report releases its slot too */
    f.contact_count = 1;
    hid_touch_map_frame(&map, &f, cur);
    assert(!cur[0].active && cur[1].active);

    /* a duplicated id inside one report is one contact */
    f.contact_count = 2;
    f.contacts[1] = f.contacts[0];
    hid_touch_map_frame(&map, &f, cur);
    assert(!cur[0].active && cur[1].active);

    /* a count larger than the slot array is clamped, never read past the
       frame (the wire frame carries at most HID_MT_MAX_CONTACTS slots) */
    memset(&f, 0, sizeof(f));
    f.contact_count = 255;
    for (unsigned i = 0; i < HID_MT_MAX_CONTACTS; i++) {
        f.contacts[i].id = (uint8_t)(100 + i);
        f.contacts[i].tip = 1;
    }
    hid_touch_map_frame(&map, &f, cur);
    {
        unsigned active = 0;
        for (unsigned s = 0; s < N; s++)
            active += cur[s].active;
        assert(active == N);
    }

    /* through the queue: a slot handed to another id lifts the old
       contact first, in a frame of its own */
    reset();
    hid_touch_map_init(&map);
    frame_one(&f, 5, 100, 100);
    hid_touch_map_frame(&map, &f, cur);
    assert(step() == 2);
    client_read(~0U);
    frame_one(&f, 9, 2000, 1500);
    hid_touch_map_frame(&map, &f, cur);
    assert(cur[0].fresh);
    assert(step() == 4);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_RELEASE, 0), 100, 100);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 0), 2000, 1500);
}

int main(void) {
    hid_mt_record_t rec;
    hid_mt_record_t queued[HID_MT_UPDATE_MAX];

    /* one finger: press, hold, move, release at the last position */
    reset();
    assert(step() == 0 && mt.count == 0);
    set(0, 100, 200);
    assert(hid_mt_update(&mt, cur, queued) == 2);
    assert(queued[0].state == HID_MT_FRAME && queued[0].x == 1);
    assert(queued[1].state == REC(HID_MT_ACT_PRESS, 0) &&
            queued[1].x == 100 && queued[1].y == 200);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 0), 100, 200);
    assert(step() == 0);                        /* parked: no edge */
    set(0, 101, 200);
    assert(step() == 2);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_MOVE, 0), 101, 200);
    lift(0);
    assert(step() == 2);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_RELEASE, 0), 101, 200);
    assert(!hid_mt_pop(&mt, &rec));
    assert(mt.presses == 1 && mt.releases == 1 && mt.drops == 0);

    /* several fingers share a frame; the slot index is the id */
    reset();
    set(2, 10, 20);
    set(7, 30, 40);
    assert(step() == 3);
    expect(HID_MT_FRAME, 2, 0);
    expect(REC(HID_MT_ACT_PRESS, 2), 10, 20);
    expect(REC(HID_MT_ACT_PRESS, 7), 30, 40);
    set(7, 31, 41);                             /* only 7 moved */
    set(9, 50, 60);
    lift(2);
    assert(step() == 4);
    expect(HID_MT_FRAME, 3, 0);
    expect(REC(HID_MT_ACT_RELEASE, 2), 10, 20);
    expect(REC(HID_MT_ACT_MOVE, 7), 31, 41);
    expect(REC(HID_MT_ACT_PRESS, 9), 50, 60);
    /* NULL is "no contact": everything lifts */
    assert(hid_mt_update(&mt, NULL, NULL) == 3);
    expect(HID_MT_FRAME, 2, 0);
    expect(REC(HID_MT_ACT_RELEASE, 7), 31, 41);
    expect(REC(HID_MT_ACT_RELEASE, 9), 50, 60);

    /* a slot handed to a new finger lifts first, in a frame of its own */
    reset();
    set(0, 100, 100);
    set(1, 900, 900);
    assert(step() == 3);
    client_read(~0U);
    hand_over(0, 2000, 1500);
    set(1, 901, 900);
    assert(step() == 5);
    expect(HID_MT_FRAME, 2, 0);
    expect(REC(HID_MT_ACT_RELEASE, 0), 100, 100);
    expect(REC(HID_MT_ACT_MOVE, 1), 901, 900);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 0), 2000, 1500);
    /* fresh on a free slot is a plain press */
    hand_over(3, 5, 5);
    assert(step() == 2);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 3), 5, 5);

    /* a reader that never reads: the queue stays bounded, and once it does
     * read it lands on the current contacts, lifts included */
    reset();
    set(0, 0, 0);
    set(1, 500, 500);
    for (unsigned i = 1; i <= 200; i++) {
        set(0, (uint16_t)i, (uint16_t)i);
        if (i == 150)
            lift(1);
        step();
    }
    assert(mt.drops != 0);
    client_read(~0U);
    client_matches_cur();
    assert(seen[0].active && seen[0].x == 200 && !seen[1].active);

    /* the reader holds a finger, the slot goes to another one, and the lift
     * between them is dropped with the overflow: the reader must still see
     * the first finger end where it last saw it, then the second begin */
    reset();
    set(0, 100, 100);
    step();
    client_read(~0U);
    client_matches_cur();
    hand_over(0, 2000, 1500);
    assert(step() == 4);                        /* queued, never read */
    for (unsigned i = 1; mt.drops == 0; i++) {
        assert(i < 100);
        set(0, (uint16_t)(2000 + i), 1500);
        step();
    }
    assert(mt.count == 4);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_RELEASE, 0), 100, 100);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 0), cur[0].x, 1500);
    /* ... and from there on it is one finger again */
    set(0, 9, 9);
    assert(step() == 2);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_MOVE, 0), 9, 9);

    /* same when the first finger lifted and the slot was pressed again, the
     * release and the press both among the dropped frames */
    reset();
    set(0, 100, 100);
    step();
    client_read(~0U);
    client_matches_cur();
    lift(0);
    assert(step() == 2);
    set(0, 500, 500);
    assert(step() == 2);
    for (unsigned i = 1; mt.drops == 0; i++) {
        assert(i < 100);
        set(0, (uint16_t)(500 + i), 500);
        step();
    }
    assert(mt.count == 4);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_RELEASE, 0), 100, 100);
    expect(HID_MT_FRAME, 1, 0);
    expect(REC(HID_MT_ACT_PRESS, 0), cur[0].x, 500);

    /* the reader stopped in the middle of a frame: that frame is finished
     * before anything newer, and a slot it presses is lifted again when the
     * finger there has changed since */
    reset();
    set(0, 1, 1);
    set(1, 2, 2);
    set(2, 3, 3);
    step();
    client_read(2);                             /* header + first press */
    assert(owed == 2 && mt.head_left == 2);
    /* run on until an update overflows, so the queue is exactly a rebuild */
    for (unsigned i = 0, drops = 0; i < 200 || mt.drops == drops; i++) {
        assert(i < 400);
        drops = mt.drops;
        set(0, (uint16_t)(10 + i), 1);
        set(2, 3, (uint16_t)(10 + i));
        if (i == 100)
            lift(1);
        if (i == 120)
            set(1, 77, 88);                     /* back on the same slot */
        if (i == 150)
            hand_over(2, 900, 900);
        step();
    }
    assert(mt.drops != 0);
    expect(REC(HID_MT_ACT_PRESS, 1), 2, 2);   /* the frame it was in */
    expect(REC(HID_MT_ACT_PRESS, 2), 3, 3);
    expect(HID_MT_FRAME, 3, 0);
    expect(REC(HID_MT_ACT_MOVE, 0), cur[0].x, 1);
    expect(REC(HID_MT_ACT_RELEASE, 1), 2, 2);
    expect(REC(HID_MT_ACT_RELEASE, 2), 3, 3);
    expect(HID_MT_FRAME, 2, 0);
    expect(REC(HID_MT_ACT_PRESS, 1), 77, 88);
    expect(REC(HID_MT_ACT_PRESS, 2), 3, cur[2].y);
    assert(!hid_mt_pop(&mt, &rec));

    /* a reader that keeps up, slowly: one record per update, long run */
    reset();
    for (unsigned i = 0; i < 2000; i++) {
        unsigned id = i % N;
        if ((i / 7U) % 3U == 0)
            lift(id);
        else
            set(id, (uint16_t)(i * 3U), (uint16_t)(i * 5U));
        step();
        client_read(1);
    }
    client_read(~0U);
    client_matches_cur();
    assert(frames_seen != 0);

    /* everything lifts through a full queue too */
    for (unsigned i = 0; i < 100; i++) {
        set(4, (uint16_t)i, 9);
        step();
    }
    memset(cur, 0, sizeof(cur));
    hid_mt_update(&mt, NULL, NULL);
    client_read(~0U);
    client_matches_cur();

    /* fingers coming, going and trading slots at random under a reader
     * that stalls, trickles and catches up */
    reset();
    rnd_state = 20261118U;
    {
        unsigned mode = 0;
        unsigned catchups = 0;

        for (unsigned i = 0; i < 200000; i++) {
            for (unsigned id = 0; id < N; id++) {
                unsigned r = rnd(id < 3 ? 16 : 64);
                uint16_t x = (uint16_t)rnd(2560);
                uint16_t y = (uint16_t)rnd(1600);

                if (!cur[id].active) {
                    if (r < 2)
                        set(id, x, y);
                } else if (r < 2) {
                    lift(id);
                } else if (r == 2) {
                    hand_over(id, x, y);
                } else if (r < 10) {
                    set(id, x, y);
                }
            }
            step();
            if (i % 37U == 0)
                mode = rnd(4);
            if (mode == 1)
                client_read(1);
            else if (mode == 2)
                client_read(rnd(8));
            else if (mode == 3 || i % 997U == 0)
                client_read(~0U);
            if (mt.count == 0 && owed == 0) {
                client_matches_cur();
                catchups++;
            }
        }
        client_read(~0U);
        client_matches_cur();
        assert(mt.drops > 100 && catchups > 100 && frames_seen > 1000);
    }

    /* the mapping unit cases */
    map_test();

    /* end to end: device ids come and go in random report order, the map
     * places them on wire slots and the queue carries the edges to a
     * reader that stalls, trickles and catches up.  A device id that lifts
     * and presses again is a new contact even on the same wire slot. */
    reset();
    hid_touch_map_init(&map);
    rnd_state = 880311U;
    {
        bool down[256];
        bool was_down[256];
        unsigned gen_of[256];
        unsigned next_gen = 0;
        unsigned mode = 0;
        unsigned catchups = 0;
        int last_lifted = -1;

        memset(down, 0, sizeof(down));
        memset(was_down, 0, sizeof(was_down));
        memset(gen_of, 0, sizeof(gen_of));
        for (unsigned i = 0; i < 100000; i++) {
            touch_mt_evt_t f;
            unsigned ids[16];
            unsigned ndown = 0;

            /* churn the device-side contact set (a small id pool so the
               contacts actually come and go; ids are device-defined and
               need not be small, the map must handle any of them) */
            for (unsigned k = 0; k < 3; k++) {
                unsigned id = 40 + rnd(16);
                if (down[id]) {
                    if (rnd(2) == 0) {
                        down[id] = false;
                        last_lifted = (int)id;
                    }
                } else if (rnd(2) == 0) {
                    down[id] = true;
                }
            }
            /* build the report: the down ids in random order, sometimes a
               lifted one still listed with tip 0 (the frame carries at
               most HID_MT_MAX_CONTACTS slots, normalize clamps the count
               to that before the map ever sees it) */
            memset(&f, 0, sizeof(f));
            for (unsigned id = 40; id < 56; id++) {
                if (down[id] && ndown < HID_MT_MAX_CONTACTS)
                    ids[ndown++] = id;
            }
            for (unsigned j = ndown; j > 1; j--) {
                unsigned r = rnd(j);
                unsigned t = ids[j - 1];
                ids[j - 1] = ids[r];
                ids[r] = t;
            }
            for (unsigned j = 0; j < ndown; j++) {
                f.contacts[j].id = (uint8_t)ids[j];
                f.contacts[j].tip = 1;
                f.contacts[j].x = (uint16_t)rnd(4096);
                f.contacts[j].y = (uint16_t)rnd(4096);
            }
            /* a lifted id may still be listed with tip 0 - but only while
               it is really up: the churn can lift and re-press the same id
               within one round, and a report never lists one id both down
               and up */
            if (last_lifted >= 0 && !down[last_lifted] &&
                    ndown < HID_MT_MAX_CONTACTS && rnd(4) == 0) {
                f.contacts[ndown].id = (uint8_t)last_lifted;
                f.contacts[ndown].tip = 0;
                ndown++;
            }
            f.contact_count = (uint8_t)ndown;

            /* a contact is a new finger only when it appears after being
               absent: a lift and re-press inside one report interval is
               invisible to the host, exactly as on a real device */
            memset(was_down, 0, sizeof(was_down));
            for (unsigned j = 0; j < ndown; j++) {
                unsigned id = f.contacts[j].id;
                if (f.contacts[j].tip) {
                    was_down[id] = true;
                    if (gen_of[id] == 0)
                        gen_of[id] = ++next_gen;
                }
            }
            for (unsigned id = 40; id < 56; id++) {
                if (!was_down[id])
                    gen_of[id] = 0;
            }

            hid_touch_map_frame(&map, &f, cur);
            for (unsigned s = 0; s < N; s++)
                finger[s] = map.used[s] ? gen_of[map.id[s]] : 0;
            step();
            if (i % 37U == 0)
                mode = rnd(4);
            if (mode == 1)
                client_read(1);
            else if (mode == 2)
                client_read(rnd(8));
            else if (mode == 3 || i % 997U == 0)
                client_read(~0U);
            if (mt.count == 0 && owed == 0) {
                client_matches_cur();
                catchups++;
            }
        }
        client_read(~0U);
        client_matches_cur();
        assert(catchups > 100 && frames_seen > 1000);
    }

    puts("hid_mt_test: all checks passed");
    return 0;
}