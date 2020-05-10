#include <ext2read.h>
#include <kernel/proc.h>
#include <mm/kmalloc.h>
#include <kstring.h>

int32_t load_init(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
		strcpy(proc->cmd, prog);
		int32_t res = proc_load_elf(proc, elf, sz);
		kfree(elf);
		return res;
	}
	return -1;
}
