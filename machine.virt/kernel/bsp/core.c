#include <kernel/core.h>
#include <kernel/system.h>

#ifdef KERNEL_SMP

extern void gic_v3_core_init(uint32_t core);

void cpu_core_ready(uint32_t core_id) {
    gic_v3_core_init(core_id);
    set_vector_table(&interrupt_table_start);
    ipi_enable(core_id);
    __irq_enable();
}

inline uint32_t get_cpu_cores(void) {
    return 4;
}

#endif