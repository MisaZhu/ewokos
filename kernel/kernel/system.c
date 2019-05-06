#include <system.h>
#include <types.h>
#include <mm/kalloc.h>
#include <proc.h>
#include <kstring.h>
#include <hardware.h>
#include <timer.h>

inline void loopd(uint32_t times) {
	while(times > 0)
		times--;
}

static int32_t kill_proc(int32_t pid) {
	process_t* proc = proc_get(pid);
	if(proc == NULL)
		return -1;

	if(_current_proc->owner != proc->owner && _current_proc->owner > 0)
		return -1;

	proc_exit(proc);
	return 0;
}

int32_t system_cmd(int32_t cmd, int32_t arg0, int32_t arg1) {
	int32_t ret = -1;
	switch(cmd) {
	case 0: //get total mem size
		ret = (int32_t)get_phy_ram_size();
		break;
	case 1: //get free mem size
		ret = (int32_t)get_free_mem_size();
		break;
	case 2: //get procs
		ret = (int32_t)get_procs((int32_t*)arg0);
		break;
	case 3: //kill proc
		ret = (int32_t)kill_proc(arg0);
		break;
	case 4: //get cpu tick
		cpu_tick((uint32_t*)arg0, (uint32_t*)arg1);
		ret = 0;
		break;
	default:
		break;
	}
	return ret;
}
