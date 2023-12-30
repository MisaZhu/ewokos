#include "ext2read.h"
#include <stddef.h>
#include <kprintf.h>
#include <kernel/proc.h>
#include <mm/kmalloc.h>
#include <dev/sd.h>
#include <kstring.h>

int32_t load_init_proc(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	printf("  load /sbin/init from sdc .... ");
	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
		strcpy(proc->info.cmd, prog);
		proc->info.uid = -1;
		int32_t res = proc_load_elf(proc, elf, sz);
		kfree(elf);
		printf("[ok]\n");
		return res;
	}
	printf("[failed]!\n");
	return -1;
}
