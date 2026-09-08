#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <ewoksys/klog.h>
#include <ewoksys/vfs.h>

#include "../platform.h"

#include "util.h"
#include "sock.h"
#include "net.h"
#include "ip.h"
#include "tcp.h"
#include "../task.h"

#define TCP_FLG_FIN 0x01
#define TCP_FLG_SYN 0x02
#define TCP_FLG_RST 0x04
#define TCP_FLG_PSH 0x08
#define TCP_FLG_ACK 0x10
#define TCP_FLG_URG 0x20

#define TCP_OPT_EOL 0
#define TCP_OPT_NOP 1
#define TCP_OPT_MSS 2
#define TCP_OPT_WS 3
#define TCP_OPT_MSS_LEN 4
#define TCP_OPT_WS_LEN 3
#define TCP_MAX_WSCALE 14

#define TCP_FLG_IS(x, y) ((x & 0x3f) == (y))
#define TCP_FLG_ISSET(x, y) ((x & 0x3f) & (y) ? 1 : 0)

#define TCP_PCB_SIZE 64

#define TCP_PCB_MODE_RFC793 1
#define TCP_PCB_MODE_SOCKET 2

#define TCP_PCB_STATE_FREE         0
#define TCP_PCB_STATE_CLOSED       1
#define TCP_PCB_STATE_LISTEN       2
#define TCP_PCB_STATE_SYN_SENT     3
#define TCP_PCB_STATE_SYN_RECEIVED 4
#define TCP_PCB_STATE_ESTABLISHED  5
#define TCP_PCB_STATE_FIN_WAIT1    6
#define TCP_PCB_STATE_FIN_WAIT2    7
#define TCP_PCB_STATE_CLOSING      8
#define TCP_PCB_STATE_TIME_WAIT    9
#define TCP_PCB_STATE_CLOSE_WAIT  10
#define TCP_PCB_STATE_LAST_ACK    11

#define TCP_DEFAULT_RTO 200000 /* micro seconds */
#define TCP_RETRANSMIT_DEADLINE 60 /* seconds - increased for better reliability */
#define TCP_TIMEWAIT_SEC 30 /* substitute for 2MSL */
/*
 * With WSOPT enabled the peer can legitimately advertise far more than the
 * legacy 64KB window, so the retransmit queue must no longer be sized for a
 * 16-bit window only. The timer path now drives retransmission from the queue
 * head instead of walking the whole list every tick, which removes the old
 * CPU/lock reason for keeping this cap artificially tiny. 512 entries provide
 * ~700KB of Ethernet-MTU payload in flight, enough to test beyond the current
 * ~550KB/s plateau while still keeping worst-case per-connection memory sane.
 */
#define TCP_RETRANSMIT_QUEUE_MAX 512 /* max queued segments per connection */
#define TCP_PERSIST_RTO_MAX 60000000 /* max persist backoff: 60 seconds */
#define TCP_DELACK_TIMEOUT_USEC 40000 /* RFC 1122 delayed-ACK ceiling */
#define TCP_DELACK_SEGS 2 /* ACK every 2nd in-order data segment */
/*
 * Download flow control (direction-aware, see the pcb->buf comment).
 * The advertised receive window gates inbound throughput:
 * throughput = window / effective_RTT, and the wland<->netd<->sshd IPC
 * chain makes that RTT ~100ms, so the legacy 32KB window pinned scp/ssh
 * downloads near 300KB/s regardless of SDIO clock or SDMA. Advertise RFC
 * 7323 window scaling with a real shift and size the buffer well past the
 * bandwidth-delay product so the sender keeps the pipe full while sshd
 * drains it. 256KB >> 3 = 32KB still fits the 16-bit window field. This is
 * safe for the WLAN TX-credit pool: during a download credits refill from
 * the very RX frames being ACKed, and delayed ACK emits at most one ACK per
 * two inbound segments, so ACKs can never outrun credit refresh. Upload is
 * unaffected - it is gated by the PEER's advertised window, not ours. */
#define TCP_RCV_BUF_SIZE (1024*256)
#define TCP_RCV_WSCALE   3
#define TCP_SOURCE_PORT_MIN 49152
#define TCP_SOURCE_PORT_MAX 65535

struct pseudo_hdr {
    uint32_t src;
    uint32_t dst;
    uint8_t zero;
    uint8_t protocol;
    uint16_t len;
};

struct tcp_hdr {
    uint16_t src;
    uint16_t dst;
    uint32_t seq;
    uint32_t ack;
    uint8_t off;
    uint8_t flg;
    uint16_t wnd;
    uint16_t sum;
    uint16_t up;
};

struct tcp_segment_info {
    uint32_t seq;
    uint32_t ack;
    uint16_t len;
    uint16_t wnd;
    uint16_t up;
    uint8_t wscale;
    uint8_t has_wscale;
};

struct tcp_pcb {
    int state;
    int mode; /* user command mode */
    int close_reason; /* 0=normal, 1=RST, 2=timeout */
    struct ip_endpoint local;
    struct ip_endpoint foreign;
    struct {
        uint32_t nxt;
        uint32_t una;
        uint32_t wnd;
        uint16_t up;
        uint32_t wl1;
        uint32_t wl2;
    } snd;
    uint32_t iss;
    struct {
        uint32_t nxt;
        uint32_t wnd; /* advertised receive window; scaled onto the wire by TCP_RCV_WSCALE */
        uint16_t up;
    } rcv;
    uint32_t irs;
    uint16_t mtu;
    uint16_t mss;
    /* Receive buffer / advertised window.
     * Downloads are RTT-bound: throughput = advertised window / effective
     * RTT, and the wland<->netd<->sshd IPC chain makes that RTT ~100ms, so a
     * 32KB window capped scp/ssh downloads near 300KB/s no matter the SDIO
     * clock. Size the buffer (and, via TCP_RCV_WSCALE, the advertised window)
     * well past the bandwidth-delay product so the sender streams
     * continuously while sshd drains the buffer; tcp_receive() emits a
     * window-update ACK on every drain, keeping the window advancing.
     * Direction note: this large window helps download only. Upload is gated
     * by the PEER's advertised window, and the old WLAN TX-credit-exhaustion
     * hazard does not apply here - during a download credits refill from the
     * inbound frames themselves and delayed ACK (TCP_DELACK_SEGS=2) emits at
     * most one ACK per two inbound segments, so ACKs never outrun refresh. */
    uint8_t buf[TCP_RCV_BUF_SIZE]; /* receive buffer */
    struct sched_ctx state_ctx;
    struct sched_ctx send_ctx;
    struct sched_ctx recv_ctx;
    struct queue_head queue; /* retransmit queue */
    struct timeval tw_timer;
    struct timeval persist_timer; /* zero-window probe timer */
    unsigned int persist_rto;    /* current persist backoff (usec) */
    uint8_t persist_probing;     /* persist timer armed */
    uint8_t delack_pending;      /* delayed ACK armed (RFC 1122) */
    uint8_t delack_count;        /* in-order segs awaiting ACK */
    struct timeval delack_timer; /* first unacked seg arrival time */
    uint8_t dupacks;             /* consecutive dup ACKs (fast retransmit) */
    uint8_t peer_wscale;         /* peer's receive-window shift (RFC 7323) */
    uint8_t wsopt_ok;            /* true once both SYNs carried WSOPT */
    uint32_t ooo_seq;            /* out-of-order stash: first seq (valid if ooo_len) */
    uint32_t ooo_len;            /* out-of-order stash: byte count, 0 = empty */
    struct tcp_pcb *parent;
    struct queue_head backlog;
};

struct tcp_queue_entry {
    struct timeval first;
    struct timeval last;
    unsigned int rto; /* micro seconds */
    uint32_t seq;
    uint8_t flg;
    size_t len;
};

static mutex_t mutex = MUTEX_INITIALIZER;
static struct tcp_pcb pcbs[TCP_PCB_SIZE];
static inline void tcp_sched_wakeup_all(struct tcp_pcb *pcb)
{
    sched_wakeup(&pcb->state_ctx);
    sched_wakeup(&pcb->send_ctx);
    sched_wakeup(&pcb->recv_ctx);
}

static inline void tcp_sched_interrupt_all(struct tcp_pcb *pcb)
{
    sched_interrupt(&pcb->state_ctx);
    sched_interrupt(&pcb->send_ctx);
    sched_interrupt(&pcb->recv_ctx);
}

static int
tcp_due_from_deadline(const struct timeval *now, const struct timeval *deadline)
{
    struct timeval diff;

    if (timercmp(deadline, now, <=) != 0) {
        return 0;
    }
    timersub(deadline, now, &diff);
    if (diff.tv_sec > INT32_MAX / 1000000) {
        return INT32_MAX;
    }
    return (int)(diff.tv_sec * 1000000 + diff.tv_usec);
}

static void
tcp_due_track_min(int *min_due_us, int due_us)
{
    if (due_us < 0) {
        return;
    }
    if (*min_due_us < 0 || due_us < *min_due_us) {
        *min_due_us = due_us;
    }
}

static int
tcp_pcb_used_count(void)
{
    int used = 0;
    struct tcp_pcb *pcb;

    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if (pcb->state != TCP_PCB_STATE_FREE) {
            used++;
        }
    }
    return used;
}

static ssize_t
tcp_output_segment(uint32_t seq, uint32_t ack, uint8_t flg, uint16_t wnd, uint8_t *data, size_t len, struct ip_endpoint *local, struct ip_endpoint *foreign);

static void tcp_persist_arm(struct tcp_pcb *pcb);
static void tcp_persist_disarm(struct tcp_pcb *pcb);

/* stack/sock.c reverse map: which socket (if any) owns a tcp desc. Takes the
 * leaf socks_lock; safe under the stack mutex (same order as the
 * task_wakeup_* paths). Used by the tcp_timer() orphan reaper. */
extern int sock_id_from_tcp_desc(int tcp_desc);

static uint32_t
tcp_peer_window(const struct tcp_pcb *pcb, uint16_t wnd)
{
    if (pcb && pcb->wsopt_ok) {
        return ((uint32_t)wnd) << pcb->peer_wscale;
    }
    return wnd;
}

/*
 * Value for the 16-bit TCP window field of an outgoing segment. RFC 7323:
 * the window scale is negotiated on the SYN and applies only to segments
 * AFTER the SYN, so the SYN itself must carry the raw (unscaled) window;
 * every later segment is right-shifted by our advertised scale, and only
 * when the peer also sent WSOPT (wsopt_ok). Clamp to 0xffff so an
 * over-large receive buffer can never overflow the field.
 */
static uint16_t
tcp_wire_window(const struct tcp_pcb *pcb, uint8_t flg)
{
    uint32_t w = pcb->rcv.wnd;

    if (!TCP_FLG_ISSET(flg, TCP_FLG_SYN) && pcb->wsopt_ok) {
        w >>= TCP_RCV_WSCALE;
    }
    return w > 0xffff ? (uint16_t)0xffff : (uint16_t)w;
}

static void
tcp_parse_syn_options(const struct tcp_hdr *hdr, uint16_t hlen,
                      uint8_t *wscale, uint8_t *has_wscale)
{
    const uint8_t *opt;
    uint16_t remain;

    *wscale = 0;
    *has_wscale = 0;
    if (hlen <= sizeof(*hdr)) {
        return;
    }

    opt = (const uint8_t *)(hdr + 1);
    remain = hlen - (uint16_t)sizeof(*hdr);
    while (remain > 0) {
        uint8_t kind = opt[0];

        if (kind == TCP_OPT_EOL) {
            break;
        }
        if (kind == TCP_OPT_NOP) {
            opt++;
            remain--;
            continue;
        }
        if (remain < 2 || opt[1] < 2 || opt[1] > remain) {
            break;
        }
        if (kind == TCP_OPT_WS && opt[1] == TCP_OPT_WS_LEN) {
            *wscale = opt[2] > TCP_MAX_WSCALE ? TCP_MAX_WSCALE : opt[2];
            *has_wscale = 1;
        }
        remain -= opt[1];
        opt += opt[1];
    }
}

