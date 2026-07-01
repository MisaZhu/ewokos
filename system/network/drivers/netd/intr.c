#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/semaphore.h>

#include "stack/util.h"
#include "stack/net.h"
#include "stack/tcp.h"
#include "platform.h"
#include "task.h"


struct irq_entry {
    struct irq_entry *next;
    unsigned int irq;
    int (*handler)(unsigned int irq, void *dev);
    int flags;
    char name[16];
    void *dev;
};

struct irq_entry *irq_vec;

int
intr_request_irq(unsigned int irq, int (*handler)(unsigned int irq, void *dev), int flags, const char *name, void *dev)
{
    debugf("irq=%u, handler=%p, flags=%d, name=%s, dev=%p", irq, handler, flags, name, dev);
    struct irq_entry *entry;
    for (entry = irq_vec; entry; entry = entry->next) {
        if (entry->irq == irq) {
            if (entry->flags ^ NET_IRQ_SHARED || flags ^ NET_IRQ_SHARED) {
                errorf("conflicts with already registered IRQs");
                return -1;
            }
        }
    }
    entry = memory_alloc(sizeof(*entry));
    if (!entry) {
        errorf("memory_alloc() failure");
        return -1;
    }
    entry->irq = irq;
    entry->handler = handler;
    entry->flags = flags;
    strncpy(entry->name, name, sizeof(entry->name)-1);
    entry->dev = dev;
    entry->next = irq_vec;
    irq_vec = entry;
    debugf("registered: irq=%u, name=%s", irq, name);
    return 0;
}

struct irq_entry *irq_vec;
#define NET_BLOCK_EVT 66666
static uint32_t gSignel[SIGMAX] = {0};
static mutex_t gSignelLock = MUTEX_INITIALIZER;

#define NETD_BUSY_SLEEP_US 10000U
#define NETD_IDLE_SLEEP_STEP_US 5000U
#define NETD_IDLE_SLEEP_MAX_US 50000U

int dflag[16];
int dcnt = 0;
/* #region debug-point A-E:softirq */
static uint32_t dbg_signet_raise_count = 0;
static uint32_t dbg_signet_handle_count = 0;
/* #endregion */

static uint32_t softirq_pending(uint32_t sig)
{
    uint32_t pending = 0;

    if (sig >= SIGMAX)
        return 0;

    mutex_lock(&gSignelLock);
    pending = gSignel[sig];
    mutex_unlock(&gSignelLock);
    return pending;
}

static int softirq_take(uint32_t sig)
{
    int pending = 0;

    if (sig >= SIGMAX)
        return 0;

    mutex_lock(&gSignelLock);
    if (gSignel[sig] != 0) {
        gSignel[sig] = 0;
        pending = 1;
    }
    mutex_unlock(&gSignelLock);
    return pending;
}

void raise_softirq(uint32_t  sig){
    if(sig < SIGMAX){
        mutex_lock(&gSignelLock);
        gSignel[sig] = 1;
        mutex_unlock(&gSignelLock);
        /* #region debug-point A-E:softirq */
        if (sig == SIGNET) {
            dbg_signet_raise_count++;
            if (dbg_signet_raise_count <= 128 ||
                    (dbg_signet_raise_count % 32) == 0) {
                slog("netd: signet raise seq=%u pending=%u\n",
                        dbg_signet_raise_count,
                        softirq_pending(sig));
            }
        }
        /* #endregion */
    }
}

int intr_poll_once(void) {
    struct irq_entry *entry;
    int handled = 0;

    while (softirq_take(SIGNET)) {
        /* #region debug-point A-E:softirq */
        dbg_signet_handle_count++;
        if (dbg_signet_handle_count <= 128 ||
                (dbg_signet_handle_count % 32) == 0) {
            slog("netd: signet handle seq=%u pending=%u\n",
                    dbg_signet_handle_count,
                    softirq_pending(SIGNET));
        }
        /* #endregion */
        net_protocol_handler();
        handled = 1;
    }
    while (softirq_take(SIGINT)) {
        net_event_handler();
        handled = 1;
    }
    for (entry = irq_vec; entry; entry = entry->next) {
        if (entry->irq == SIGIRQ && entry->handler(entry->irq, entry->dev) == 0) {
            handled = 1;
        }
    }
    if (softirq_pending(SIGNET)) {
        while (softirq_take(SIGNET)) {
            /* #region debug-point A-E:softirq */
            dbg_signet_handle_count++;
            if (dbg_signet_handle_count <= 128 ||
                    (dbg_signet_handle_count % 32) == 0) {
                slog("netd: signet handle seq=%u pending=%u\n",
                        dbg_signet_handle_count,
                        softirq_pending(SIGNET));
            }
            /* #endregion */
            net_protocol_handler();
            handled = 1;
        }
    }
    return handled;
}

void intr_loop(void) {
	struct irq_entry *entry;
    uint32_t sleep_us = NETD_BUSY_SLEEP_US;
    while(1){
        int had_signal = 0;
        int tap_pending = 0;
        int need_task_scan = task_has_read_watchers();
        int task_ready = 0;

        while(softirq_take(SIGNET)){
            had_signal = 1;
            /* #region debug-point A-E:softirq */
            dbg_signet_handle_count++;
            if (dbg_signet_handle_count <= 128 ||
                    (dbg_signet_handle_count % 32) == 0) {
                slog("netd: signet handle seq=%u pending=%u\n",
                        dbg_signet_handle_count,
                        softirq_pending(SIGNET));
            }
            /* #endregion */
            net_protocol_handler();
        }
        while(softirq_take(SIGINT)){
            had_signal = 1;
            net_event_handler();
        }
        for (entry = irq_vec; entry; entry = entry->next) {
            if (entry->irq == SIGIRQ) {
                int ret = entry->handler(entry->irq, entry->dev);
                if (ret == 0) {
                    tap_pending = 1;
                }
            }
        }
        net_timer_handler();
        start_task();
        if (need_task_scan) {
            task_ready = task_check_read_events();
        }

        kernel_tic(NULL, NULL);

        if (had_signal || tap_pending || task_ready) {
            sleep_us = NETD_BUSY_SLEEP_US;
        } else if (sleep_us < NETD_IDLE_SLEEP_MAX_US) {
            /*
             * TCP timers are evaluated against wall-clock time in
             * net_timer_handler(), so keeping the loop pinned at 1ms whenever a
             * retransmit/TIME_WAIT entry exists just burns CPU. Only real
             * packets or read-ready wakeups need the fast path.
             */
            sleep_us += NETD_IDLE_SLEEP_STEP_US;
            if (sleep_us > NETD_IDLE_SLEEP_MAX_US) {
                sleep_us = NETD_IDLE_SLEEP_MAX_US;
            }
        }
        usleep(sleep_us);
    }
    return;
}

int
intr_init(void)
{
    return 0;
}
