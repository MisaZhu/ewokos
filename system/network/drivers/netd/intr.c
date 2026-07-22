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

#define NETD_BUSY_SLEEP_US 1000U
#define NETD_IDLE_SLEEP_STEP_US 1000U
#define NETD_IDLE_SLEEP_MAX_US 10000U
#define NETD_DEEP_IDLE_SLEEP_MAX_US 50000U

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
    }
}

int intr_poll_once(void) {
    struct irq_entry *entry;
    int handled = 0;

    while (softirq_take(SIGNET)) {
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
            net_protocol_handler();
            handled = 1;
        }
    }
    return handled;
}

void intr_protocol_loop(void) {
    uint32_t sleep_us = NETD_BUSY_SLEEP_US;

    while (1) {
        int more_pending = 0;

        while (softirq_take(SIGNET)) {
            net_protocol_handler();
        }

        /*
         * Only keep the fast cadence when new packets were queued *while* we
         * were draining (i.e. sustained load). Processing a single enqueued
         * frame (e.g. a lone broadcast/ARP) and then finding the queue empty
         * must let the loop back off, otherwise ambient broadcast traffic keeps
         * this thread cycling at the busy cadence and idle CPU load fluctuates.
         */
        if (softirq_pending(SIGNET)) {
            more_pending = 1;
        }

        if (more_pending) {
            sleep_us = NETD_BUSY_SLEEP_US;
        } else {
            if (sleep_us < NETD_IDLE_SLEEP_MAX_US) {
                sleep_us += NETD_IDLE_SLEEP_STEP_US;
                if (sleep_us > NETD_IDLE_SLEEP_MAX_US)
                    sleep_us = NETD_IDLE_SLEEP_MAX_US;
            }
        }
        usleep(sleep_us);
    }
}

void intr_loop(void) {
	struct irq_entry *entry;
    uint32_t sleep_us = NETD_BUSY_SLEEP_US;
    while(1){
        int had_signal = 0;
        int tap_pending = 0;
        int task_ready = 0;
        int tcp_timer_busy = 0;

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

        kernel_tic(NULL, NULL);

        tcp_timer_busy = tcp_timer_active();

        if (had_signal || tap_pending || task_ready) {
            sleep_us = NETD_BUSY_SLEEP_US;
        } else if (tcp_timer_busy) {
            if (sleep_us < NETD_IDLE_SLEEP_MAX_US) {
                /*
                 * Active TCP timers (retransmit/TIME_WAIT) still need a
                 * relatively tight polling cadence, but idle acceptors do not.
                 */
                sleep_us += NETD_IDLE_SLEEP_STEP_US;
                if (sleep_us > NETD_IDLE_SLEEP_MAX_US) {
                    sleep_us = NETD_IDLE_SLEEP_MAX_US;
                }
            }
        } else if (sleep_us < NETD_DEEP_IDLE_SLEEP_MAX_US) {
            /*
             * TCP timers are evaluated against wall-clock time in
             * net_timer_handler(), so keeping the loop pinned at 1ms whenever a
             * retransmit/TIME_WAIT entry exists just burns CPU. Only real
             * packets or read-ready wakeups need the fast path.
             */
            sleep_us += NETD_IDLE_SLEEP_STEP_US;
            if (sleep_us > NETD_DEEP_IDLE_SLEEP_MAX_US) {
                sleep_us = NETD_DEEP_IDLE_SLEEP_MAX_US;
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
