#include <dev/timer.h>
#include <kernel/kernel.h>
#include <kernel/trace.h>
#include <kernel/core.h>
#include <kstring.h>
#include <kprintf.h>
#include <mm/kalloc.h>
#include <stddef.h>

#ifdef SCHD_TRACE

static uint32_t _traces_num = 0; 

#define MAX_SCHD_TRACE_NUM 256
static trace_t _traces[MAX_CORE_NUM][MAX_SCHD_TRACE_NUM];
static bool _paused = false;


void pause_trace(void) {
	_paused = true;
}

void resume_trace(void) {
	_paused = false;
}

uint32_t get_trace(trace_t *traces) {
	for(uint32_t core=0; core<_sys_info.cores; core++) {
		for(uint32_t trace=0; trace<_traces_num; trace++) {
			memcpy(&traces[core*MAX_SCHD_TRACE_NUM + trace], &_traces[core][trace], sizeof(trace_t));
		}
	}

	uint32_t ret = _traces_num;
	if(!_paused)
		_traces_num = 0;
	return ret;
}

uint32_t get_trace_fps() {
	return MAX_SCHD_TRACE_NUM;
}

void update_trace(uint32_t usec_gap) {
	if(_paused)
		return;

	static uint32_t usec_trace = 0;
	usec_trace += usec_gap;

	uint32_t fps = get_trace_fps();
	if(fps > _kernel_config.timer_freq)
		fps = _kernel_config.timer_freq;

	if(usec_trace < (1000000 / fps))
		return;
	usec_trace = 0;

	if(_traces_num >= fps) {
		return;
	}

	for(uint32_t core=0; core<_sys_info.cores; core++) {
		proc_t* proc = get_current_core_proc(core);
		if(proc == NULL || _cpu_cores[core].halt_proc == proc) {
			_traces[core][_traces_num].pid = -1;
		}
		else {
			_traces[core][_traces_num].pid = proc->info.pid;
			memcpy(&_traces[core][_traces_num].ctx, &proc->ctx, sizeof(context_t));
		}
	}
	_traces_num++;
}

#endif
