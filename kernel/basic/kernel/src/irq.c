#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/kevqueue.h>
#include <string.h>
#include <signals.h>
#include <kernel/signal.h>
#include <kprintf.h>

uint32_t _kernel_sec = 0;
uint64_t _kernel_usec = 0;
static uint32_t _schedule_tic = 0;
static uint32_t _timer_tic = 0;

void irq_handler(context_t* ctx) {
	__irq_disable();
	uint32_t irqs = gic_get_irqs();
	//handle irq
	if((irqs & IRQ_TIMER0) != 0) {
		uint64_t usec = timer_read_sys_usec();
		if(_kernel_usec == 0) {
			_kernel_usec = usec;
		}
		else {
			uint64_t usec_gap = usec - _kernel_usec;
			_kernel_usec = usec;
			_schedule_tic += usec_gap;
			_timer_tic += usec_gap;
			if(_timer_tic >= 1000000) { //1 sec
				_kernel_sec++;
				_timer_tic = 0;
			}

			if(_schedule_tic >= 20000) { //20 msec, 50 times scheduling per second
				_schedule_tic = 0;
			}
			renew_sleep_counter(usec_gap);
		}
		timer_clear_interrupt(0);
		if(_schedule_tic == 0)
			schedule(ctx);
	}
}

static void dump_ctx(context_t* ctx) {
	printf("ctx dump:\n"
		"  cpsr=0x%x\n"
		"  pc=0x%x\n"
		"  sp=0x%x\n"
		"  lr=0x%x\n",
		ctx->cpsr,
		ctx->pc,
		ctx->sp,
		ctx->lr);
	uint32_t i;
	for(i=0; i<13; i++) 
		printf("  r%d: 0x%x\n", i, ctx->gpr[i]);
}

void prefetch_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, prefetch abort!!\n");
		dump_ctx(ctx);
		while(1);
	}

	printf("pid: %d(%s), prefetch abort!!\n", _current_proc->info.pid, _current_proc->info.cmd);
	dump_ctx(ctx);
	proc_signal_send(ctx, _current_proc, SYS_SIG_STOP);
	//_current_proc = NULL;
	//schedule(ctx);
}

void data_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, data abort!!\n");
		dump_ctx(ctx);
		while(1);
	}

	printf("pid: %d(%s), data abort!!\n", _current_proc->info.pid, _current_proc->info.cmd);
	dump_ctx(ctx);
	//proc_exit(ctx, _current_proc, -1);
	proc_signal_send(ctx, _current_proc, SYS_SIG_STOP);
	//_current_proc = NULL;
	//schedule(ctx);
}

void irq_init(void) {
	irq_arch_init();
	_kernel_sec = 0;
	_kernel_usec = 0;
	_schedule_tic = 0;
	_timer_tic = 0;
	//gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	gic_set_irqs(IRQ_TIMER0);
}