static char *
tcp_flg_ntoa(uint8_t flg)
{
    static char str[9];

    sprintf(str, "--%c%c%c%c%c%c",
        TCP_FLG_ISSET(flg, TCP_FLG_URG) ? 'U' : '-',
        TCP_FLG_ISSET(flg, TCP_FLG_ACK) ? 'A' : '-',
        TCP_FLG_ISSET(flg, TCP_FLG_PSH) ? 'P' : '-',
        TCP_FLG_ISSET(flg, TCP_FLG_RST) ? 'R' : '-',
        TCP_FLG_ISSET(flg, TCP_FLG_SYN) ? 'S' : '-',
        TCP_FLG_ISSET(flg, TCP_FLG_FIN) ? 'F' : '-');
    return str;
}

static void
tcp_dump(const uint8_t *data, size_t len)
{
#ifdef NET_DEBUG
    struct tcp_hdr *hdr;
    hdr = (struct tcp_hdr *)data;
    slog( "        src: %u\n", ntoh16(hdr->src));
    slog( "        dst: %u\n", ntoh16(hdr->dst));
    slog( "        seq: %u\n", ntoh32(hdr->seq));
    slog( "        ack: %u\n", ntoh32(hdr->ack));
    slog( "        off: 0x%02x (%d)\n", hdr->off, (hdr->off >> 4) << 2);
    slog( "        flg: 0x%02x (%s)\n", hdr->flg, tcp_flg_ntoa(hdr->flg));
    slog( "        wnd: %u\n", ntoh16(hdr->wnd));
    slog( "        sum: 0x%04x\n", ntoh16(hdr->sum));
    slog( "         up: %u\n", ntoh16(hdr->up));
#ifdef HEXDUMP
    hexdump(stderr, data, len);
#endif
#endif
}

/*
 * TCP Protocol Control Block (PCB)
 *
 * NOTE: TCP PCB functions must be called after mutex locked
 */

static int pcb_alloc_count = 0;
static int pcb_release_count = 0;

static struct tcp_pcb *
tcp_pcb_alloc(void)
{
    struct tcp_pcb *pcb;
    int free_count = 0;

    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if (pcb->state == TCP_PCB_STATE_FREE) {
            /*
             * Fully reset the reused slot. tcp_pcb_release() already drained
             * pcb->queue and pcb->backlog and destroyed the sched_ctx (waiters
             * gone), so zeroing here is safe and avoids the release-time race
             * the release path warns about. Without this, stale snd.wnd/wl1/wl2
             * from the previous connection freeze the send window on reuse and
             * hang the next connection's writes.
             */
            memset(pcb, 0, sizeof(*pcb));
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 0;
            sched_ctx_init(&pcb->state_ctx);
            sched_ctx_init(&pcb->send_ctx);
            sched_ctx_init(&pcb->recv_ctx);
            pcb_alloc_count++;
            infof("tcp_pcb_alloc: total=%d, released=%d, free=%d/%zu",
                  pcb_alloc_count, pcb_release_count, free_count, tailof(pcbs));
            return pcb;
        }
        free_count++;
    }
    errorf("tcp_pcb_alloc: no free PCB! total=%d, released=%d",
           pcb_alloc_count, pcb_release_count);
    return NULL;
}

static void
tcp_pcb_release(struct tcp_pcb *pcb)
{
    struct queue_entry *entry;
    struct tcp_pcb *est;
    char ep1[IP_ENDPOINT_STR_LEN];
    char ep2[IP_ENDPOINT_STR_LEN];

    /*
     * Release only destroys the internal sched_ctx below, which reaches a
     * worker sleeping inside tcp_receive(). A client blocked in poll(POLLIN)
     * or poll(POLLOUT) with an idle net task is on the VFS wait queue and only
     * a vfs_wakeup() edge can release it; without this a peer RST/timeout would
     * strand it forever. Raise both edges so pending reads return EOF/error and
     * pending writes return the connection error on their next retry.
     */
    task_wakeup_tcp_readers(indexof(pcbs, pcb));
    task_wakeup_tcp_writers(indexof(pcbs, pcb));

    /*
     * A not-yet-accepted child sits in its parent's backlog by bare pointer.
     * Releasing it here (peer RST before accept, orphan reap) without
     * unlinking left a dangling entry: tcp_accept() would later pop a FREE
     * or reused pcb and hand a stale/foreign connection to the caller.
     * queue_remove() is a no-op when the child was already popped or the
     * parent's own release drained it first.
     */
    if (pcb->parent) {
        queue_remove(&pcb->parent->backlog, pcb);
        pcb->parent = NULL;
    }

    tcp_persist_disarm(pcb);

    // First, clean up all resources regardless of sched_ctx_destroy result
    // Clean up retransmit queue
    while ((entry = queue_pop(&pcb->queue)) != NULL) {
        memory_free(entry);
    }
    // Clean up backlog connections
    while ((est = queue_pop(&pcb->backlog)) != NULL) {
        tcp_pcb_release(est);
    }

    // CRITICAL: Wake up any process waiting on this pcb before releasing it.
    // This prevents the race condition where sched_sleep is blocked on this ctx
    // and the pcb is released, causing sched_sleep to access freed memory.
    // sched_ctx_destroy will wait for all waiters to exit before returning.
    sched_ctx_destroy(&pcb->state_ctx);
    sched_ctx_destroy(&pcb->send_ctx);
    sched_ctx_destroy(&pcb->recv_ctx);

    pcb_release_count++;
    infof("tcp_pcb_release: total=%d, released=%d, state=%d, local=%s, foreign=%s",
          pcb_alloc_count, pcb_release_count, pcb->state,
          ip_endpoint_ntop(&pcb->local, ep1, sizeof(ep1)), ip_endpoint_ntop(&pcb->foreign, ep2, sizeof(ep2)));
    
    // DO NOT memset the pcb here!
    // There is a race condition where sched_sleep in another thread
    // may still be accessing one of the pcb sched_ctx objects after
    // sched_ctx_destroy returns.
    // Instead, just mark the pcb as FREE and let tcp_pcb_alloc reuse it.
    pcb->state = TCP_PCB_STATE_FREE;
}

static struct tcp_pcb *
tcp_pcb_select(struct ip_endpoint *local, struct ip_endpoint *foreign)
{
    struct tcp_pcb *pcb, *listen_pcb = NULL;

    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if ((pcb->local.addr == IP_ADDR_ANY || pcb->local.addr == local->addr) && pcb->local.port == local->port) {
            if (!foreign) {
                return pcb;
            }
            if (pcb->foreign.addr == foreign->addr && pcb->foreign.port == foreign->port) {
                return pcb;
            }
            if (pcb->state == TCP_PCB_STATE_LISTEN) {
                if (pcb->foreign.addr == IP_ADDR_ANY && pcb->foreign.port == 0) {
                    /* LISTENed with wildcard foreign address/port */
                    listen_pcb = pcb;
                }
            }
        }
    }
    return listen_pcb;
}

static struct tcp_pcb *
tcp_pcb_get(int id)
{
    struct tcp_pcb *pcb;

    if (id < 0 || id >= (int)countof(pcbs)) {
        /* out of range */
        return NULL;
    }
    pcb = &pcbs[id];
    if (pcb->state == TCP_PCB_STATE_FREE) {
        return NULL;
    }
    return pcb;
}

static int
tcp_pcb_id(struct tcp_pcb *pcb)
{
    return indexof(pcbs, pcb);
}

static const char *
tcp_close_reason_ntoa(int reason)
{
    switch (reason) {
    case 0:
        return "normal";
    case 1:
        return "rst";
    case 2:
        return "timeout";
    case 3:
        return "output_failure";
    case 4:
        return "interrupted";
    default:
        return "unknown";
    }
}

static void
tcp_log_close_event(const char *tag, struct tcp_pcb *pcb)
{
    char ep1[IP_ENDPOINT_STR_LEN];
    char ep2[IP_ENDPOINT_STR_LEN];

    (void)tag;
    (void)pcb;
    (void)ep1;
    (void)ep2;
}

/*
 * TCP Retransmit
 *
 * NOTE: TCP Retransmit functions must be called after mutex locked
 */
static int
tcp_retransmit_queue_add(struct tcp_pcb *pcb, uint32_t seq, uint8_t flg, uint8_t *data, size_t len)
{
    struct tcp_queue_entry *entry;
    if (pcb->queue.num >= TCP_RETRANSMIT_QUEUE_MAX) {
        errorf("retransmit queue full (%u), dropping segment seq=%u",
               pcb->queue.num, seq);
        return -1;
    }
    entry = memory_alloc(sizeof(*entry) + len);
    if (!entry) {
        errorf("memory_alloc() failure");
        return -1;
    }
    entry->rto = TCP_DEFAULT_RTO;
    entry->seq = seq;
    entry->flg = flg;
    entry->len = len;
    memcpy(entry + 1, data, entry->len);
    gettimeofday(&entry->first, NULL);
    entry->last = entry->first;
    if (!queue_push(&pcb->queue, entry)) {
        errorf("queue_push() failure");
        memory_free(entry);
        return -1;
    }
    return 0;
}

static void
tcp_retransmit_queue_cleanup(struct tcp_pcb *pcb)
{
    struct tcp_queue_entry *entry;

    while ((entry = queue_peek(&pcb->queue))) {
        /*
         * Serial (mod 2^32) comparison. iss = random(), so snd.una can wrap
         * within a session; a raw `entry->seq >= pcb->snd.una` then keeps
         * fully-ACKed entries queued forever, the queue sticks at
         * TCP_RETRANSMIT_QUEUE_MAX, every send returns EAGAIN and the
         * connection stalls permanently while probe/wake churn spins netd.
         */
        if ((int32_t)(entry->seq - pcb->snd.una) >= 0) {
            break;
        }
        entry = queue_pop(&pcb->queue);
        debugf("remove, seq=%u, flags=%s, len=%zu", entry->seq, tcp_flg_ntoa(entry->flg), entry->len);
        memory_free(entry);
    }
    return;
}

static void
tcp_retransmit_queue_emit(void *arg, void *data)
{
    struct tcp_pcb *pcb;
    struct tcp_queue_entry *entry;
    struct timeval now, diff, timeout;

    pcb = (struct tcp_pcb *)arg;
    entry = (struct tcp_queue_entry *)data;
    gettimeofday(&now, NULL);
    timersub(&now, &entry->first, &diff);
    if (diff.tv_sec >= TCP_RETRANSMIT_DEADLINE) {
        pcb->state = TCP_PCB_STATE_CLOSED;
        pcb->close_reason = 2; /* timeout */
        tcp_log_close_event("tcp_retransmit timeout close", pcb);
        tcp_sched_wakeup_all(pcb);
        return;
    }
    timeout = entry->last;
    timeval_add_usec(&timeout, entry->rto);
    if (timercmp(&now, &timeout, >)) {
        tcp_output_segment(entry->seq, pcb->rcv.nxt, entry->flg, tcp_wire_window(pcb, entry->flg), (uint8_t *)(entry+1), entry->len, &pcb->local, &pcb->foreign);
        entry->last = now;
        entry->rto *= 2;
    }
}

/*
 * TCP Persist Timer (Zero-Window Probe)
 *
 * When the send window closes (snd.wnd == 0) and all data is ACKed, the sender
 * must periodically probe the receiver to detect window reopening. Without this,
 * a lost window-update from the receiver causes the sender to wait forever.
 */
static void
tcp_persist_arm(struct tcp_pcb *pcb)
{
    if (pcb->persist_probing)
        return;
    pcb->persist_probing = 1;
    pcb->persist_rto = TCP_DEFAULT_RTO;
    gettimeofday(&pcb->persist_timer, NULL);
    timeval_add_usec(&pcb->persist_timer, pcb->persist_rto);
}

static void
tcp_persist_disarm(struct tcp_pcb *pcb)
{
    pcb->persist_probing = 0;
}

static void
tcp_set_timewait_timer(struct tcp_pcb *pcb)
{
    gettimeofday(&pcb->tw_timer, NULL);
    pcb->tw_timer.tv_sec += TCP_TIMEWAIT_SEC;
    debugf("start time_wait timer: %d seconds", TCP_TIMEWAIT_SEC);
}

