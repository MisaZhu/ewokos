#ifndef KINTR_H
#define KINTR_H

#include <stdint.h>
#include <interrupt.h>
#include <kernel/context.h>
#include <kernel/ipc.h>

struct st_proc;

enum {
	INTR_STATE_IDLE = 0,
	INTR_STATE_START,
	INTR_STATE_WORKING
};

typedef struct {
	ewokos_addr_t entry;
	uint32_t      interrupt;
	ewokos_addr_t data;

	uint32_t      state;
	uint32_t      stack; //mapped stack page

    ipc_res_t     saved_ipc_res;
	saved_state_t saved_state;
} proc_interrupt_t;

void interrupt_init(void);

int32_t interrupt_setup(struct st_proc* proc, uint32_t interrupt, ewokos_addr_t entry, ewokos_addr_t data);

int32_t interrupt_send(context_t* ctx, uint32_t irq);

int32_t interrupt_soft_send(context_t* ctx, int32_t to_pid, ewokos_addr_t entry, ewokos_addr_t data);

void interrupt_end(context_t* ctx);

#endif
