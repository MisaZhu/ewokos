#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
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
static pthread_mutex_t gMutex;
int tid;

#define NETD_BUSY_SLEEP_US 1000U
#define NETD_IDLE_SLEEP_STEP_US 1000U
#define NETD_IDLE_SLEEP_MAX_US 10000U

int dflag [16];
int dcnt = 0;

void raise_softirq(uint32_t  sig){
    if(sig < SIGMAX){
        gSignel[sig]++; 
    }
}

static void print_trace(void){
    int start = dcnt - 16;
    slog("%d %d:", dcnt,   gSignel[SIGNET]);
    for(int i = 0; i < 16; i++){
        slog("%d ", dflag[start%(sizeof(dflag)/sizeof(int))]);
        start++;
    }
    slog("\n");
}

void intr_loop(void) {
	struct irq_entry *entry;
    uint32_t sleep_us = NETD_BUSY_SLEEP_US;
    while(1){
        int had_signal = 0;
        int tap_pending = 0;
        int need_task_scan = task_has_read_watchers();
        int task_ready = 0;

        while(gSignel[SIGNET]){
            had_signal = 1;
            net_protocol_handler();
            gSignel[SIGNET]--;
        }
        while(gSignel[SIGINT]){
            had_signal = 1;
            net_event_handler();
            gSignel[SIGINT]--;
        }
        for (entry = irq_vec; entry; entry = entry->next) {
            if (entry->irq == SIGIRQ) {
                int cnt = tap_select(entry->dev);
                if (cnt > 0) {
                    tap_pending = 1;
                }
                for(int i = 0; i < cnt; i++){
                    entry->handler(entry->irq, entry->dev);
                }
            }
        }
        net_timer_handler();
        start_task();
        if (need_task_scan) {
            task_ready = task_check_read_events();
        }

        if (had_signal || tap_pending || task_ready || tcp_timer_active()) {
            sleep_us = NETD_BUSY_SLEEP_US;
        } else if (sleep_us < NETD_IDLE_SLEEP_MAX_US) {
            sleep_us += NETD_IDLE_SLEEP_STEP_US;
            if (sleep_us > NETD_IDLE_SLEEP_MAX_US) {
                sleep_us = NETD_IDLE_SLEEP_MAX_US;
            }
        }
        usleep(sleep_us);
    }
    return;
}

void* debug_thread(void* p){
    while(1){
        print_trace();
        int ret = sleep(1);
    }
}

int
intr_init(void)
{
    pthread_mutex_init(&gMutex, NULL);
    return 0;
}