static ssize_t
tcp_output_segment(uint32_t seq, uint32_t ack, uint8_t flg, uint16_t wnd, uint8_t *data, size_t len, struct ip_endpoint *local, struct ip_endpoint *foreign)
{
    struct tcp_hdr *hdr;
    struct pseudo_hdr pseudo;
    uint16_t psum;
    uint16_t total;
    char ep1[IP_ENDPOINT_STR_LEN];
    char ep2[IP_ENDPOINT_STR_LEN];
    uint8_t *buf;
    uint8_t sbuf[2048];
    uint8_t opts[8];
    size_t optlen = 0;

    /*
     * RFC 1122 4.2.2.6: a peer that receives no MSS option in our SYN /
     * SYN-ACK must assume the 536-byte default for the data it sends us.
     * This header used to carry no options at all, so every inbound
     * (upload) stream ran at 536-byte segments - about 1/3 of the payload
     * per frame of the outbound direction - while the per-frame
     * SDIO/IPC pipeline cost is identical. Advertise our real MSS so the
     * peer can fill full-size segments.
     */
    if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
        struct ip_iface *iface = ip_route_get_iface(local->addr);
        uint16_t mss = 536;

        if (iface) {
            mss = NET_IFACE(iface)->dev->mtu - (IP_HDR_SIZE_MIN + sizeof(struct tcp_hdr));
        }
        opts[0] = TCP_OPT_MSS;
        opts[1] = TCP_OPT_MSS_LEN;
        opts[2] = (uint8_t)(mss >> 8);
        opts[3] = (uint8_t)(mss & 0xff);
        /*
         * Advertise WSOPT with our real receive-window scale. This lets the
         * peer left-shift the window we advertise on later segments, so our
         * receive window can exceed the 16-bit field and cover the ~100ms
         * IPC RTT that otherwise pins downloads near 300KB/s. The peer only
         * applies the scale if it too sent WSOPT; tcp_wire_window() mirrors
         * that by shifting only when wsopt_ok and never on the SYN itself.
         */
        opts[4] = TCP_OPT_NOP;
        opts[5] = TCP_OPT_WS;
        opts[6] = TCP_OPT_WS_LEN;
        opts[7] = TCP_RCV_WSCALE;
        optlen = sizeof(opts);
    }

    total = sizeof(*hdr) + optlen + len;
    /*
     * Fast path: any MTU-sized ethernet segment (and every bare ACK) fits on
     * the stack, so skip the heap alloc/free pair that used to run once per
     * outgoing segment. Only oversized loopback segments (mtu=64KB) still
     * fall back to the heap.
     */
    if (total <= sizeof(sbuf)) {
        buf = sbuf;
    } else {
        buf = memory_alloc(total);
        if(!buf)
            return -1;
    }

    hdr = (struct tcp_hdr *)buf;
    hdr->src = local->port;
    hdr->dst = foreign->port;
    hdr->seq = hton32(seq);
    hdr->ack = hton32(ack);
    hdr->off = (uint8_t)(((sizeof(*hdr) + optlen) >> 2) << 4);
    hdr->flg = flg;
    hdr->wnd = hton16(wnd);
    hdr->sum = 0;
    hdr->up = 0;
    if (optlen) {
        memcpy(hdr + 1, opts, optlen);
    }
    memcpy((uint8_t *)(hdr + 1) + optlen, data, len);
    pseudo.src = local->addr;
    pseudo.dst = foreign->addr;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTOCOL_TCP;
    pseudo.len = hton16(total);
    psum = ~cksum16((uint16_t *)&pseudo, sizeof(pseudo), 0);
    hdr->sum = cksum16((uint16_t *)hdr, total, psum);

    int ret = ip_output(IP_PROTOCOL_TCP, (uint8_t *)hdr, total, local->addr, foreign->addr);
    if (buf != sbuf)
        free(buf);
    if(ret < 0){
        /*
         * A negative return here during bulk transfer is almost always
         * transient wl0 TX-credit backpressure. The genuine hard failures
         * already log their own [E] at the origin (net_device_output for a
         * dead link, arp_* for unresolved neighbours), and this segment is
         * retransmitted regardless, so keep the per-segment noise off the
         * error channel to avoid an [E] storm during upload.
         */
        debugf("tcp_output_segment send deferred: flg=0x%x seq=%u ack=%u local=%s foreign=%s",
            flg, seq, ack,
            ip_endpoint_ntop(local, ep1, sizeof(ep1)),
            ip_endpoint_ntop(foreign, ep2, sizeof(ep2)));
    }
    return ret;
}

static ssize_t
tcp_output(struct tcp_pcb *pcb, uint8_t flg, uint8_t *data, size_t len)
{
    uint32_t seq;

    seq = pcb->snd.nxt;
    if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
        seq = pcb->iss;
    }
    if (TCP_FLG_ISSET(flg, TCP_FLG_SYN | TCP_FLG_FIN) || len) {
        tcp_retransmit_queue_add(pcb, seq, flg, data, len);
    }
    return tcp_output_segment(seq, pcb->rcv.nxt, flg, tcp_wire_window(pcb, flg), data, len, &pcb->local, &pcb->foreign);
}

static void
tcp_send_ack_only(struct tcp_pcb *pcb)
{
    struct tcp_hdr hdr;
    struct pseudo_hdr pseudo;
    uint16_t psum;
    uint16_t total;

    hdr.src = pcb->local.port;
    hdr.dst = pcb->foreign.port;
    hdr.seq = hton32(pcb->snd.nxt);
    hdr.ack = hton32(pcb->rcv.nxt);
    hdr.off = (sizeof(hdr) >> 2) << 4;
    hdr.flg = TCP_FLG_ACK;
    hdr.wnd = hton16(tcp_wire_window(pcb, TCP_FLG_ACK));
    hdr.sum = 0;
    hdr.up = 0;
    pseudo.src = pcb->local.addr;
    pseudo.dst = pcb->foreign.addr;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTOCOL_TCP;
    total = sizeof(hdr);
    pseudo.len = hton16(total);
    psum = ~cksum16((uint16_t *)&pseudo, sizeof(pseudo), 0);
    hdr.sum = cksum16((uint16_t *)&hdr, total, psum);

    ip_output(IP_PROTOCOL_TCP, (uint8_t *)&hdr, total, pcb->local.addr, pcb->foreign.addr);
}

