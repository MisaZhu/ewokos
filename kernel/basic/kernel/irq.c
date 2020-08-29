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
#include <kprintf.h>
#include <usinterrupt.h>

uint32_t _kernel_sec = 0;
uint64_t _kernel_usec = 0;
static uint32_t _schedule_tic = 0;
static uint32_t _timer_tic = 0;

static void keyboard_interrupt(proto_t* data) {
	kevent_t* kev = kev_push(KEV_US_INT, NULL);
	PF->addi(kev->data, US_INT_PS2_KEY);
	PF->addi(kev->data, proto_read_int(data));
}

static bool do_userspace_int(uint32_t irqs, proto_t* data) {
	if((irqs & IRQ_KEY) != 0) {
		keyboard_interrupt(data);
		return true;
	}
	return false;
}

void irq_handler(context_t* ctx) {
	__irq_disable();
	proto_t data;
	bool do_int = false;

	PF->init(&data, NULL, 0);
	uint32_t irqs = gic_get_irqs(&data);

	//handle userspace interrupt
	do_int = do_userspace_int(irqs, &data);
	PF->clear(&data);
	if(do_int && _core_pid >= 0) { 
		//if userspace interrtup event pushed, switch to core process immidiately.
		proc_switch(ctx, proc_get(_core_pid), true);
		return;
	}

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

			if(_schedule_tic >= 10000) { //10 msec, 100 times scheduling per second
				_schedule_tic = 0;
			}
			renew_sleep_counter(usec_gap);
		}
		timer_clear_interrupt(0);
		if(_schedule_tic == 0)
			schedule(ctx);
	}
}

void prefetch_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, prefetch abort!!\n");
		return;
	}

	printf("pid: %d(%s), prefetch abort!!\n", _current_proc->info.pid, _current_proc->info.cmd);
	while(1);
}

void data_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, data abort!!\n");
		return;
	}

	printf("pid: %d(%s), data abort!!\n", _current_proc->info.pid, _current_proc->info.cmd);
	proc_exit(ctx, _current_proc, -1);
	_current_proc = NULL;
	schedule(ctx);
}

void irq_init(void) {
	irq_arch_init();
	//gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	//gic_set_irqs(IRQ_TIMER0 | IRQ_SDC);
	gic_set_irqs(IRQ_TIMER0 | IRQ_KEY);
	__irq_enable();
	_kernel_sec = 0;
	_kernel_usec = 0;
	_schedule_tic = 0;
	_timer_tic = 0;
}
