#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <ewoksys/vfs.h>
#include <ewoksys/semaphore.h>

#include "stack/util.h"
#include "stack/net.h"
#include "platform.h"


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

int dflag [16];
int dcnt = 0;
int debug_flag = 0;

void raise_softirq(uint32_t  sig){
    if(sig < SIGMAX){
        gSignel[sig]++; 
        //proc_wakeup(NET_BLOCK_EVT);
    }
}

static void print_trace(void){
    int start = dcnt - 16;
    klog("%d %d:", dcnt,   gSignel[SIGNET]);
    for(int i = 0; i < 16; i++){
        klog("%d ", dflag[start%(sizeof(dflag)/sizeof(int))]);
        start++;
    }
    klog("\n");
}

void intr_loop(void) {
	struct irq_entry *entry;
    while(1){
        while(gSignel[SIGNET]){
            net_protocol_handler();
            gSignel[SIGNET]--; 
        }
        while(gSignel[SIGINT]){
            net_event_handler();
            gSignel[SIGINT]--;
        }
        for (entry = irq_vec; entry; entry = entry->next) {
                if (entry->irq == SIGIRQ) {
                    int cnt = tap_select(entry->dev);
                    for(int i = 0; i < cnt; i++){
                        entry->handler(entry->irq, entry->dev);
                }
            }
        }
        net_timer_handler();
        start_task();
        usleep(1000);
    }
    return 0;
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
