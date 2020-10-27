#include <mm/kmalloc.h>
#include <kernel/system.h>
#include <kprintf.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include "romfs.h"

int32_t load_init_proc(void) {
	romfs_load();
	const char* prog = "/sbin/init";

	printf("  read %s from kernel-romfs\n", prog);
  int32_t init_size;
  char* elf = romfs_get_by_name(prog, &init_size);
  if(elf == NULL) {
    return -1;
  }

  proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
	proc->info.owner = -1;
  strcpy(proc->info.cmd, prog);
  set_translation_table_base((uint32_t)V2P(proc->space->vm));
  int32_t res = proc_load_elf(proc, elf, init_size);
  set_translation_table_base((uint32_t)V2P(_kernel_vm));
  kfree(elf);
  return res;
}

