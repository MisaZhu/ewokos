#ifndef __HID_MT_H__
#define __HID_MT_H__
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Multi-touch edge frames for /dev/touch0 (no hardware, host-testable).
 *
 * The record stream is the one xtouch reads from the goodixd/himaxd panels:
 * three uint16_t fields per record, a header followed by one record per
 * contact edge of the frame.
 *
 *   state HID_MT_FRAME ........ header: x = record count, y = 0
 *   state HID_MT_FRAME | (act << 8) | id ... contact record, act is
 *     HID_MT_ACT_PRESS/MOVE/RELEASE, id 0..9; x/y are device coordinates
 *     (a release carries the contact's last known position)
 *
 * Only edges are sent: a contact that did not move produces no record, and a
 * frame with no edge at all produces nothing.  Gesture policy and
 * single-finger mouse compatibility live in xtouch/xserverd.
 *
 * Nothing here locks.  hid_mt_update() and hid_mt_pop() must not run inside
 * one another: the caller serialises them (in the daemon the pop is an IPC
 * handler that can cut into the report loop anywhere).
 */
#define HID_MT_FRAME          0x8000U
#define HID_MT_ACT_PRESS      1U
#define HID_MT_ACT_MOVE       2U
#define HID_MT_ACT_RELEASE    3U
#define HID_MT_MAX_CONTACTS   10U
#define HID_MT_QUEUE_SIZE     32U
/* Most records one update queues: a lift frame and the frame proper. */
#define HID_MT_UPDATE_MAX     (2U * (1U + HID_MT_MAX_CONTACTS))

/* An update must fit behind the rest of a frame the reader is half through. */
#if HID_MT_QUEUE_SIZE < HID_MT_UPDATE_MAX + HID_MT_MAX_CONTACTS
#error "HID_MT_QUEUE_SIZE cannot hold a half-read frame plus one update"
#endif

typedef struct {
    uint16_t state;
    uint16_t x;
    uint16_t y;
} hid_mt_record_t;

/* One wire slot as the caller sees it; the slot index is the contact id. */
typedef struct {
    bool active;
    /* The slot holds a contact that started this frame, even if the slot
     * was in use the frame before.  Set it for that one frame only. */
    bool fresh;
    uint16_t x;
    uint16_t y;
} hid_mt_contact_t;

/* A slot as the stream knows it.  `gen` names the contact: every press gets
 * a new one, so two fingers that used the same slot are never taken for one,
 * however many frames between them were dropped. */
typedef struct {
    bool active;
    unsigned gen;
    uint16_t x;
    uint16_t y;
} hid_mt_slot_t;

typedef struct {
    hid_mt_record_t rec;
    unsigned gen;
} hid_mt_entry_t;

typedef struct {
    hid_mt_entry_t queue[HID_MT_QUEUE_SIZE];
    unsigned count;
    /* Records of the frame at the head of the queue whose header the reader
     * already took; 0 when the queue starts on a frame boundary. */
    unsigned head_left;
    /* Contact set after every queued frame / after the frames read so far. */
    hid_mt_slot_t emitted[HID_MT_MAX_CONTACTS];
    hid_mt_slot_t delivered[HID_MT_MAX_CONTACTS];
    unsigned last_gen;
    unsigned presses;
    unsigned releases;
    unsigned drops;
} hid_mt_t;

void hid_mt_init(hid_mt_t* mt);
/* Queue the frames that take the reader to `cur` (HID_MT_MAX_CONTACTS
 * slots, NULL for no contact).  Returns the number of records queued and
 * copies them to `queued` (room for HID_MT_UPDATE_MAX) unless it is NULL. */
unsigned hid_mt_update(hid_mt_t* mt, const hid_mt_contact_t* cur,
        hid_mt_record_t* queued);
/* Take the oldest record; false when the queue is empty. */
bool hid_mt_pop(hid_mt_t* mt, hid_mt_record_t* out);

#ifdef __cplusplus
}
#endif

#endif /* __HID_MT_H__ */