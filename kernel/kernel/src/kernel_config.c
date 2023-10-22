#include <kernel/kernel.h>
#include <kernel/core.h>
#include <stddef.h>
#include <sconf.h>
#include <atoi.h>

kernel_conf_t _kernel_config;

static void read_kernel_config(void) {
	memset(&_kernel_config, 0, sizeof(kernel_conf_t));

	sconf_t* sconf = sconf_load("/etc/kernel/kernel.conf");
	if(sconf == NULL)
		return;
	const char* v = sconf_get(sconf, "cores");
	if(v[0] != 0)
		_kernel_config.cores = atoi(v);

	v = sconf_get(sconf, "timer_freq");
	if(v[0] != 0)
		_kernel_config.timer_freq = atoi(v);
	sconf_free(sconf);
}

void load_kernel_config(void) {
	read_kernel_config();

	uint32_t cores_max = get_cpu_cores();
	if(_kernel_config.cores == 0 || _kernel_config.cores > cores_max)
		_kernel_config.cores = cores_max; 
	_sys_info.cores = _kernel_config.cores;

	if(_kernel_config.timer_freq < MIN_SCHD_FREQ)
		_kernel_config.timer_freq = MIN_SCHD_FREQ;
}
