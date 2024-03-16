#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kernel/core.h>
#include <stddef.h>
#include <sconf.h>
#include <atoi.h>

kernel_conf_t _kernel_config;

static void load_kernel_config_file() {
	sconf_t* sconf = sconf_load("/etc/kernel/kernel.conf");
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "cores");
	if(v[0] != 0)
		_kernel_config.cores = atoi(v);

	v = sconf_get(sconf, "timer_freq");
	if(v[0] != 0)
		_kernel_config.timer_freq = atoi(v);

	v = sconf_get(sconf, "schedule_freq");
	if(v[0] != 0)
		_kernel_config.schedule_freq = atoi(v);

	v = sconf_get(sconf, "max_proc_num");
	if(v[0] != 0)
		_kernel_config.max_proc_num = atoi(v);

	v = sconf_get(sconf, "max_task_num");
	if(v[0] != 0)
		_kernel_config.max_task_num = atoi(v);

	v = sconf_get(sconf, "max_task_per_proc");
	if(v[0] != 0)
		_kernel_config.max_task_per_proc = atoi(v);

	v = sconf_get(sconf, "uart_baud");
	if(v[0] != 0)
		_kernel_config.uart_baud = atoi(v);

	sconf_free(sconf);
}

void load_kernel_config(void) {
	load_kernel_config_file();

	uint32_t cores_max = get_cpu_cores();
	if(_kernel_config.cores == 0 || _kernel_config.cores > cores_max)
		_kernel_config.cores = cores_max;

	if(_kernel_config.schedule_freq == 0)
		_kernel_config.schedule_freq = SCHEDULE_FREQ_DEF;
	if(_kernel_config.timer_freq < _kernel_config.schedule_freq*2)
		_kernel_config.timer_freq = _kernel_config.schedule_freq*2;

	if(_kernel_config.max_proc_num < MAX_PROC_NUM_DEF)
		_kernel_config.max_proc_num = MAX_PROC_NUM_DEF;

	if(_kernel_config.max_task_num < _kernel_config.max_proc_num*4)
		_kernel_config.max_task_num = _kernel_config.max_proc_num*4;

	if(_kernel_config.max_task_per_proc < MAX_TASK_PER_PROC_DEF)
		_kernel_config.max_task_per_proc = MAX_TASK_PER_PROC_DEF;

	if(_kernel_config.uart_baud == 0)
		_kernel_config.uart_baud = 115200;

	_kernel_config.kmalloc_size = MIN_KMALLOC_SIZE + 
			_kernel_config.max_proc_num * (PAGE_DIR_SIZE) +
			_kernel_config.max_task_num*sizeof(proc_t);
}
