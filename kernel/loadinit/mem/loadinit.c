#include <base16.h>
#include <kernel/proc.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <stddef.h>

extern const char* init_data[];
extern const int init_size;

int32_t load_init(void) {
	if(init_size <= 0)
		return -1;

	char* elf = (char*)kmalloc(init_size);
	if(elf == NULL) {
		return -1;
	}

	uint32_t i = 0;
	char* p = elf;
	while(1) {
		const char* s = init_data[i];
		if(s == NULL)
			break;
		uint32_t sz = 0;
		b16_decode(s, strlen(s), p , &sz);
		p += sz;
		i++;
	}

	proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
	strcpy(proc->cmd, "/sbin/init");
	int32_t res = proc_load_elf(proc, elf, init_size);
	kfree(elf);
	return res;
}
