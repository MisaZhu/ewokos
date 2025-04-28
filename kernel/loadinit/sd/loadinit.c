#include "ext2read.h"
#include <stddef.h>
#include <kprintf.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <mm/kmalloc.h>
#include <dev/sd.h>
#include <kstring.h>

int32_t load_init_proc(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(TASK_TYPE_PROC, NULL);
		strcpy(proc->info.cmd, prog);
		proc->info.uid = -1;
		page_dir_entry_t *vm = proc->space->vm;
		set_translation_table_base((uint32_t)V2P(vm));
		int32_t res = proc_load_elf(proc, elf, sz);
		kfree(elf);
		return res;
	}
	return -1;
}