/* rfc793 - section 3.9 [Event Processing > SEGMENT ARRIVES] */
static void
tcp_segment_arrives(struct tcp_segment_info *seg, uint8_t flags, uint8_t *data, size_t len, struct ip_endpoint *local, struct ip_endpoint *foreign)
{
    struct tcp_pcb *pcb, *new_pcb;
    int acceptable = 0;
    char ep1[IP_ENDPOINT_STR_LEN];
    char ep2[IP_ENDPOINT_STR_LEN];
    pcb = tcp_pcb_select(local, foreign);
    if (!pcb || pcb->state == TCP_PCB_STATE_CLOSED) {
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            return;
        }
        if (!TCP_FLG_ISSET(flags, TCP_FLG_ACK)) {
            tcp_output_segment(0, seg->seq + seg->len, TCP_FLG_RST | TCP_FLG_ACK, 0, NULL, 0, local, foreign);
        } else {
            tcp_output_segment(seg->ack, 0, TCP_FLG_RST, 0, NULL, 0, local, foreign);
        }
        return;
    }
    switch(pcb->state) {
    case TCP_PCB_STATE_LISTEN:
        /*
         * first check for an RST
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            return;
        }
        /*
         * second check for an ACK
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_ACK)) {
            tcp_output_segment(seg->ack, 0, TCP_FLG_RST, 0, NULL, 0, local, foreign);
            return;
        }
        /*
         * third check for an SYN
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_SYN)) {
            /* ignore: security/compartment check */
            /* ignore: precedence check */
            if (pcb->mode == TCP_PCB_MODE_SOCKET) {
                new_pcb = tcp_pcb_alloc();
                if (!new_pcb) {
                    errorf("tcp_pcb_alloc() failure");
                    return;
                }
                new_pcb->mode = TCP_PCB_MODE_SOCKET;
                new_pcb->parent = pcb;
                pcb = new_pcb;
            }
            pcb->local = *local;
            pcb->foreign = *foreign;
            pcb->rcv.wnd = sizeof(pcb->buf);
            pcb->rcv.nxt = seg->seq + 1;
            pcb->irs = seg->seq;
            pcb->iss = random();
            pcb->peer_wscale = seg->has_wscale ? seg->wscale : 0;
            pcb->wsopt_ok = seg->has_wscale ? 1 : 0;
            tcp_output(pcb, TCP_FLG_SYN | TCP_FLG_ACK, NULL, 0);
            pcb->snd.nxt = pcb->iss + 1;
            pcb->snd.una = pcb->iss;
            pcb->state = TCP_PCB_STATE_SYN_RECEIVED;
            /* ignore: Note that any other incoming control or data (combined with SYN) will be processed
                        in the SYN-RECEIVED state, but processing of SYN and ACK  should not be repeated */
            return;
        }
        /*
         * fourth other text or control
         */
        /* drop segment */
        return;
    case TCP_PCB_STATE_SYN_SENT:
        /*
         * first check the ACK bit
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_ACK)) {
            if (seg->ack <= pcb->iss || seg->ack > pcb->snd.nxt) {
                tcp_output_segment(seg->ack, 0, TCP_FLG_RST, 0, NULL, 0, local, foreign);
                return;
            }
            if (pcb->snd.una <= seg->ack && seg->ack <= pcb->snd.nxt) {
                acceptable = 1;
            }
        }
        /*
         * second check the RST bit
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            if (acceptable) {
                errorf("connection reset");
                pcb->state = TCP_PCB_STATE_CLOSED;
                pcb->close_reason = 1; /* RST */
                tcp_log_close_event("tcp_syn_sent rst close", pcb);
                tcp_pcb_release(pcb);
            }
            /* drop segment */
            return;
        }
        /*
         * ignore: third check security and precedence
         */
        /*
         * fourth check the SYN bit
         */
        if (TCP_FLG_ISSET(flags, TCP_FLG_SYN)) {
            pcb->rcv.nxt = seg->seq + 1;
            pcb->irs = seg->seq;
            if (acceptable) {
                pcb->snd.una = seg->ack;
                tcp_retransmit_queue_cleanup(pcb);
            }
            if (pcb->snd.una > pcb->iss) {
                pcb->state = TCP_PCB_STATE_ESTABLISHED;
                tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
                /* NOTE: not specified in the RFC793, but send window initialization required */
                pcb->peer_wscale = seg->has_wscale ? seg->wscale : 0;
                pcb->wsopt_ok = seg->has_wscale ? 1 : 0;
                pcb->snd.wnd = tcp_peer_window(pcb, seg->wnd);
                pcb->snd.wl1 = seg->seq;
                pcb->snd.wl2 = seg->ack;
                tcp_sched_wakeup_all(pcb);
                task_wakeup_tcp_writers(indexof(pcbs, pcb));
                /* ignore: continue processing at the sixth step below where the URG bit is checked */
                return;
            } else {
                pcb->state = TCP_PCB_STATE_SYN_RECEIVED;
                tcp_output(pcb, TCP_FLG_SYN | TCP_FLG_ACK, NULL, 0);
                /* ignore: If there are other controls or text in the segment, queue them for processing after the ESTABLISHED state has been reached */
                return;
            }
        }
        /*
         * fifth, if neither of the SYN or RST bits is set then drop the segment and return
         */
        /* drop segment */
        return;
    }
    /*
     * Otherwise
     */
    /*
     * first check sequence number
     */
    switch (pcb->state) {
    case TCP_PCB_STATE_SYN_RECEIVED:
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
    case TCP_PCB_STATE_CLOSE_WAIT:
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        if (!seg->len) {
            if (!pcb->rcv.wnd) {
                if (seg->seq == pcb->rcv.nxt) {
                    acceptable = 1;
                }
            } else {
                if (pcb->rcv.nxt <= seg->seq && seg->seq < pcb->rcv.nxt + pcb->rcv.wnd) {
                    acceptable = 1;
                }
            }
        } else {
            if (!pcb->rcv.wnd) {
                /* not acceptable */
            } else {
                if ((pcb->rcv.nxt <= seg->seq && seg->seq < pcb->rcv.nxt + pcb->rcv.wnd) ||
                    (pcb->rcv.nxt <= seg->seq + seg->len - 1 && seg->seq + seg->len - 1 < pcb->rcv.nxt + pcb->rcv.wnd)) {
                    acceptable = 1;
                }
            }
        }
        if (!acceptable) {
            if (!TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
                tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
            }
            return;
        }
        /*
         * In the following it is assumed that the segment is the idealized
         * segment that begins at RCV.NXT and does not exceed the window.
         * One could tailor actual segments to fit this assumption by
         * trimming off any portions that lie outside the window (including
         * SYN and FIN), and only processing further if the segment then
         * begins at RCV.NXT.  Segments with higher begining sequence
         * numbers may be held for later processing.
         */
    }
    /*
     * second check the RST bit
     */
    switch (pcb->state) {
    case TCP_PCB_STATE_SYN_RECEIVED:
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 1; /* RST */
            tcp_log_close_event("tcp_segment rst close", pcb);
            tcp_pcb_release(pcb);
            return;
        }
        break;
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
    case TCP_PCB_STATE_CLOSE_WAIT:
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 1; /* RST */
            tcp_log_close_event("tcp_segment rst close", pcb);
            tcp_pcb_release(pcb);
            return;
        }
        break;
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        if (TCP_FLG_ISSET(flags, TCP_FLG_RST)) {
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 1; /* RST */
            tcp_log_close_event("tcp_segment rst close", pcb);
            tcp_pcb_release(pcb);
            return;
        }
        break;
    }
    /*
     * ignore: third check security and precedence
     */
    /*
     * fourth check the SYN bit
     */
    switch (pcb->state) {
    case TCP_PCB_STATE_SYN_RECEIVED:
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
    case TCP_PCB_STATE_CLOSE_WAIT:
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        if (TCP_FLG_ISSET(flags, TCP_FLG_SYN)) {
            tcp_output(pcb, TCP_FLG_RST, NULL, 0);
            errorf("connection reset (unexpected SYN in closing state)");
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 1; /* RST */
            tcp_log_close_event("tcp_segment unexpected syn close", pcb);
            tcp_pcb_release(pcb);
            return;
        }
    }
    /*
     * fifth check the ACK field
     */
    if (!TCP_FLG_ISSET(flags, TCP_FLG_ACK)) {
        /* drop segment */
        return;
    }
    switch (pcb->state) {
    case TCP_PCB_STATE_SYN_RECEIVED:
        if (pcb->snd.una <= seg->ack && seg->ack <= pcb->snd.nxt) {
            pcb->state = TCP_PCB_STATE_ESTABLISHED;
            /*
             * Passive open must initialize the peer send window exactly like
             * active connect does. Otherwise accepted sockets inherit a zero
             * snd.wnd and tcp_writable() reports permanently non-writable, so
             * server-first protocols (sshd banner, telnet negotiation) stall
             * forever right after accept().
             *
             * peer_wscale/wsopt_ok were already captured from the peer's SYN
             * (LISTEN handler); do NOT overwrite them from this segment. This
             * is the handshake-completing ACK, and RFC 7323 makes WSOPT valid
             * only in SYN segments, so seg->has_wscale is always 0 here and
             * overwriting dropped the negotiated scale - the peer's window
             * then collapsed to its raw 16-bit value (e.g. a 256KB window
             * shifted by 6 read as 4096) and capped bulk upload at
             * window/RTT (~4KB per RTT on wifi).
             */
            pcb->snd.wnd = tcp_peer_window(pcb, seg->wnd);
            pcb->snd.wl1 = seg->seq;
            pcb->snd.wl2 = seg->ack;
            debugf("passive established desc=%d wnd=%u una=%u nxt=%u ack=%u seq=%u q=%u",
                   indexof(pcbs, pcb), pcb->snd.wnd, pcb->snd.una, pcb->snd.nxt,
                   seg->ack, seg->seq, pcb->queue.num);
            tcp_sched_wakeup_all(pcb);
            task_wakeup_tcp_writers(indexof(pcbs, pcb));
            if (pcb->parent) {
                queue_push(&pcb->parent->backlog, pcb);
                tcp_sched_wakeup_all(pcb->parent);
                /*
                 * tcp_sched_wakeup_all() only wakes a worker blocked inside a
                 * synchronous accept(). A client polling the listening socket
                 * via poll(POLLIN) is parked on the listen node's VFS wait
                 * queue instead, so raise a VFS RD edge on the parent socket
                 * to release single-process acceptors like telnetd.
                 */
                task_wakeup_tcp_readers(indexof(pcbs, pcb->parent));
            }
        } else {
            tcp_output_segment(seg->ack, 0, TCP_FLG_RST, 0, NULL, 0, local, foreign);
            return;
        }
        /* fall through */
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
    case TCP_PCB_STATE_CLOSE_WAIT:
    case TCP_PCB_STATE_CLOSING:
        if (pcb->snd.una <= seg->ack && seg->ack <= pcb->snd.nxt) {
            int ack_advanced = pcb->snd.una < seg->ack;
            uint32_t acked_bytes = seg->ack - pcb->snd.una;
            /*
             * Snapshot writability (same formula as tcp_writable(): window
             * capacity AND retransmit-queue room) before this ACK mutates
             * snd.una/snd.wnd/queue, so the wakeup below can fire on the
             * blocked->writable EDGE only.
             */
            int was_writable = ((pcb->snd.nxt - pcb->snd.una) < pcb->snd.wnd) &&
                               (pcb->queue.num < TCP_RETRANSMIT_QUEUE_MAX);

            pcb->snd.una = seg->ack;
            if (ack_advanced) {
                tcp_retransmit_queue_cleanup(pcb);
                pcb->dupacks = 0;
            } else if (len == 0 && pcb->queue.num > 0 &&
                       tcp_peer_window(pcb, seg->wnd) == pcb->snd.wnd &&
                       !TCP_FLG_ISSET(flags, TCP_FLG_SYN | TCP_FLG_FIN)) {
                /*
                 * RFC 5681 fast retransmit. Without it, every frame lost on a
                 * lossy link (raspix WLAN) stalls the whole in-flight window
                 * for a full RTO (200ms+, doubling), capping throughput at
                 * roughly window/RTO regardless of link speed. Three pure dup
                 * ACKs (no payload, no window change, unACKed data queued)
                 * mean the receiver got later segments, so resend the oldest
                 * unACKed segment immediately -- once per loss episode; the
                 * RTO timer still backstops multi-loss windows. Refresh
                 * entry->last without doubling rto so the timer path doesn't
                 * fire a duplicate retransmit right behind this one.
                 *
                 * Compare the SCALED peer window with snd.wnd: a raw-vs-
                 * scaled compare never matches once window scaling is in
                 * effect, which silently disabled fast retransmit.
                 */
                if (pcb->dupacks < 0xff) {
                    pcb->dupacks++;
                }
                if (pcb->dupacks == 3) {
                    struct tcp_queue_entry *re = queue_peek(&pcb->queue);
                    if (re) {
                        tcp_output_segment(re->seq, pcb->rcv.nxt, re->flg, tcp_wire_window(pcb, re->flg),
                                           (uint8_t *)(re + 1), re->len, &pcb->local, &pcb->foreign);
                        gettimeofday(&re->last, NULL);
                    }
                }
            }
            /* ignore: Users should receive positive acknowledgments for buffers
                        which have been SENT and fully acknowledged (i.e., SEND buffer should be returned with "ok" response) */
            if (pcb->snd.wl1 < seg->seq || (pcb->snd.wl1 == seg->seq && pcb->snd.wl2 <= seg->ack)) {
                uint32_t old_wnd = pcb->snd.wnd;
                pcb->snd.wnd = tcp_peer_window(pcb, seg->wnd);
                pcb->snd.wl1 = seg->seq;
                pcb->snd.wl2 = seg->ack;
                (void)old_wnd;
            }
            tcp_sched_wakeup_all(pcb);
            /*
             * The send window may have just opened (this ACK freed inflight
             * bytes or the peer advertised a larger window). Since tcp_send()
             * no longer blocks the netd worker, raise VFS_EVT_WR so a client
             * that returned EAGAIN and blocked on write can retry its send.
             * Mirrors task_wakeup_tcp_readers() on the RECV path.
             *
             * Writability must include the retransmit-queue-room condition
             * (tcp_send()/tcp_writable() gate on it too): waking while the
             * queue is still full just makes the client retry into another
             * EAGAIN, and the disarm+rearm below would keep resetting the
             * persist backoff to its minimum -- a perpetual probe/wake/retry
             * churn. Fire the VFS wakeup only on the blocked->writable edge;
             * an ACK stream with the window already open must not emit one
             * reverse IPC per segment (wakeup storm on the shared /dev/net0
             * node). Level-triggered rechecks (check_poll_events and the
             * worker's post-EAGAIN sock_writable() recheck) cover the races.
             */
            {
                uint32_t inflight_now = pcb->snd.nxt - pcb->snd.una;
                int now_writable = (inflight_now < pcb->snd.wnd) &&
                                   (pcb->queue.num < TCP_RETRANSMIT_QUEUE_MAX);
                if (now_writable) {
                    tcp_persist_disarm(pcb);
                    if (!was_writable) {
                        task_wakeup_tcp_writers(indexof(pcbs, pcb));
                    }
                }
            }
        } else if (seg->ack < pcb->snd.una) {
            /* ignore */
        } else if (seg->ack > pcb->snd.nxt) {
            tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
            return;
        }
        switch (pcb->state) {
        case TCP_PCB_STATE_FIN_WAIT1:
            if (seg->ack == pcb->snd.nxt) {
                pcb->state = TCP_PCB_STATE_FIN_WAIT2;
            }
            break;
        case TCP_PCB_STATE_FIN_WAIT2:
            /* do not delete the TCB */
            break;
        case TCP_PCB_STATE_CLOSE_WAIT:
            /* do nothing */
            break;
        case TCP_PCB_STATE_CLOSING:
            if (seg->ack == pcb->snd.nxt) {
                pcb->state = TCP_PCB_STATE_TIME_WAIT;
                /* NOTE: set 2MSL timer, although it is not explicitly stated in the RFC */
                tcp_set_timewait_timer(pcb);
                tcp_sched_wakeup_all(pcb);
            }
            break;
        }
        break;
    case TCP_PCB_STATE_LAST_ACK:
        if (seg->ack == pcb->snd.nxt) {
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 0; /* normal close */
            tcp_pcb_release(pcb);
        }
        return;
    case TCP_PCB_STATE_TIME_WAIT:
        if (TCP_FLG_ISSET(flags, TCP_FLG_FIN)) {
            tcp_set_timewait_timer(pcb); /* restart time-wait timer */
        }
        break;
    }
    /*
     * ignore: sixth, check the URG bit
     */
    /*
     * seventh, process the segment text
     */
    switch (pcb->state) {
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
        if (len) {
            size_t copy_off = 0;
            size_t copy_len = len;

            /*
             * The checks above only proved the segment overlaps the receive
             * window somewhere. They do NOT guarantee that payload begins at
             * RCV.NXT or that the whole payload fits in the remaining window.
             *
             * Blindly memcpy()ing the full payload corrupts the receive buffer
             * and window bookkeeping on retransmits/overlaps, which later
             * poisons queued send metadata and can crash tcp_output_segment()
             * with a huge bogus len. Only consume the in-order, in-window tail
             * that actually advances RCV.NXT.
             */
            if (seg->seq < pcb->rcv.nxt) {
                uint32_t already_recv = pcb->rcv.nxt - seg->seq;
                if ((size_t)already_recv >= copy_len) {
                    copy_len = 0;
                } else {
                    copy_off = (size_t)already_recv;
                    copy_len -= copy_off;
                }
            } else if (seg->seq > pcb->rcv.nxt) {
                /*
                 * Out-of-order segment. Dropping it turns every lost WLAN
                 * frame into go-back-N: the whole in-flight window behind the
                 * hole is discarded and (without SACK) the peer re-sends one
                 * MSS per RTT under NewReno -- uploads run at half the
                 * download rate. Instead stash the payload directly at its
                 * natural position in pcb->buf (the seq->offset mapping is
                 * stable: rcv.nxt and rcv.wnd always move together) and track
                 * ONE contiguous range -- a single lost frame, the dominant
                 * loss pattern, leaves exactly one hole. When the retransmit
                 * fills the hole, rcv.nxt jumps over the stash in one step.
                 * A rarer second hole just falls back to the old drop path.
                 */
                copy_len = 0;
                uint32_t ooo_off = seg->seq - pcb->rcv.nxt;
                if (len > 0 && ooo_off < pcb->rcv.wnd) {
                    size_t keep = len;
                    if (ooo_off + keep > pcb->rcv.wnd) {
                        keep = pcb->rcv.wnd - ooo_off;
                    }
                    uint32_t s = seg->seq;
                    uint32_t e = seg->seq + (uint32_t)keep;
                    int stash = 0;
                    if (!pcb->ooo_len) {
                        pcb->ooo_seq = s;
                        pcb->ooo_len = e - s;
                        stash = 1;
                    } else {
                        uint32_t os = pcb->ooo_seq;
                        uint32_t oe = pcb->ooo_seq + pcb->ooo_len;
                        /* overlapping or adjacent (serial arithmetic) */
                        if ((int32_t)(s - oe) <= 0 && (int32_t)(e - os) >= 0) {
                            if ((int32_t)(s - os) < 0) {
                                os = s;
                            }
                            if ((int32_t)(e - oe) > 0) {
                                oe = e;
                            }
                            pcb->ooo_seq = os;
                            pcb->ooo_len = oe - os;
                            stash = 1;
                        }
                    }
                    if (stash) {
                        memcpy(pcb->buf + (sizeof(pcb->buf) - pcb->rcv.wnd) + ooo_off,
                               data, keep);
                    }
                }
            }

            if (copy_len > pcb->rcv.wnd) {
                copy_len = pcb->rcv.wnd;
            }

            if (copy_len > 0) {
                memcpy(pcb->buf + (sizeof(pcb->buf) - pcb->rcv.wnd), data + copy_off, copy_len);
                pcb->rcv.nxt += copy_len;
                pcb->rcv.wnd -= copy_len;
                /*
                 * If this segment filled the hole in front of the stashed
                 * out-of-order range, the stash is already sitting at the
                 * right buffer offset -- consuming it is pure bookkeeping.
                 */
                int ooo_merged = 0;
                if (pcb->ooo_len) {
                    uint32_t ooo_end = pcb->ooo_seq + pcb->ooo_len;
                    if ((int32_t)(ooo_end - pcb->rcv.nxt) <= 0) {
                        pcb->ooo_len = 0; /* fully covered by in-order data */
                    } else if ((int32_t)(pcb->ooo_seq - pcb->rcv.nxt) <= 0) {
                        uint32_t delta = ooo_end - pcb->rcv.nxt;
                        pcb->rcv.nxt += delta;
                        pcb->rcv.wnd -= delta;
                        pcb->ooo_len = 0;
                        ooo_merged = 1;
                    }
                }
                /*
                 * RFC 1122 delayed ACK: acknowledge every TCP_DELACK_SEGS-th
                 * in-order segment, or after TCP_DELACK_TIMEOUT_USEC. On the
                 * raspix WLAN link every pure ACK costs a full SDIO TX frame
                 * (~0.5ms of bus time plus one TX credit), so immediate ACKs
                 * burn ~1/3 of the bus during downloads.
                 *
                 * A hole-filling merge ends a loss-recovery episode: ACK the
                 * big rcv.nxt jump immediately so the peer exits recovery.
                 */
                pcb->delack_count++;
                if (ooo_merged || pcb->delack_count >= TCP_DELACK_SEGS) {
                    pcb->delack_pending = 0;
                    pcb->delack_count = 0;
                    tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
                } else if (!pcb->delack_pending) {
                    pcb->delack_pending = 1;
                    gettimeofday(&pcb->delack_timer, NULL);
                }
            } else {
                /*
                 * Dup/out-of-window segment: RFC 1122 requires an immediate
                 * ACK so the sender can fast-retransmit; also the correct
                 * zero-window probe response.
                 */
                pcb->delack_pending = 0;
                pcb->delack_count = 0;
                tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
            }
            tcp_sched_wakeup_all(pcb);
            task_wakeup_tcp_readers(indexof(pcbs, pcb));
        }
        break;
    case TCP_PCB_STATE_CLOSE_WAIT:
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        /* ignore segment text */
        break;
    }

    /*
     * eighth, check the FIN bit
     */
    if (TCP_FLG_ISSET(flags, TCP_FLG_FIN)) {
        switch (pcb->state) {
        case TCP_PCB_STATE_CLOSED:
        case TCP_PCB_STATE_LISTEN:
        case TCP_PCB_STATE_SYN_SENT:
            /* drop segment */
            return;
        }
        /*
         * The FIN occupies the sequence number right after this segment's
         * payload. Consume it ONLY when it is the exact next expected byte
         * (RCV.NXT). If a hole still precedes it -- a reordered FIN, or a
         * data+FIN whose payload the seventh step could not buffer in-order --
         * then setting RCV.NXT = seg->seq + 1 here would leap over the missing
         * bytes AND (below) clear the out-of-order stash, so the peer's real
         * in-order response segments that arrive right behind the FIN are then
         * REJECTed as "behind RCV.NXT". That is the "HTTP response body lost
         * -> xBrowser empty body" failure. Defer instead: the dup-ACK from the
         * seventh step plus the one below keeps the peer retransmitting until
         * the hole fills, after which the FIN is redelivered in-order.
         */
        uint32_t fin_seq = seg->seq + (uint32_t)len;
        if (fin_seq != pcb->rcv.nxt) {
            tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
            return;
        }
        pcb->rcv.nxt = fin_seq + 1;
        /*
         * Any out-of-order stash is dead now (no more data follows the FIN)
         * and would poison the tcp_receive() memmove-span math.
         */
        pcb->ooo_len = 0;
        /* FIN gets an immediate ACK; drop any pending delayed ACK. */
        pcb->delack_pending = 0;
        pcb->delack_count = 0;
        tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
        switch (pcb->state) {
        case TCP_PCB_STATE_SYN_RECEIVED:
        case TCP_PCB_STATE_ESTABLISHED:
            pcb->state = TCP_PCB_STATE_CLOSE_WAIT;
            tcp_sched_wakeup_all(pcb);
            break;
        case TCP_PCB_STATE_FIN_WAIT1:
            if (seg->ack == pcb->snd.nxt) {
                pcb->state = TCP_PCB_STATE_TIME_WAIT;
                tcp_set_timewait_timer(pcb);
            } else {
                pcb->state = TCP_PCB_STATE_CLOSING;
            }
            break;
        case TCP_PCB_STATE_FIN_WAIT2:
            pcb->state = TCP_PCB_STATE_TIME_WAIT;
            tcp_set_timewait_timer(pcb);
            break;
        case TCP_PCB_STATE_CLOSE_WAIT:
            /* Remain in the CLOSE-WAIT state */
            break;
        case TCP_PCB_STATE_CLOSING:
            /* Remain in the CLOSING state */
            break;
        case TCP_PCB_STATE_LAST_ACK:
            /* Remain in the LAST-ACK state */
            break;
        case TCP_PCB_STATE_TIME_WAIT:
            /* Remain in the TIME-WAIT state */
            tcp_set_timewait_timer(pcb); /* restart time-wait timer */
            break;
        }
        /*
         * A pure FIN carries no segment text, so the len>0 branch above did
         * not wake VFS readers. A client blocked in poll(POLLIN) with an idle
         * read task needs an explicit RD edge to observe EOF; tcp_receive()
         * then returns 0 once the receive buffer drains.
         */
        task_wakeup_tcp_readers(indexof(pcbs, pcb));
    }
    return;
}

