/*
 * AArch64 process arch hooks.
 *
 * Strong overrides of the weak defaults in the common kernel/kernel/src/proc.c
 * (declared in <kernel/proc.h>). They reproduce the `__aarch64__` blocks that
 * used to live inline in the common proc.c so that file stays arch-neutral:
 *
 *   - TPIDR_EL0 is the user thread register libc's emulated TLS hangs its
 *     per-task block off; the context switch carries it explicitly because it
 *     is not part of context_t.
 *   - freshly mapped user stack pages are marked UXN (user execute-never).
 */
#include <kernel/proc.h>
#include <mm/mmu_arch.h>

ewokos_addr_t arch_proc_tls_base_read(void) {
	ewokos_addr_t v;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(v));
	return v;
}

void arch_proc_tls_base_write(ewokos_addr_t v) {
	__asm__ volatile("msr tpidr_el0, %0" :: "r"(v));
}

void arch_mark_stack_pte_noexec(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
	page_table_entry_t* pte = get_page_table_entry(vm, vaddr);
	if(pte != NULL) {
		pte->UXN = 1;
	}
}
