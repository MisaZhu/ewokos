#include <dev/timer.h>
#include <kernel/kernel.h>
#include <kernel/core.h>
#include <kstring.h>
#include <kprintf.h>
#include <mm/kalloc.h>
#include <stddef.h>

#ifdef SCHD_TRACE

static uint32_t _traces = 0; 

#define MAX_SCHD_TRACE_NUM 256
static int _pids[MAX_CORE_NUM][MAX_SCHD_TRACE_NUM];

uint32_t get_trace(int *pids) {
	for(uint32_t core=0; core<_sys_info.cores; core++) {
		for(uint32_t trace=0; trace<_traces; trace++) {
			pids[core*MAX_SCHD_TRACE_NUM + trace] = _pids[core][trace];
		}
	}

	uint32_t ret = _traces;
	_traces = 0;
	return ret;
}

uint32_t get_trace_fps() {
	return MAX_SCHD_TRACE_NUM;
}

void update_trace(uint32_t usec_gap) {
	static uint32_t usec_trace = 0;
	usec_trace += usec_gap;

	uint32_t fps = get_trace_fps();
	if(fps > _kernel_config.timer_freq)
		fps = _kernel_config.timer_freq;

	if(usec_trace < (1000000 / fps))
		return;
	usec_trace = 0;

	if(_traces >= fps) {
		return;
	}

	for(uint32_t core=0; core<_sys_info.cores; core++) {
		proc_t* proc = get_current_core_proc(core);
		if(proc == NULL || _cpu_cores[core].halt_proc == proc) {
			_pids[core][_traces] = -1;
		}
		else {
			_pids[core][_traces] = proc->info.pid;
		}
	}
	_traces++;
}

#endif
