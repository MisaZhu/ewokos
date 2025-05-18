#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/system.h>
#include <kernel/kernel.h>
#include <stdio.h>

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 * Notice: virtual and physical address inputed must be all aliend by PAGE_SIZE !
 */
int32_t map_page(page_dir_entry_t *vm, uint32_t virtual_addr,
		     uint32_t physical, uint32_t permissions, uint32_t pte_attr) {
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;

	uint32_t l1_index = PAGE_L1_INDEX(virtual_addr);
	uint32_t l2_index = PAGE_L2_INDEX(virtual_addr);
	uint32_t l3_index = PAGE_L3_INDEX(virtual_addr);
	/* if this page_dirEntry is not mapped before, map it to a new page table */
	if (vm[l1_index].EntryType == 0) {
		l2_table = kalloc4k();
		if(l2_table == NULL)
			return -1;
		memset(l2_table, 0, PAGE_TABLE_SIZE);
		vm[l1_index].NSTable = 1;
		vm[l1_index].EntryType = TYPE_TABLE;
		vm[l1_index].Address = (uint64_t)V2P(l2_table) >> 12;	
		vm[l1_index].AF = 1;
		// vm[l1_index].SH = STAGE2_SH_INNER_SHAREABLE;
		// vm[l1_index].MemAttr = MT_NORMAL;
	}else{
		l2_table = P2V((page_dir_entry_t*)(vm[l1_index].Address << 12)); 
	}

	if(l2_table[l2_index].EntryType == 0){
		l3_table = kalloc4k();	
		if(l3_table == NULL)
			return -1;	
		memset(l3_table, 0, PAGE_TABLE_SIZE);
		l2_table[l2_index].NSTable = 1;
		l2_table[l2_index].EntryType = TYPE_TABLE;
		l2_table[l2_index].Address = (uint64_t)V2P(l3_table) >> 12;	
		l2_table[l2_index].AF = 1;
		// l2_table[l2_index].SH = STAGE2_SH_INNER_SHAREABLE;
		// l2_table[l2_index].MemAttr = MT_NORMAL;
	}else{
		l3_table = P2V((page_dir_entry_t*)(l2_table[l2_index].Address << 12)); 	
	}

	l3_table[l3_index].S2AP = permissions;
	l3_table[l3_index].Address = (uint64_t)physical >> 12; 

	set_pte_flags(&l3_table[l3_index], pte_attr);
	return 0;
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr) {
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;

	uint32_t l1_index = PAGE_L1_INDEX(virtual_addr);
	uint32_t l2_index = PAGE_L2_INDEX(virtual_addr);
	uint32_t l3_index = PAGE_L3_INDEX(virtual_addr);

	if(vm[l1_index].EntryType != 0){
		l2_table = (page_dir_entry_t*)P2V(vm[l1_index].Address << 12);
		if(l2_table[l2_index].EntryType != 0){
			l3_table = (page_dir_entry_t*)P2V(l2_table[l2_index].Address << 12);
			l3_table[l3_index].EntryType = 0;	
		}
	}
}

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual) {
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;
	uint32_t l1_index = PAGE_L1_INDEX(virtual);
	uint32_t l2_index = PAGE_L2_INDEX(virtual);
	uint32_t l3_index = PAGE_L3_INDEX(virtual);

	l2_table = (page_table_entry_t*)P2V(vm[l1_index].Address << 12); 
	l3_table = (page_table_entry_t*)P2V(l2_table[l2_index].Address << 12); 
	uint32_t phy = (l3_table[l3_index].Address << 12) | (virtual & 0xFFF);	
	return phy;
}

/*
get page entry(virtual addr) by virtual address
*/
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual) {
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;

	uint32_t l1_index = PAGE_L1_INDEX(virtual);
	uint32_t l2_index = PAGE_L2_INDEX(virtual);
	uint32_t l3_index = PAGE_L3_INDEX(virtual);

	l2_table = (page_table_entry_t*)P2V(vm[l1_index].Address << 12); 
	return (page_table_entry_t*)P2V(l2_table[l2_index].Address << 12); 	
}


void free_page_tables(page_dir_entry_t *vm) {
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;
	int i,j;

	for(int i = 0; i < PAGE_DIR_NUM; i++){
		if(vm[i].EntryType != 0){
			l2_table = (page_table_entry_t*)P2V(vm[i].Address << 12); 
			for(int j = 0; j < PAGE_DIR_NUM; j++){
				if(l2_table[j].EntryType != 0){
					l3_table = (page_table_entry_t*)P2V(l2_table[j].Address << 12);
					kfree4k(l3_table);
				}
			}
			kfree4k(l2_table);
		}
	}
}

void dump_page_tables(page_dir_entry_t *vm){
	page_table_entry_t *l2_table = 0;
	page_table_entry_t *l3_table = 0;
	int i,j;
	printf("\n");
	for(int i = 0; i < 4; i++){
		if(vm[i].NSTable){
			l2_table = (page_table_entry_t*)P2V(vm[i].Address << 12); 
			printf("%08x:%016x\n", i<<30, *(uint64_t*)&vm[i]);
			for(int j = 0; j < PAGE_DIR_NUM; j++){
				if(l2_table[j].EntryType == TYPE_BLOCK){
					printf("\t%08x:%16x\n", (i << 30) + (j << 21), *(uint64_t*)&l2_table[j]);	
				}
				else if(l2_table[j].NSTable){
					l3_table = (page_table_entry_t*)P2V(l2_table[j].Address << 12);
					printf("\t%08x:%016x\n", (i << 30) + (j<<21),  *(uint64_t*)&l2_table[j]);
					uint64_t chunk_start = 0;
					for(int k = 0; k < PAGE_DIR_NUM; k++)
						if((uint64_t)l3_table[k].AF){
							printf("\t\t%08x:%016x\n", (i << 30) + (j << 21) + (k << 12), *(uint64_t*)&l3_table[k]);
						}
				}
			}
		}
	}
}

inline void clear_cache(void *start, void *end) {
    uintptr_t addr = (uintptr_t)start;
    uintptr_t end_addr = (uintptr_t)end;
    
    // 对齐到缓存行（通常 64 字节）
    addr &= ~(64 - 1);

    // 使用 DC CIVAC 刷新数据缓存
    for (; addr < end_addr; addr += 64) {
        __asm volatile("dc civac, %0" :: "r"(addr));
    }

    // 确保指令缓存同步（IC IALLU）
    __asm volatile("ic iallu");
    __asm volatile("dsb sy");  // 数据同步屏障
    __asm volatile("isb");      // 指令同步屏障
}