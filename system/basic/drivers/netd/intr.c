#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

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

static int
intr_timer_setup(struct itimerspec *interval)
{

}

static int soft_signal = 0;
struct irq_entry *irq_vec;

static uint32_t gSignel = 0;

void raise_softirq(uint32_t  sig){
	//printf("put signal %08x\n", sig);
    gSignel |= sig; 
}

void* intr_thread(void* p) {
	struct irq_entry *entry;
    while(1){
       if(SIGUSR1 & gSignel){
           net_protocol_handler();
           gSignel &= ~SIGUSR1; 
       }
       if(SIGUSR2 & gSignel){
           net_event_handler();
          gSignel &= ~SIGUSR2; 
       }
       if(SIGALRM & gSignel){
           net_timer_handler();
         gSignel &= ~SIGALRM; 
       }
       if(gSignel){
           for (entry = irq_vec; entry; entry = entry->next) {
               if (entry->irq & gSignel) {
                   debugf("irq=%d, name=%s", entry->irq, entry->name);
                   entry->handler(entry->irq, entry->dev);
                   gSignel &= ~entry->irq;
               }
           }
       }
       proc_usleep(1000);
    }
    return 0;
}

int
intr_run(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, intr_thread, NULL);
}

int
intr_init(void)
{
    return 0;
}
