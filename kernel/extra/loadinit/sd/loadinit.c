#include <ext2read.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kprintf.h>
#include <dev/sd.h>

int32_t load_init_sdc(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	printf("kernel: sd card initing.\n");
  printf("  %16s ", "mmc_sd");
  if(sd_init() != 0)  {
    printf("[Failed!]\n");
		return -1;
	}
  printf("[ok]\n  read %s from sdc\n", prog);
	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
		strcpy(proc->info.cmd, prog);
  	//set_translation_table_base((uint32_t)V2P(proc->space->vm));
		int32_t res = proc_load_elf(proc, elf, sz);
  	//set_translation_table_base((uint32_t)V2P(_kernel_vm));
		kfree(elf);
		return res;
	}
	return -1;
}