static void
tcp_input(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst, struct ip_iface *iface)
{
    struct tcp_hdr *hdr;
    struct pseudo_hdr pseudo;
    uint16_t psum, hlen;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[IP_ADDR_STR_LEN];
    struct ip_endpoint local, foreign;
    struct tcp_segment_info seg;

    if (len < sizeof(*hdr)) {
        errorf("too short");
        return;
    }
    hdr = (struct tcp_hdr *)data;
    pseudo.src = src;
    pseudo.dst = dst;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTOCOL_TCP;
    pseudo.len = hton16(len);
    psum = ~cksum16((uint16_t *)&pseudo, sizeof(pseudo), 0);
    if (cksum16((uint16_t *)hdr, len, psum) != 0) {
        return;
    }
    if (src == IP_ADDR_BROADCAST || src == iface->broadcast || dst == IP_ADDR_BROADCAST || dst == iface->broadcast) {
        return;
    }
    hlen = (hdr->off >> 4) << 2;
    if (hlen < sizeof(*hdr) || hlen > len) {
        errorf("header length error: hlen=%u, len=%zu", hlen, len);
        return;
    }
    local.addr = dst;
    local.port = hdr->dst;
    foreign.addr = src;
    foreign.port = hdr->src;
    seg.seq = ntoh32(hdr->seq);
    seg.ack = ntoh32(hdr->ack);
    seg.len = len - hlen;
    if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_SYN)) {
        seg.len++; /* SYN flag consumes one sequence number */
    }
    if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_FIN)) {
        seg.len++; /* FIN flag consumes one sequence number */
    }
    seg.wnd = ntoh16(hdr->wnd);
    seg.up = ntoh16(hdr->up);
    seg.wscale = 0;
    seg.has_wscale = 0;
    if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_SYN)) {
        tcp_parse_syn_options(hdr, hlen, &seg.wscale, &seg.has_wscale);
    }
    mutex_lock(&mutex);
    tcp_segment_arrives(&seg, hdr->flg, (uint8_t *)hdr + hlen, len - hlen, &local, &foreign);
    mutex_unlock(&mutex);
    return;
}

static void
tcp_timer(void)
{
    struct tcp_pcb *pcb;
    struct timeval now;

    gettimeofday(&now, NULL);
    mutex_lock(&mutex);
    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if (pcb->state == TCP_PCB_STATE_FREE) {
            continue;
        }
        if (pcb->state == TCP_PCB_STATE_TIME_WAIT) {
            if (timercmp(&now, &pcb->tw_timer, >) != 0) {
                tcp_pcb_release(pcb);
                continue;
            }
        }
        /*
         * Orphan reaper. A socket-mode pcb whose owning sock is gone can
         * never see another tcp_close(): sock_free() already ran, so if the
         * pcb parks in CLOSED (RST/retransmit-deadline, close_reason != 0)
         * or in a FIN-wait state whose final peer segment never arrives, the
         * slot leaks forever and after <= TCP_PCB_SIZE leaks netd refuses
         * every new connection ("no free PCB"). close_reason == 0 CLOSED is
         * deliberately spared: a fresh tcp_open() pcb sits in that state for
         * a moment before sock_open() registers the desc->sock mapping.
         * sock_id_from_tcp_desc() takes only the leaf socks_lock, the same
         * order the task_wakeup_* paths already use under this mutex.
         */
        if (pcb->mode == TCP_PCB_MODE_SOCKET &&
            sock_id_from_tcp_desc(indexof(pcbs, pcb)) < 0) {
            if (pcb->state == TCP_PCB_STATE_CLOSED && pcb->close_reason != 0) {
                tcp_log_close_event("tcp_timer orphan reap", pcb);
                tcp_pcb_release(pcb);
                continue;
            }
            if ((pcb->state == TCP_PCB_STATE_FIN_WAIT1 ||
                 pcb->state == TCP_PCB_STATE_FIN_WAIT2 ||
                 pcb->state == TCP_PCB_STATE_CLOSING ||
                 pcb->state == TCP_PCB_STATE_LAST_ACK) &&
                pcb->queue.num == 0) {
                /* Our FIN is ACKed but the peer's closing segment never
                 * came. Nobody waits on this pcb; borrow the TIME_WAIT
                 * timer as a grace period, then reclaim the slot. */
                if (!timerisset(&pcb->tw_timer)) {
                    tcp_set_timewait_timer(pcb);
                } else if (timercmp(&now, &pcb->tw_timer, >) != 0) {
                    tcp_log_close_event("tcp_timer orphan fin-wait reap", pcb);
                    tcp_pcb_release(pcb);
                    continue;
                }
            }
        }
        /*
         * Only the oldest unacknowledged segment gates forward progress:
         * cumulative ACK cannot pass a hole before that head segment is
         * recovered, and retransmitting younger queued data first does not
         * reopen the send path. Walking the full retransmit queue every timer
         * tick under the global mutex adds avoidable CPU/lock hold time during
         * bulk WiFi transfers, so drive the timer from the queue head only.
         */
        if (pcb->queue.num > 0) {
            struct tcp_queue_entry *entry = queue_peek(&pcb->queue);

            debugf("tcp_timer: processing queue for pcb state=%d, queue.num=%u", pcb->state, pcb->queue.num);
            if (entry) {
                tcp_retransmit_queue_emit(pcb, entry);
            }
        }
        /*
         * Persist timer (zero-window probe). When the send window is closed and
         * there is nothing in the retransmit queue, periodically send a probe to
         * elicit a window update from the receiver. Without this, a lost window
         * update causes the sender to hang forever (sshd long-output hang).
         */
        if (pcb->persist_probing &&
            (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
             pcb->state == TCP_PCB_STATE_CLOSE_WAIT)) {
            if (timercmp(&now, &pcb->persist_timer, >) != 0) {
                /* Send a zero-window probe: a keep-alive style segment with
                 * seq = snd.una - 1. The receiver sees this as out-of-window
                 * and responds with an ACK containing its current window. */
                tcp_output_segment(pcb->snd.una - 1, pcb->rcv.nxt,
                    TCP_FLG_ACK, tcp_wire_window(pcb, TCP_FLG_ACK), NULL, 0,
                    &pcb->local, &pcb->foreign);
                /* Exponential backoff */
                pcb->persist_rto *= 2;
                if (pcb->persist_rto > TCP_PERSIST_RTO_MAX)
                    pcb->persist_rto = TCP_PERSIST_RTO_MAX;
                pcb->persist_timer = now;
                timeval_add_usec(&pcb->persist_timer, pcb->persist_rto);
                /* Also fire a write wakeup: the window may have already
                 * reopened (missed edge) and the probe response will confirm.
                 * This recovers connections stuck before the probe round-trip. */
                task_wakeup_tcp_writers(indexof(pcbs, pcb));
            }
        }
        /* Delayed-ACK timer: flush a pending ACK after the 40ms ceiling. */
        if (pcb->delack_pending &&
            (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
             pcb->state == TCP_PCB_STATE_CLOSE_WAIT)) {
            struct timeval deadline = pcb->delack_timer;
            timeval_add_usec(&deadline, TCP_DELACK_TIMEOUT_USEC);
            if (timercmp(&now, &deadline, >) != 0) {
                pcb->delack_pending = 0;
                pcb->delack_count = 0;
                tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
            }
        }
    }
    debugf("tcp_timer: finished");
    mutex_unlock(&mutex);
}

