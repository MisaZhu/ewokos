#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kernel/core.h>
#include <stddef.h>
#include <sconf.h>
#include <kstring.h>
#include <atoi.h>

kernel_conf_t _kernel_config;

static void load_kernel_config_file() {
	sconf_t* sconf = sconf_load("/etc/kernel/kernel.conf");
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "machine");
	if(v[0] != 0)
		sstrncpy(_sys_info.machine, v, MACHINE_MAX-1);

	v = sconf_get(sconf, "cores");
	if(v[0] != 0)
		_kernel_config.cores = atoi(v);

	v = sconf_get(sconf, "allocable_base_reserv_size");
	if(v[0] != 0)
		_allocable_phy_mem_base += ALIGN_UP(atoi(v), PAGE_SIZE);
	
	v = sconf_get(sconf, "allocable_top_reserv_size");
	if(v[0] != 0)
		_allocable_phy_mem_top -= ALIGN_UP(atoi(v), PAGE_SIZE);

	v = sconf_get(sconf, "timer_freq");
	if(v[0] != 0)
		_kernel_config.timer_freq = atoi(v);

	v = sconf_get(sconf, "max_proc_num");
	if(v[0] != 0)
		_kernel_config.max_proc_num = atoi(v);
	

/*
	v = sconf_get(sconf, "max_task_num");
	if(v[0] != 0)
		_kernel_config.max_task_num = atoi(v);
		*/

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

	if(_kernel_config.timer_freq < 256)
		_kernel_config.timer_freq = 256;

	if(_kernel_config.max_proc_num > MAX_PROC_NUM)
		_kernel_config.max_proc_num = MAX_PROC_NUM;

	if(_kernel_config.max_proc_num < MAX_PROC_NUM_DEF)
		_kernel_config.max_proc_num = MAX_PROC_NUM_DEF;

	if(_kernel_config.max_task_num < _kernel_config.max_proc_num*4)
		_kernel_config.max_task_num = _kernel_config.max_proc_num*4;

	if(_kernel_config.max_task_per_proc < MAX_TASK_PER_PROC_DEF)
		_kernel_config.max_task_per_proc = MAX_TASK_PER_PROC_DEF;

	if(_kernel_config.uart_baud == 0)
		_kernel_config.uart_baud = 115200;
}
