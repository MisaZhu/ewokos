#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <mm/mmudef.h>
#include <kernel/core.h>
#include <stddef.h>
#include <sconf.h>
#include <kstring.h>
#include <atoi.h>
#include <kprintf.h>

kernel_conf_t _kernel_config;

/* 64-bit magnitude parse: decimal, or 0x-prefixed hex (physical addresses
   above 4GB do not fit the int32_t atoi()). */
static ewokos_addr_t mem_atou(const char* s) {
    ewokos_addr_t v = 0;
    if(s == NULL)
        return 0;
    if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        for(uint32_t i = 2; s[i] != 0; i++) {
            char c = s[i];
            uint32_t d;
            if(c >= '0' && c <= '9') d = c - '0';
            else if(c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if(c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else return 0;
            v = v * 16 + d;
        }
        return v;
    }
    for(uint32_t i = 0; s[i] != 0; i++) {
        if(s[i] < '0' || s[i] > '9')
            return 0;
        v = v * 10 + (s[i] - '0');
    }
    return v;
}

static ewokos_addr_t mem_size(const char* v) {
    if(v == NULL)
        return 0;

    uint32_t len = strlen(v);
    if(len == 0)
        return 0;

    char unit = v[len-1];
    ewokos_addr_t size;
    if(unit == 'k' || unit == 'K' || unit == 'm' || unit == 'M' ||
            unit == 'g' || unit == 'G') {
        char s[16] = {0};
        sstrncpy(s, v, len-1);
        size = mem_atou(s);
        if(unit == 'k' || unit == 'K')
            size *= KB;
        else if(unit == 'm' || unit == 'M')
            size *= MB;
        else
            size *= GB;
    }
    else {
        size = mem_atou(v);
    }
    return size;
}

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

    v = sconf_get(sconf, "timer_freq");
    if(v[0] != 0)
        _kernel_config.timer_freq = atoi(v);

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

    v = sconf_get(sconf, "kmalloc_size");
    if(v[0] != 0)
        _kernel_config.kmalloc_size = mem_size(v);

    v = sconf_get(sconf, "dma_base");
    if(v[0] != 0)
        _kernel_config.dma_base = mem_size(v);

    v = sconf_get(sconf, "dma_size");
    if(v[0] != 0)
        _kernel_config.dma_size = mem_size(v);

    sconf_free(sconf);
}

void load_kernel_config(void) {
    memset(&_kernel_config, 0, sizeof(kernel_conf_t));
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