static int
event_handler(void *arg)
{
    struct tcp_pcb *pcb;
    int handled = 0;

    (void)arg;

    mutex_lock(&mutex);
    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if (pcb->state != TCP_PCB_STATE_FREE &&
                (pcb->state_ctx.sleeping || pcb->send_ctx.sleeping || pcb->recv_ctx.sleeping)) {
            tcp_sched_interrupt_all(pcb);
            handled++;
        }
    }
    mutex_unlock(&mutex);
    return handled;
}

int
tcp_init(void)
{
    /*
     * A 1ms global TCP timer forces netd to rescan every PCB at kHz rate even
     * when the nearest real deadline is tens of milliseconds away (delayed ACK
     * 40ms, default RTO 200ms, persist/time-wait much larger). Under
     * telnetd/sshd load this keeps both netd worker threads in RUN and starves
     * IPC responsiveness. 10ms is still comfortably below the smallest real TCP
     * deadline we use and removes the synthetic spin.
     */
    struct timeval interval = {0,10000};

    if (ip_protocol_register("TCP", IP_PROTOCOL_TCP, tcp_input) == -1) {
        errorf("ip_protocol_register() failure");
        return -1;
    }
    if (net_timer_register("TCP Timer", interval, tcp_timer) == -1) {
        errorf("net_timer_register() failure");
        return -1;
    }
    net_event_subscribe(event_handler, NULL);
    return 0;
}

/*
 * TCP User Command (RFC793)
 */

int
tcp_open_rfc793(struct ip_endpoint *local, struct ip_endpoint *foreign, int active)
{
    struct tcp_pcb *pcb;
    char ep1[IP_ENDPOINT_STR_LEN];
    char ep2[IP_ENDPOINT_STR_LEN];
    int state, id;

    mutex_lock(&mutex);
    pcb = tcp_pcb_alloc();
    if (!pcb) {
        errorf("tcp_pcb_alloc() failure");
        mutex_unlock(&mutex);
        return -1;
    }
    pcb->mode = TCP_PCB_MODE_RFC793;
    if (!active) {
        debugf("passive open: local=%s, waiting for connection...", ip_endpoint_ntop(local, ep1, sizeof(ep1)));
        pcb->local = *local;
        if (foreign) {
            pcb->foreign = *foreign;
        }
        pcb->state = TCP_PCB_STATE_LISTEN;
    } else {
        debugf("active open: local=%s, foreign=%s, connecting...",
            ip_endpoint_ntop(local, ep1, sizeof(ep1)), ip_endpoint_ntop(foreign, ep2, sizeof(ep2)));
        pcb->local = *local;
        pcb->foreign = *foreign;
        pcb->rcv.wnd = sizeof(pcb->buf);
        pcb->iss = random();
        if (tcp_output(pcb, TCP_FLG_SYN, NULL, 0) == -1) {
            errorf("tcp_output() failure");
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 3; /* output failure */
            tcp_pcb_release(pcb);
            mutex_unlock(&mutex);
            return -1;
        }
        pcb->snd.una = pcb->iss;
        pcb->snd.nxt = pcb->iss + 1;
        pcb->state = TCP_PCB_STATE_SYN_SENT;
    }
AGAIN:
    state = pcb->state;
    /* waiting for state changed */
    while (pcb->state == state) {
        if (sched_sleep(&pcb->state_ctx, &mutex, NULL) == -1) {
            debugf("interrupted");
            pcb->state = TCP_PCB_STATE_CLOSED;
            pcb->close_reason = 4; /* interrupted */
            tcp_pcb_release(pcb);
            mutex_unlock(&mutex);
            errno = EINTR;
            return -1;
        }
    }
    if (pcb->state != TCP_PCB_STATE_ESTABLISHED) {
        if (pcb->state == TCP_PCB_STATE_SYN_RECEIVED) {
            goto AGAIN;
        }
        errorf("open error: %d", pcb->state);
        pcb->state = TCP_PCB_STATE_CLOSED;
        tcp_pcb_release(pcb);
        mutex_unlock(&mutex);
        return -1;
    }
    id = tcp_pcb_id(pcb);
    debugf("connection established: local=%s, foreign=%s",
        ip_endpoint_ntop(&pcb->local, ep1, sizeof(ep1)), ip_endpoint_ntop(&pcb->foreign, ep2, sizeof(ep2)));
    mutex_unlock(&mutex);
    return id;
}

/*
 * Read-only state query, valid for BOTH pcb modes: the socket layer's poll
 * path (sock_connect_pending()) inspects socket-mode pcbs through here, so
 * no RFC793-mode guard. A missing pcb is a normal race with teardown (poll
 * after release), not an error worth logging.
 */
int
tcp_state(int id)
{
    struct tcp_pcb *pcb;
    int state;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        debugf("pcb not found %d", id);
        mutex_unlock(&mutex);
        return -17;
    }
    state = pcb->state;
    mutex_unlock(&mutex);
    return state;
}

/*
 * Pending socket error for getsockopt(SO_ERROR).
 *
 * Only a pcb parked in CLOSED with an abnormal close_reason represents a real
 * pending error; a normal close (reason 0) and every live/in-progress state
 * report 0. netd's sock_getsockopt() used to hardcode 0 here, which made a
 * peer RST or a retransmit-timeout connect failure invisible to consumers that
 * probe SO_ERROR after a connect (TLS clients, ffmpeg). Mirrors the
 * close_reason -> errno mapping already used by tcp_send()/tcp_receive().
 */
int
tcp_socket_error(int id)
{
    struct tcp_pcb *pcb;
    int err = 0;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        mutex_unlock(&mutex);
        return 0;
    }
    if (pcb->state == TCP_PCB_STATE_CLOSED) {
        switch (pcb->close_reason) {
        case 1:  err = ECONNRESET; break; /* RST from peer */
        case 2:  err = ETIMEDOUT;  break; /* retransmit deadline */
        case 3:  err = EIO;        break; /* tcp_output failure */
        case 4:  err = EINTR;      break; /* interrupted */
        default: err = 0;          break; /* normal close */
        }
    }
    mutex_unlock(&mutex);
    return err;
}

/*
 * TCP User Command (Socket)
 */

int
tcp_open(void)
{
    struct tcp_pcb *pcb;
    int id;

    mutex_lock(&mutex);
    pcb = tcp_pcb_alloc();
    if (!pcb) {
        errorf("tcp_pcb_alloc() failure: used=%d max=%d", tcp_pcb_used_count(), TCP_PCB_SIZE);
        mutex_unlock(&mutex);
        return -1;
    }
    pcb->mode = TCP_PCB_MODE_SOCKET;
    id = tcp_pcb_id(pcb);
    mutex_unlock(&mutex);
    return id;
}

int
tcp_connect(int id, struct ip_endpoint *foreign)
{
    struct tcp_pcb *pcb;
    struct ip_endpoint local;
    struct ip_iface *iface;
    char addr[IP_ADDR_STR_LEN];
    int p;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        errorf("pcb not found %d\n", id);
        errno = ECONNRESET;
        mutex_unlock(&mutex);
        return -17;
    }
    if (pcb->mode != TCP_PCB_MODE_SOCKET) {
        errorf("not opened in socket mode");
        errno = EINVAL;
        mutex_unlock(&mutex);
        return -1;
    }
    switch (pcb->state) {
    case TCP_PCB_STATE_ESTABLISHED:
        mutex_unlock(&mutex);
        return 0;
    case TCP_PCB_STATE_SYN_SENT:
    case TCP_PCB_STATE_SYN_RECEIVED:
        errno = EAGAIN;
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_CLOSED:
        break;
    default:
        errorf("connect invalid state: %d", pcb->state);
        errno = ECONNRESET;
        mutex_unlock(&mutex);
        return -1;
    }
    local.addr = pcb->local.addr;
    local.port = pcb->local.port;
    if (local.addr == IP_ADDR_ANY) {
        iface = ip_route_get_iface(foreign->addr);
        if (!iface) {
            errorf("ip_route_get_iface() failure");
            errno = EIO;
            mutex_unlock(&mutex);
            return -1;
        }
        debugf("select source address: %s", ip_addr_ntop(iface->unicast, addr, sizeof(addr)));
        local.addr = iface->unicast;
    }
    if (!local.port) {
        // Use random starting port to avoid sequential allocation
        int start_port = TCP_SOURCE_PORT_MIN + (random() % (TCP_SOURCE_PORT_MAX - TCP_SOURCE_PORT_MIN + 1));
        int found = 0;
        for (p = start_port; p <= TCP_SOURCE_PORT_MAX; p++) {
            local.port = hton16(p);
            struct tcp_pcb *existing = tcp_pcb_select(&local, NULL);
            if (!existing) {
                // Port is completely free
                debugf("dynamic assign source port: %d", p);
                pcb->local.port = local.port;
                found = 1;
                break;
            }
            // Check if existing connection is in TIME_WAIT and can be reused
            if (existing->state == TCP_PCB_STATE_TIME_WAIT) {
                // Allow port reuse for TIME_WAIT connections
                debugf("reusing TIME_WAIT port: %d", p);
                pcb->local.port = local.port;
                found = 1;
                break;
            }
        }
        if (!found) {
            // Try from beginning to start_port
            for (p = TCP_SOURCE_PORT_MIN; p < start_port; p++) {
                local.port = hton16(p);
                struct tcp_pcb *existing = tcp_pcb_select(&local, NULL);
                if (!existing) {
                    debugf("dynamic assign source port: %d", p);
                    pcb->local.port = local.port;
                    found = 1;
                    break;
                }
                if (existing->state == TCP_PCB_STATE_TIME_WAIT) {
                    debugf("reusing TIME_WAIT port: %d", p);
                    pcb->local.port = local.port;
                    found = 1;
                    break;
                }
            }
        }
        if (!found) {
            errorf("failed to dynamic assign source port (all ports in use)");
            errno = EBUSY;
            mutex_unlock(&mutex);
            return -1;
        }
    }
    pcb->local.addr = local.addr;
    pcb->local.port = local.port;
    pcb->foreign.addr = foreign->addr;
    pcb->foreign.port = foreign->port;
    pcb->rcv.wnd = sizeof(pcb->buf);
    pcb->iss = random();
    if (tcp_output(pcb, TCP_FLG_SYN, NULL, 0) == -1) {
        errorf("tcp_output() failure");
        pcb->state = TCP_PCB_STATE_CLOSED;
        tcp_pcb_release(pcb);
        errno = EIO;
        mutex_unlock(&mutex);
        return -1;
    }
    pcb->snd.una = pcb->iss;
    pcb->snd.nxt = pcb->iss + 1;
    pcb->state = TCP_PCB_STATE_SYN_SENT;
    mutex_unlock(&mutex);
    errno = EAGAIN;
    return -1;
}

