#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/system.h>
#include <kernel/kernel.h>

static inline page_table_entry_t* entry_to_table(page_table_entry_t* entry) {
	return (page_table_entry_t*)P2V((ewokos_addr_t)(entry->Address << 12));
}

static inline int shared_kernel_pdpt(uint32_t pml4_index, uint32_t pdpt_index) {
	if (pml4_index != PAGE_PML4_INDEX(KERNEL_BASE)) {
		return 0;
	}

	/*
	 * clone_kernel_vm() reuses the kernel high-half PDPT entries instead of
	 * copying them, so they must not be released during per-process teardown.
	 */
	return pdpt_index >= PAGE_PDPT_INDEX(KERNEL_BASE);
}

static page_table_entry_t* next_table(page_table_entry_t* table, uint32_t index, int create) {
	if (!table[index].present) {
		page_table_entry_t* next;
		if (!create) {
			return NULL;
		}
		next = kalloc4k();
		if (next == NULL) {
			return NULL;
		}
		memset(next, 0, PAGE_TABLE_SIZE);
		table[index].value = 0;
		table[index].present = 1;
		table[index].rw = 1;
		table[index].us = 1;
		table[index].Address = (uint64_t)V2P(next) >> 12;
	}
	return entry_to_table(&table[index]);
}

int32_t map_page(page_dir_entry_t* vm, uint32_t virtual_addr,
		uint32_t physical, uint32_t permissions, uint32_t pte_attr) {
	page_table_entry_t* pdpt;
	page_table_entry_t* pd;
	page_table_entry_t* pt;
	page_table_entry_t* pte;

	if (((virtual_addr | physical) & (PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	pdpt = next_table(vm, PAGE_PML4_INDEX(virtual_addr), 1);
	if (pdpt == NULL) {
		return -1;
	}
	pd = next_table(pdpt, PAGE_PDPT_INDEX(virtual_addr), 1);
	if (pd == NULL) {
		return -1;
	}
	pt = next_table(pd, PAGE_PD_INDEX(virtual_addr), 1);
	if (pt == NULL) {
		return -1;
	}

	pte = &pt[PAGE_PT_INDEX(virtual_addr)];
	pte->value = 0;
	pte->present = 1;
	pte->rw = (permissions != AP_RW_R);
	pte->us = (permissions != AP_RW_D);
	pte->Address = (uint64_t)physical >> 12;
	set_pte_flags(pte, pte_attr);
	return 0;
}

void unmap_page(page_dir_entry_t* vm, uint32_t virtual_addr) {
	page_table_entry_t* pdpt = next_table(vm, PAGE_PML4_INDEX(virtual_addr), 0);
	page_table_entry_t* pd;
	page_table_entry_t* pt;

	if (pdpt == NULL) {
		return;
	}
	pd = next_table(pdpt, PAGE_PDPT_INDEX(virtual_addr), 0);
	if (pd == NULL) {
		return;
	}
	pt = next_table(pd, PAGE_PD_INDEX(virtual_addr), 0);
	if (pt == NULL) {
		return;
	}
	pt[PAGE_PT_INDEX(virtual_addr)].value = 0;
}

uint32_t resolve_phy_address(page_dir_entry_t* vm, uint32_t virtual_addr) {
	page_table_entry_t* pte = get_page_table_entry(vm, virtual_addr);
	if (pte == NULL || !pte->present) {
		return 0;
	}
	return ((uint32_t)(pte->Address << 12)) | (virtual_addr & (PAGE_SIZE - 1));
}

page_table_entry_t* get_page_table_entry(page_dir_entry_t* vm, uint32_t virtual_addr) {
	page_table_entry_t* pdpt = next_table(vm, PAGE_PML4_INDEX(virtual_addr), 0);
	page_table_entry_t* pd;
	page_table_entry_t* pt;

	if (pdpt == NULL) {
		return NULL;
	}
	pd = next_table(pdpt, PAGE_PDPT_INDEX(virtual_addr), 0);
	if (pd == NULL) {
		return NULL;
	}
	pt = next_table(pd, PAGE_PD_INDEX(virtual_addr), 0);
	if (pt == NULL) {
		return NULL;
	}
	return &pt[PAGE_PT_INDEX(virtual_addr)];
}

void free_page_tables(page_dir_entry_t* vm) {
	for (uint32_t i = 0; i < PAGE_DIR_NUM; ++i) {
		page_table_entry_t* pdpt;
		if (!vm[i].present) {
			continue;
		}
		pdpt = entry_to_table(&vm[i]);
		for (uint32_t j = 0; j < PAGE_DIR_NUM; ++j) {
			page_table_entry_t* pd;
			if (!pdpt[j].present) {
				continue;
			}
			if (shared_kernel_pdpt(i, j)) {
				continue;
			}
			pd = entry_to_table(&pdpt[j]);
			for (uint32_t k = 0; k < PAGE_DIR_NUM; ++k) {
				page_table_entry_t* pt;
				if (!pd[k].present) {
					continue;
				}
				pt = entry_to_table(&pd[k]);
				kfree4k(pt);
			}
			kfree4k(pd);
		}
		kfree4k(pdpt);
	}
}
