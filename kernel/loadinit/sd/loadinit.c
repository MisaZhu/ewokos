#include <readfs.h>
#include <stddef.h>
#include <kprintf.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <dev/sd.h>
#include <kstring.h>

int32_t load_init_proc(void) {
    const char* prog = "/sbin/init";
    int32_t sz;

    char* elf = read_fs(prog, &sz);
    if(elf == NULL)
        return -1;

    /* Create the init process first */
    proc_t *proc = proc_create(TASK_TYPE_PROC, NULL);
    strcpy(proc->info.cmd, prog);
    proc->info.uid = -1;
    page_dir_entry_t *vm = proc->space->vm;
    set_translation_table_base(V2P(vm));

    /* Allocate shm and map it into the new process, then copy ELF image */
    int32_t shm_id = shm_get(0, sz, 0666);
    if(shm_id <= 0) {
        kfree(elf);
        return -1;
    }
    uint8_t* shm_buf = (uint8_t*)shm_proc_map(proc, shm_id);
    if(shm_buf == NULL) {
        shm_ctrl(shm_id, 0); /* IPC_RMID */
        kfree(elf);
        return -1;
    }
    memcpy(shm_buf, elf, sz);
    kfree(elf);

    int32_t res = proc_load_elf(proc, shm_id, sz);
    /* Note: shm is unmapped by proc_load_elf on success, or we clean up on failure */
    if(res != 0) {
        shm_proc_unmap(proc, shm_buf);
        shm_ctrl(shm_id, 0); /* IPC_RMID */
    }
    return res;
}