int
tcp_bind(int id, struct ip_endpoint *local)
{
    struct tcp_pcb *pcb, *exist;
    char ep[IP_ENDPOINT_STR_LEN];

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        errorf("pcb not found %d\n", id);
        mutex_unlock(&mutex);
        return -17;
    }
    if (pcb->mode != TCP_PCB_MODE_SOCKET) {
        errorf("not opened in socket mode");
        mutex_unlock(&mutex);
        return -1;
    }
    exist = tcp_pcb_select(local, NULL);
    if (exist) {
        errorf("already bound, exist=%s", ip_endpoint_ntop(&exist->local, ep, sizeof(ep)));
        mutex_unlock(&mutex);
        return -1;
    }
    pcb->local = *local;
    debugf("success: local=%s", ip_endpoint_ntop(&pcb->local, ep, sizeof(ep)));
    mutex_unlock(&mutex);
    return 0;
}

int
tcp_readable(int id)
{
    struct tcp_pcb *pcb;
    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        mutex_unlock(&mutex);
        return 0;
    }
    /*
     * A LISTEN socket is "readable" for poll()/accept() purposes when it has a
     * completed connection queued in its backlog. Without this, poll(POLLIN)
     * on the listening socket never reports readiness and a single-process
     * acceptor (e.g. telnetd) sleeps forever waiting for the first client.
     */
    if (pcb->state == TCP_PCB_STATE_LISTEN) {
        int has_pending = (pcb->backlog.num > 0);
        mutex_unlock(&mutex);
        return has_pending;
    }
    size_t remain = sizeof(pcb->buf) - pcb->rcv.wnd;
    int readable = (remain > 0) || (pcb->state == TCP_PCB_STATE_CLOSE_WAIT) || (pcb->state == TCP_PCB_STATE_CLOSED);
    mutex_unlock(&mutex);
    return readable;
}

int
tcp_poll_readable(int id)
{
    struct tcp_pcb *pcb;
    int readable;

    if (pthread_mutex_trylock(&mutex) != 0) {
        return -1;
    }
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    if (pcb->state == TCP_PCB_STATE_LISTEN) {
        readable = (pcb->backlog.num > 0) ? 1 : 0;
        pthread_mutex_unlock(&mutex);
        return readable;
    }
    readable = (((size_t)sizeof(pcb->buf) - pcb->rcv.wnd) > 0 ||
                pcb->state == TCP_PCB_STATE_CLOSE_WAIT ||
                pcb->state == TCP_PCB_STATE_CLOSED) ? 1 : 0;
    pthread_mutex_unlock(&mutex);
    return readable;
}

int
tcp_data_readable(int id)
{
    struct tcp_pcb *pcb;
    int readable = 0;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (pcb) {
        size_t remain = sizeof(pcb->buf) - pcb->rcv.wnd;
        readable = (remain > 0) ? 1 : 0;
    }
    mutex_unlock(&mutex);
    return readable;
}

int
tcp_recv_remain(int id)
{
    struct tcp_pcb *pcb;
    int remain = -1;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (pcb) {
        remain = (int)(sizeof(pcb->buf) - pcb->rcv.wnd);
    }
    mutex_unlock(&mutex);
    return remain;
}

int
tcp_writable(int id)
{
    struct tcp_pcb *pcb;
    int writable = 1;
    uint32_t inflight = 0;
    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        /* No pcb: let a write proceed so it returns an immediate error. */
        mutex_unlock(&mutex);
        return 1;
    }
    /*
     * Only report writable when the send window actually has capacity. When
     * the window is closed tcp_send() returns EAGAIN and the netd worker task
     * falls back to IDLE; if poll() still advertised WR purely from the IDLE
     * task state, a client like sshd would busy-spin write()/poll() forever
     * (appearing hung) instead of blocking until the window reopens. The ACK
     * path calls task_wakeup_tcp_writers() to raise VFS_EVT_WR once capacity
     * returns. Non-transfer states stay writable so closed/erroring writes
     * return immediately rather than block.
     */
    if (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
        pcb->state == TCP_PCB_STATE_CLOSE_WAIT) {
        inflight = pcb->snd.nxt - pcb->snd.una;
        writable = (inflight < pcb->snd.wnd) ? 1 : 0;
        if (writable && pcb->queue.num >= TCP_RETRANSMIT_QUEUE_MAX) {
            /* tcp_send() also stops when the retransmit queue is full;
             * report not-writable as well or poll()-driven clients
             * busy-spin on EAGAIN instead of blocking for the wakeup. */
            writable = 0;
        }
    }
    mutex_unlock(&mutex);
    return writable;
}

int
tcp_poll_writable(int id)
{
    struct tcp_pcb *pcb;
    int writable = 1;
    uint32_t inflight = 0;

    if (pthread_mutex_trylock(&mutex) != 0) {
        return -1;
    }
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    if (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
        pcb->state == TCP_PCB_STATE_CLOSE_WAIT) {
        inflight = pcb->snd.nxt - pcb->snd.una;
        writable = (inflight < pcb->snd.wnd) ? 1 : 0;
        if (writable && pcb->queue.num >= TCP_RETRANSMIT_QUEUE_MAX) {
            writable = 0;
        }
    }
    pthread_mutex_unlock(&mutex);
    return writable;
}

int
tcp_timer_active(void)
{
    int active = 0;
    struct tcp_pcb *pcb;

    mutex_lock(&mutex);
    if (pcb_alloc_count == pcb_release_count) {
        /* no live PCBs: skip the full table scan */
        mutex_unlock(&mutex);
        return 0;
    }
    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        if (pcb->state == TCP_PCB_STATE_TIME_WAIT || pcb->queue.num > 0 ||
            pcb->persist_probing || pcb->delack_pending) {
            active = 1;
            break;
        }
    }
    mutex_unlock(&mutex);
    return active;
}

int
tcp_timer_due_us(void)
{
    int min_due_us = -1;
    struct tcp_pcb *pcb;
    struct timeval now;

    mutex_lock(&mutex);
    if (pcb_alloc_count == pcb_release_count) {
        /* no live PCBs: no timers can be pending, skip scan + gettimeofday */
        mutex_unlock(&mutex);
        return -1;
    }
    gettimeofday(&now, NULL);
    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        struct tcp_queue_entry *entry;

        if (pcb->state == TCP_PCB_STATE_FREE) {
            continue;
        }

        if (pcb->state == TCP_PCB_STATE_TIME_WAIT) {
            tcp_due_track_min(&min_due_us, tcp_due_from_deadline(&now, &pcb->tw_timer));
        }

        entry = queue_peek(&pcb->queue);
        if (entry) {
            struct timeval timeout = entry->last;

            timeval_add_usec(&timeout, entry->rto);
            tcp_due_track_min(&min_due_us, tcp_due_from_deadline(&now, &timeout));
        }

        if (pcb->persist_probing &&
            (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
             pcb->state == TCP_PCB_STATE_CLOSE_WAIT)) {
            tcp_due_track_min(&min_due_us, tcp_due_from_deadline(&now, &pcb->persist_timer));
        }

        if (pcb->delack_pending &&
            (pcb->state == TCP_PCB_STATE_ESTABLISHED ||
             pcb->state == TCP_PCB_STATE_CLOSE_WAIT)) {
            struct timeval deadline = pcb->delack_timer;
            timeval_add_usec(&deadline, TCP_DELACK_TIMEOUT_USEC);
            tcp_due_track_min(&min_due_us, tcp_due_from_deadline(&now, &deadline));
        }
    }
    mutex_unlock(&mutex);
    return min_due_us;
}

/*
 * True while any live connection has unacknowledged segments in flight AND
 * the most recent (re)transmission is fresh (within ~50ms), i.e. the peer's
 * ACK is plausibly imminent. intr_step uses this to keep the poll cadence
 * fast between a TX burst and the ACKs: the retransmit-queue RTO reported by
 * tcp_timer_due_us() is hundreds of ms away, but ACKs land within ~1 RTT and
 * re-open the send path, so sleeping toward the RTO deadline stalls every
 * bulk window. The freshness bound keeps a retransmit storm to a dead peer
 * from pinning the fast cadence forever (raspix netd-spin lesson).
 */
int
tcp_inflight_pending(void)
{
    struct tcp_pcb *pcb;
    struct timeval now, diff;
    int pending = 0;

    mutex_lock(&mutex);
    if (pcb_alloc_count == pcb_release_count) {
        mutex_unlock(&mutex);
        return 0;
    }
    gettimeofday(&now, NULL);
    for (pcb = pcbs; pcb < tailof(pcbs); pcb++) {
        struct tcp_queue_entry *entry;

        if (pcb->state == TCP_PCB_STATE_FREE) {
            continue;
        }
        entry = queue_peek(&pcb->queue);
        if (!entry) {
            continue;
        }
        timersub(&now, &entry->last, &diff);
        if (diff.tv_sec == 0 && diff.tv_usec < 50000) {
            pending = 1;
            break;
        }
    }
    mutex_unlock(&mutex);
    return pending;
}

int
tcp_listen(int id, int backlog)
{
    struct tcp_pcb *pcb;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        errorf("pcb not found %d\n", id);
        mutex_unlock(&mutex);
        return -17;
    }
    if (pcb->mode != TCP_PCB_MODE_SOCKET) {
        errorf("not opened in socket mode");
        mutex_unlock(&mutex);
        return -1;
    }
    pcb->state = TCP_PCB_STATE_LISTEN;
    (void)backlog; // TODO: set backlog
    mutex_unlock(&mutex);
    return 0;
}

int
tcp_accept(int id, struct ip_endpoint *foreign)
{
    struct tcp_pcb *pcb, *new_pcb;
    int new_id;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        errorf("pcb not found %d\n", id);
        errno = EBADF;
        mutex_unlock(&mutex);
        return -17;
    }
    if (pcb->mode != TCP_PCB_MODE_SOCKET) {
        errorf("not opened in socket mode");
        errno = EINVAL;
        mutex_unlock(&mutex);
        return -1;
    }
    if (pcb->state != TCP_PCB_STATE_LISTEN) {
        errorf("not in LISTEN state");
        errno = EINVAL;
        mutex_unlock(&mutex);
        return -1;
    }
    new_pcb = queue_pop(&pcb->backlog);
    if (!new_pcb) {
        mutex_unlock(&mutex);
        errno = EAGAIN;
        return -1;
    }
    /* Accepted: sever the backlog linkage so a later release of this pcb
     * does not scan the (possibly reused) parent. */
    new_pcb->parent = NULL;
    if (foreign) {
        *foreign = new_pcb->foreign;
    }
    new_id = tcp_pcb_id(new_pcb);
    mutex_unlock(&mutex);
    return new_id;
}

/*
 * TCP User Command (Common)
 */

ssize_t
tcp_send(int id, uint8_t *data, size_t len)
{
    struct tcp_pcb *pcb;
    ssize_t sent = 0;
    struct ip_iface *iface;
    size_t mss, cap, slen, inflight;
    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        mutex_unlock(&mutex);
        errno = ECONNRESET;
        return -1;
    }
