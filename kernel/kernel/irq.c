#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart.h>
#include <dev/kdevice.h>
#include <dev/sd.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/uspace_int.h>
#include <kernel/proc.h>
#include <string.h>
#include <kprintf.h>

/*static void sd_handler(void) {
	dev_t* dev = get_dev(DEV_SD);
	sd_dev_handle(dev);
}
*/

uint32_t _kernel_tic = 0;
static uint64_t _timer_usec = 0;
static uint32_t _timer_tic = 0;

void irq_handler(context_t* ctx) {
	__irq_disable();

	uint32_t irqs = gic_get_irqs();

	/*
	if((irqs & IRQ_SDC) != 0) {
		sd_handler();
	}
	*/
	if((irqs & IRQ_TIMER0) != 0) {
		uint64_t usec = timer_read_sys_usec();
		if(_current_proc == NULL || _current_proc->critical_counter == 0) {
			if(_timer_usec == 0)
				_timer_usec = usec;
			else {
				uint64_t usec_gap = usec - _timer_usec;
				_timer_usec = usec;
				_timer_tic += usec_gap;
				if(_timer_tic >= 1000000) { //1 sec
					_kernel_tic++;
					_timer_tic = 0;
				}
				renew_sleep_counter(usec_gap);
			}
		}	
		timer_clear_interrupt(0);

		if(_current_proc != NULL && _current_proc->critical_counter > 0) {
			_current_proc->critical_counter--;
		}
		else
			schedule(ctx);
		return;
	}
}

void prefetch_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, prefetch abort!!\n");
		return;
	}

	printf("pid: %d(%s), prefetch abort!!\n", _current_proc->pid, CS(_current_proc->cmd));
	while(1);
}

void data_abort_handler(context_t* ctx) {
	(void)ctx;
	if(_current_proc == NULL) {
		printf("_kernel, data abort!!\n");
		return;
	}

	printf("pid: %d(%s), data abort!!\n", _current_proc->pid, CS(_current_proc->cmd));
	proc_exit(ctx, _current_proc, -1);
	_current_proc = NULL;
	schedule(ctx);
}

void irq_init(void) {
	irq_arch_init();
	uspace_interrupt_init();
	//gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	//gic_set_irqs(IRQ_TIMER0 | IRQ_SDC);
	gic_set_irqs(IRQ_TIMER0);
	__irq_enable();
	_kernel_tic = 0;
	_timer_usec = 0;
	_timer_tic = 0;
}
