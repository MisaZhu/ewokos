#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kernel/core.h>
#include <stddef.h>
#include <sconf.h>
#include <atoi.h>

kernel_conf_t _kernel_config;

void load_kernel_config(void) {
	uint32_t cores_max = get_cpu_cores();
	_kernel_config.cores = cores_max; 
	_kernel_config.schedule_freq = SCHEDULE_FREQ_DEF;
	_kernel_config.timer_freq = _kernel_config.schedule_freq*2;
	_kernel_config.timer_intr_usec = 0;
	_kernel_config.uart_baud = 115200;
	_kernel_config.max_task_per_proc = MAX_TASK_PER_PROC_DEF;
	_kernel_config.max_task_num = MAX_TASK_NUM_DEF;
	_kernel_config.max_proc_num = MAX_PROC_NUM_DEF;

	sconf_t* sconf = sconf_load("/etc/kernel/kernel.conf");
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "cores");
	if(v[0] != 0)
		_kernel_config.cores = atoi(v);
	if(_kernel_config.cores == 0 || _kernel_config.cores > cores_max)
		_kernel_config.cores = cores_max;

	v = sconf_get(sconf, "timer_freq");
	if(v[0] != 0)
		_kernel_config.timer_freq = atoi(v);

	v = sconf_get(sconf, "schedule_freq");
	if(v[0] != 0)
		_kernel_config.schedule_freq = atoi(v);
	if(_kernel_config.schedule_freq == 0)
		_kernel_config.schedule_freq = SCHEDULE_FREQ_DEF;
	if(_kernel_config.timer_freq < _kernel_config.schedule_freq*2)
		_kernel_config.timer_freq = _kernel_config.schedule_freq*2;

	uint32_t proc_num_limit = (KMALLOC_SIZE) / (PAGE_DIR_SIZE) / 2;
	v = sconf_get(sconf, "max_proc_num");
	if(v[0] != 0)
		_kernel_config.max_proc_num = atoi(v);
	if(_kernel_config.max_proc_num < MAX_PROC_NUM_DEF)
		_kernel_config.max_proc_num = MAX_PROC_NUM_DEF;
	if(_kernel_config.max_proc_num > proc_num_limit)
		_kernel_config.max_proc_num = proc_num_limit;

	v = sconf_get(sconf, "max_task_num");
	if(v[0] != 0)
		_kernel_config.max_task_num = atoi(v);
	if(_kernel_config.max_task_num < _kernel_config.max_proc_num)
		_kernel_config.max_task_num = _kernel_config.max_proc_num;

	v = sconf_get(sconf, "max_task_per_proc");
	if(v[0] != 0)
		_kernel_config.max_task_per_proc = atoi(v);
	if(_kernel_config.max_task_per_proc < MAX_TASK_PER_PROC_DEF)
		_kernel_config.max_task_per_proc = MAX_TASK_PER_PROC_DEF;

	v = sconf_get(sconf, "uart_baud");
	if(v[0] != 0)
		_kernel_config.uart_baud = atoi(v);
	if(_kernel_config.uart_baud == 0)
		_kernel_config.uart_baud = 115200;

	sconf_free(sconf);
}
