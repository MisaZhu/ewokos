#include <kernel/signal.h>
#include <signals.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>

int32_t  proc_signal_setup(ewokos_addr_t entry) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.entry = entry;
	return 0;	
}

void  proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no, bool quick) {
	proc_t* cproc = get_current_proc();
	ctx->gpr[0] = -1;
	if(proc == NULL || proc->space->signal.entry == 0) 
		return;
	ctx->gpr[0] = 0;
	
	proc_save_state(proc, &proc->space->signal.saved_state, &proc->space->signal.saved_ipc_res);
	proc->space->signal.sig_no = sig_no;
	proc->space->signal.do_switch = true;

	if(quick)
		proc_switch_multi_core(ctx, proc, cproc->info.core);
	else
		proc_ready(proc);
}

void proc_signal_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();

	if(cproc->info.state == UNUSED ||
			cproc->info.state == ZOMBIE ||
			cproc->space->signal.entry == 0) {
		schedule(ctx);
		return;
	}

	proc_restore_state(ctx, cproc, &cproc->space->signal.saved_state, &cproc->space->signal.saved_ipc_res);
	if(cproc->space->signal.sig_no == SYS_SIG_STOP ||
			cproc->space->signal.sig_no == SYS_SIG_KILL) {
		proc_ready(cproc);
	}
	schedule(ctx);
}

