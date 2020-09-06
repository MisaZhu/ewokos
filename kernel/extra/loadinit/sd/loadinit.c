#include <ext2read.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kprintf.h>

int32_t load_init_sdc(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	printf("  read %s from sdc\n", prog);
	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
		strcpy(proc->info.cmd, prog);
  	set_translation_table_base((uint32_t)V2P(proc->space->vm));
		int32_t res = proc_load_elf(proc, elf, sz);
  	set_translation_table_base((uint32_t)V2P(_kernel_vm));
		kfree(elf);
		return res;
	}
	return -1;
}
