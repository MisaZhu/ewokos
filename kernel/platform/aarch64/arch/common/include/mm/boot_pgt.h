#ifndef BOOT_PGT_H
#define BOOT_PGT_H

#include <mm/mmu.h>
#include <stddef.h>

typedef struct {
	page_dir_entry_t* page_dir;
	uint32_t page_table_count;
#ifdef PAGE_SIZE_16K
	page_table_entry_t (*page_tables)[PAGE_DIR_NUM];
	uint32_t next_page_table;
#else
	page_table_entry_t* page_tables;
	page_table_entry_t* next_page_table;
#endif
} boot_pgt_ctx_t;

static inline void boot_pgt_zero_mem(void* p, size_t n) {
	volatile uint8_t* cur = (volatile uint8_t*)p;
	while(n-- > 0) {
		*cur++ = 0;
	}
}

static inline void boot_pgt_set_page_flags(page_table_entry_t* pte, int is_dev) {
	pte->NSTable = 1;
	pte->EntryType = TYPE_PAGE;
	pte->AF = 1;
	pte->SH = STAGE2_SH_INNER_SHAREABLE;

	if(is_dev) {
		pte->PXN = 1;
		pte->UXN = 1;
		pte->SH = STAGE2_SH_OUTER_SHAREABLE;
		pte->MemAttr = MT_DEVICE_NGNRNE;
	}
	else {
		pte->MemAttr = MT_NORMAL;
	}
}

static inline void boot_pgt_ctx_init(boot_pgt_ctx_t* ctx) {
	boot_pgt_zero_mem(ctx->page_dir, PAGE_DIR_SIZE);
#ifdef PAGE_SIZE_16K
	ctx->next_page_table = 0;
	boot_pgt_zero_mem(ctx->page_tables, ctx->page_table_count * PAGE_TABLE_SIZE);
#else
	ctx->next_page_table = ctx->page_tables;
	boot_pgt_zero_mem(ctx->page_tables, ctx->page_table_count * PAGE_TABLE_SIZE);
#endif
}

static inline page_table_entry_t* boot_pgt_alloc_table(boot_pgt_ctx_t* ctx) {
#ifdef PAGE_SIZE_16K
	if(ctx->next_page_table >= ctx->page_table_count) {
		while(1);
	}

	return ctx->page_tables[ctx->next_page_table++];
#else
	page_table_entry_t* end = ctx->page_tables + (ctx->page_table_count * PAGE_DIR_NUM);
	if(ctx->next_page_table >= end) {
		while(1);
	}

	page_table_entry_t* entry = ctx->next_page_table;
	ctx->next_page_table += PAGE_DIR_NUM;
	return entry;
#endif
}

static inline page_table_entry_t* boot_pgt_get_next_table(boot_pgt_ctx_t* ctx, page_dir_entry_t* entry) {
	page_table_entry_t* table;

	if(entry->EntryType == 0) {
		table = boot_pgt_alloc_table(ctx);
		boot_pgt_zero_mem(table, PAGE_TABLE_SIZE);
		*entry = (page_dir_entry_t){
			.NSTable = 1,
			.EntryType = TYPE_TABLE,
			.Address = (uint64_t)table >> PAGE_SHIFT,
			.AF = 1
		};
	}
	else {
		table = (page_table_entry_t*)((uint64_t)entry->Address << PAGE_SHIFT);
	}

	return table;
}

static inline void boot_pgt_map_range(boot_pgt_ctx_t* ctx, uint64_t virt, uint64_t phy, uint32_t len, int is_dev) {
#ifdef PAGE_SIZE_16K
	uint64_t end = virt + len;

	while(virt < end) {
		uint32_t l1 = PAGE_L1_INDEX(virt);
		uint32_t l2 = PAGE_L2_INDEX(virt);
		uint32_t l3 = PAGE_L3_INDEX(virt);
		page_table_entry_t* l2_table = boot_pgt_get_next_table(ctx, &ctx->page_dir[l1]);
		page_table_entry_t* l3_table = boot_pgt_get_next_table(ctx, &l2_table[l2]);

		l3_table[l3].Address = phy >> PAGE_SHIFT;
		boot_pgt_set_page_flags(&l3_table[l3], is_dev);

		virt += PAGE_SIZE;
		phy += PAGE_SIZE;
	}
#else
	uint64_t end = ALIGN_UP(virt + len, PAGE_BLOCK_SIZE);

	while(virt < end) {
		uint32_t l1 = PAGE_L1_INDEX(virt);
		uint32_t l2 = PAGE_L2_INDEX(virt);
		page_table_entry_t* l2_table = boot_pgt_get_next_table(ctx, &ctx->page_dir[l1]);

		l2_table[l2] = (page_table_entry_t){
			.NSTable = 1,
			.EntryType = TYPE_BLOCK,
			.Address = phy >> PAGE_SHIFT,
			.AF = 1,
			.SH = STAGE2_SH_OUTER_SHAREABLE,
			.S2AP = 0,
			.MemAttr = is_dev ? MT_DEVICE_NGNRNE : MT_NORMAL,
		};

		virt += PAGE_BLOCK_SIZE;
		phy += PAGE_BLOCK_SIZE;
	}
#endif
}

#endif