RETRY:
    switch (pcb->state) {
    case TCP_PCB_STATE_CLOSED:
        if (pcb->close_reason == 1) {
            errorf("connection reset by peer");
            errno = ECONNRESET;
        } else if (pcb->close_reason == 2) {
            errorf("connection timeout");
            errno = ETIMEDOUT;
        } else {
            errorf("connection closed (reason=%d)", pcb->close_reason);
            errno = ECONNRESET;
        }
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_LISTEN:
        // ignore: change the connection from passive to active
        errorf("this connection is passive");
        errno = EPIPE;
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_SYN_SENT:
    case TCP_PCB_STATE_SYN_RECEIVED:
        // ignore: Queue the data for transmission after entering ESTABLISHED state
        errorf("insufficient resources");
        errno = EAGAIN;
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_CLOSE_WAIT:
        iface = ip_route_get_iface(pcb->local.addr);
        if (!iface) {
            errorf("iface not found");
            errno = EIO;
            mutex_unlock(&mutex);
            return -1;
        }
        mss = NET_IFACE(iface)->dev->mtu - (IP_HDR_SIZE_MIN + sizeof(struct tcp_hdr));
        // Send multiple segments without waiting for ACK (pipelining)
        while (sent < (ssize_t)len) {
            inflight = pcb->snd.nxt - pcb->snd.una;
            if (inflight >= pcb->snd.wnd) {
                cap = 0;
            } else {
                cap = pcb->snd.wnd - inflight;
            }
            if (cap && pcb->queue.num >= TCP_RETRANSMIT_QUEUE_MAX) {
                /*
                 * Retransmit queue full. Sending more would emit segments
                 * with no retransmission backing; a single loss of such a
                 * segment permanently stalls snd.una (cumulative ACKs can't
                 * pass the hole, and the segment is never resent) - the
                 * raspix scp stall. Stop pipelining exactly like a closed
                 * window: the ACK path frees slots and wakes writers, and
                 * RTO retransmits of backed entries are the backstop when
                 * ACKs pause.
                 */
                cap = 0;
            }
            if (!cap) {
                debugf("tcp_send stalled desc=%d state=%d inflight=%u wnd=%u q=%u len=%zu sent=%zd",
                       id, pcb->state, (unsigned int)inflight, (unsigned int)pcb->snd.wnd,
                       pcb->queue.num, len, sent);
                /*
                 * Send window is closed. Do NOT sched_sleep() here: this runs
                 * on the single per-socket netd worker thread, and blocking it
                 * stalls the whole socket and races with the ACK-driven wakeup,
                 * which is why long transfers hang while reads stay stable.
                 * Mirror the RECV path: return what was already sent, or EAGAIN
                 * when nothing was sent, and let the client block on VFS_EVT_WR.
                 * The window-open ACK path calls task_wakeup_tcp_writers() to
                 * raise VFS_EVT_WR and resume the client.
                 *
                 * Arm the persist probe ONLY for a genuinely closed window: the
                 * probe exists to elicit a window update the peer may have
                 * lost. A queue-full stall with an open window is not a window
                 * stall -- the RTO retransmit path plus ACK-driven cleanup are
                 * its backstop, and probing there only triggers dup-ACKs that
                 * churn wakeups and keep resetting the persist backoff.
                 */
                if (inflight >= pcb->snd.wnd) {
                    tcp_persist_arm(pcb);
                }
                if (sent > 0) {
                    break;
                }
                mutex_unlock(&mutex);
                errno = EAGAIN;
                return -1;
            }
            slen = MIN(MIN(mss, len - sent), cap);
            /* Window is open — disarm persist timer if it was armed */
            if (pcb->persist_probing)
                tcp_persist_disarm(pcb);
            // Only set PSH flag on the last segment
            uint8_t flg = TCP_FLG_ACK;
            if (sent + slen >= (ssize_t)len || cap - slen < mss) {
                flg |= TCP_FLG_PSH;
            }
            if (tcp_output(pcb, flg, data + sent, slen) == -1) {
                /*
                 * A failed wire send is packet loss, not a dead connection.
                 * tcp_output() already put this segment on the retransmit
                 * queue (cap==0 above guarantees a free slot), and the RTO
                 * timer resends it - tcp_retransmit_queue_emit() likewise
                 * ignores tcp_output_segment() failures.
                 *
                 * The device reports -1 for transient link congestion: wl0
                 * answers VFS_ERR_RETRY while the driver TX queue is full and
                 * ether_tap_write() gives up after ETHER_TAP_TX_WAIT_MS.
                 * Closing the PCB here turned every such hiccup into a
                 * mid-transfer reset, so a bulk transfer was torn down and
                 * restarted by the peer instead of recovering by
                 * retransmission - the dominant throughput limit on wifi.
                 *
                 * Account the segment as sent so snd.nxt stays in step with
                 * the queued entry; retrying with the same seq would push a
                 * duplicate entry for every attempt. Stop pipelining: the
                 * link has no room right now.
                 */
                debugf("tx deferred (link backpressure), retransmit pending seq=%u len=%zu",
                       pcb->snd.nxt, slen);
                pcb->snd.nxt += slen;
                sent += slen;
                break;
            }
            pcb->snd.nxt += slen;
            sent += slen;
            /*
             * Keep pipelining until the caller's buffer is drained or the
             * window/retransmit-queue caps close (cap==0 above). The old
             * "stop after mss*4" break forced a short write every ~5.8KB,
             * which with the per-write IPC round-trip capped bulk TX far
             * below the link rate. The retransmit queue limit
             * (TCP_RETRANSMIT_QUEUE_MAX segments) still bounds one call's
             * burst, so the stack mutex is not held unboundedly.
             */
        }
        break;
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        errorf("connection closing");
        errno = EPIPE;
        mutex_unlock(&mutex);
        return -1;
    default:
        errorf("unknown state '%u'", pcb->state);
        errno = EIO;
        mutex_unlock(&mutex);
        return -1;
    }
    mutex_unlock(&mutex);
    return sent;
}

ssize_t
tcp_receive(int id, uint8_t *data, size_t size)
{
    struct tcp_pcb *pcb;
    size_t remain, len;
    size_t prev_wnd;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        mutex_unlock(&mutex);
        errno = ECONNRESET;
        return -1;
    }

    switch (pcb->state) {
    case TCP_PCB_STATE_CLOSED:
        errno = ECONNRESET;
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_LISTEN:
    case TCP_PCB_STATE_SYN_SENT:
    case TCP_PCB_STATE_SYN_RECEIVED:
        errno = EAGAIN;
        mutex_unlock(&mutex);
        return -1;
    case TCP_PCB_STATE_ESTABLISHED:
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
        remain = sizeof(pcb->buf) - pcb->rcv.wnd;
        if (!remain) {
            /*
             * Non-blocking: the per-socket worker must NEVER sleep inside the
             * TCP stack on pcb->recv_ctx (a different sched_ctx than the
             * worker's task->wait_ctx, so the O(1) targeted wakeup cannot
             * reach it). Return EAGAIN so the worker re-arms and parks on
             * task->wait_ctx, where task_wakeup_tcp_readers() releases it once
             * data actually arrives.
             */
            errno = EAGAIN;
            mutex_unlock(&mutex);
            return -1;
        }
        break;
    case TCP_PCB_STATE_CLOSE_WAIT:
        remain = sizeof(pcb->buf) - pcb->rcv.wnd;
        if (remain) {
            break;
        }
        mutex_unlock(&mutex);
        return 0;
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        mutex_unlock(&mutex);
        return 0;
    default:
        errorf("unknown state '%u'", pcb->state);
        errno = EIO;
        mutex_unlock(&mutex);
        return -1;
    }
    len = MIN(size, remain);
    prev_wnd = pcb->rcv.wnd;
    memcpy(data, pcb->buf, len);
    /*
     * The seq->offset mapping is anchored at buf[0] == rcv.nxt - remain, so
     * consuming `len` bytes shifts every live byte down by `len` -- including
     * an out-of-order stash parked beyond the in-order region (its offset is
     * remain + (ooo_seq - rcv.nxt)). Move the stash along or the mapping the
     * hole-merge relies on silently breaks after the first read.
     */
    {
        size_t tail = remain - len;
        if (pcb->ooo_len) {
            tail = remain + (size_t)(pcb->ooo_seq - pcb->rcv.nxt) + pcb->ooo_len - len;
        }
        memmove(pcb->buf, pcb->buf + len, tail);
    }
    pcb->rcv.wnd += len;
    if (pcb->rcv.wnd != prev_wnd) {
        tcp_output(pcb, TCP_FLG_ACK, NULL, 0);
    }

    mutex_unlock(&mutex);
    return len;
}

int
tcp_close(int id)
{
    struct tcp_pcb *pcb;

    mutex_lock(&mutex);
    pcb = tcp_pcb_get(id);
    if (!pcb) {
        mutex_unlock(&mutex);
        return -17;
    }
    switch (pcb->state) {
    case TCP_PCB_STATE_CLOSED:
        tcp_pcb_release(pcb);
        mutex_unlock(&mutex);
        return 0;
    case TCP_PCB_STATE_LISTEN:
        pcb->state = TCP_PCB_STATE_CLOSED;
        pcb->close_reason = 0; /* normal close */
        break;
    case TCP_PCB_STATE_SYN_SENT:
        /*
         * Best-effort RST: a wire failure here is wl0 TX backpressure, and
         * the old -1 return made sock_close() retry then give up, orphaning
         * the pcb forever. Nothing retransmits an RST anyway; the peer's
         * SYN-ACK retransmits will meet a CLOSED pcb and get RSTs then.
         */
        tcp_output_segment(pcb->snd.nxt, pcb->rcv.nxt, TCP_FLG_RST | TCP_FLG_ACK, 0, NULL, 0, &pcb->local, &pcb->foreign);
        pcb->state = TCP_PCB_STATE_CLOSED;
        pcb->close_reason = 0; /* normal close */
        break;
    case TCP_PCB_STATE_SYN_RECEIVED:
        /*
         * Same discipline as tcp_send(): tcp_output() queued the FIN on the
         * retransmit queue BEFORE the wire attempt, so a failed send is TX
         * backpressure, not a failed close -- the RTO timer delivers the
         * FIN. Returning -1 without advancing state/snd.nxt made
         * sock_close() retry: every retry queued a duplicate FIN entry at
         * the same seq, and when the wl0 congestion at sshd-upload end
         * outlived the retries the pcb was abandoned in ESTABLISHED/
         * CLOSE_WAIT with no owning socket -- leaked until "no free PCB"
         * refused every new connection.
         */
        tcp_output(pcb, TCP_FLG_ACK | TCP_FLG_FIN, NULL, 0);
        pcb->snd.nxt++;
        pcb->state = TCP_PCB_STATE_FIN_WAIT1;
        break;
    case TCP_PCB_STATE_ESTABLISHED:
        /* FIN rides the retransmit queue; wire failure is backpressure. */
        tcp_output(pcb, TCP_FLG_ACK | TCP_FLG_FIN, NULL, 0);
        pcb->snd.nxt++;
        pcb->state = TCP_PCB_STATE_FIN_WAIT1;
        break;
    case TCP_PCB_STATE_FIN_WAIT1:
    case TCP_PCB_STATE_FIN_WAIT2:
        debugf("connection closing");
        mutex_unlock(&mutex);
        return 0;
    case TCP_PCB_STATE_CLOSE_WAIT:
        /* FIN rides the retransmit queue; wire failure is backpressure. */
        tcp_output(pcb, TCP_FLG_ACK | TCP_FLG_FIN, NULL, 0);
        pcb->snd.nxt++;
        pcb->state = TCP_PCB_STATE_LAST_ACK;
        break;
    case TCP_PCB_STATE_CLOSING:
    case TCP_PCB_STATE_LAST_ACK:
    case TCP_PCB_STATE_TIME_WAIT:
        debugf("connection closing");
        mutex_unlock(&mutex);
        return 0;
    default:
        errorf("unknown state '%u'", pcb->state);
        mutex_unlock(&mutex);
        return -1;
    }

    if (pcb->state == TCP_PCB_STATE_CLOSED) {
        tcp_pcb_release(pcb);
    } else {
        tcp_sched_wakeup_all(pcb);
    }
    mutex_unlock(&mutex);
    return 0;
}
