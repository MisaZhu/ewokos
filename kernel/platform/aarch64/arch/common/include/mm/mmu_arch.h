#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include <ewokos_config.h>
#include "mmudef.h"

#define PAGE_INDEX_MASK ((uint64_t)(PAGE_DIR_NUM - 1))

#define PAGE_ROOT_INDEX(x) (((uint64_t)(x) >> PAGE_ROOT_SHIFT) & PAGE_INDEX_MASK)
#define PAGE_L1_INDEX(x) (((uint64_t)(x) >> PAGE_L1_SHIFT) & PAGE_INDEX_MASK)
#define PAGE_L2_INDEX(x) (((uint64_t)(x) >> PAGE_L2_SHIFT) & PAGE_INDEX_MASK)
#define PAGE_L3_INDEX(x) (((uint64_t)(x) >> PAGE_L3_SHIFT) & PAGE_INDEX_MASK)

#define PAGE_TABLE_TO_BASE(x) ((uint64_t)(x) >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *)((uint64_t)(x) << 10))
#define PAGE_TO_BASE(x) ((uint64_t)(x) >> PAGE_SHIFT)

#define PTE_ADDR_VALUE_MASK ((1ull << (48 - PAGE_SHIFT)) - 1)
#define PTE_ADDR_MASK       (PTE_ADDR_VALUE_MASK << PAGE_SHIFT)

typedef struct {
    uint64_t EntryType : 2;             // @0-1     1 for a block table, 3 for a page table
    uint64_t MemAttr : 4;               // @2-5
    enum {
        STAGE2_S2AP_NOREAD_EL0 = 1,     //          No read access for EL0
        STAGE2_S2AP_NO_WRITE = 2,       //          No write access
    } S2AP : 2;                         // @6-7
    enum {
        STAGE2_SH_OUTER_SHAREABLE = 2,  //          Outter shareable
        STAGE2_SH_INNER_SHAREABLE = 3,  //          Inner shareable
    } SH : 2;                           // @8-9
    uint64_t AF : 1;                    // @10      Accessable flag
    uint64_t PTE_NG : 1;                // @11      no global 
#ifdef PAGE_SIZE_64K
    uint64_t _reserved12_15 : 4;        // @12-15   Set to 0 for 64K granule descriptors
    uint64_t Address : 32;              // @16-47   32 Bits of address
#elif defined(PAGE_SIZE_16K)
    uint64_t _reserved12_13 : 2;        // @12-13   Set to 0 for 16K granule descriptors
    uint64_t Address : 34;              // @14-47   34 Bits of address
#else
    uint64_t Address : 36;              // @12-47   36 Bits of address
#endif
    uint64_t _reserved48_51 : 4;        // @48-51   Set to 0
    uint64_t Contiguous : 1;            // @52      Contiguous
    uint64_t PXN : 1;                   // @53     kernel No execute if bit set
    uint64_t UXN : 1;                   // @54     user No execute if bit set
    uint64_t _reserved55_58 : 4;        // @55-58   Set to 0
    uint64_t PXNTable : 1;              // @59      Never allow execution from a lower EL level
    uint64_t XNTable : 1;               // @60      Never allow translation from a lower EL level
    enum {
        APTABLE_NOEFFECT = 0,           // No effect
        APTABLE_NO_EL0 = 1,             // Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup
        APTABLE_NO_WRITE = 2,           // Write access not permitted, at any Exception level, regardless of permissions in subsequent levels of lookup
        APTABLE_NO_WRITE_EL0_READ = 3   // Write access not permitted, at any Exception level, Read access not permitted at EL0.
    } APTable : 2;                      // @61-62   AP Table control .. see enumerate options
    uint64_t NSTable : 1;               // @63      Secure state, for accesses from Non-secure state this bit is RES0 and is ignored
}page_table_entry_t, page_dir_entry_t;


#define  MT_DEVICE_NGNRNE  0
#define  MT_DEVICE_NGNRE   1
#define  MT_NORMAL_NC      2
#define  MT_NORMAL         3

void set_pte_flags(page_table_entry_t* pte, uint64_t pte_attr);

/*
 * Access descriptor addresses through a may-alias 64-bit view so the compiler
 * emits aligned whole-descriptor loads/stores instead of bitfield writes.
 */
typedef uint64_t pte_raw_t __attribute__((__may_alias__));

static inline uint64_t pte_get_address(const page_table_entry_t* pte) {
	const volatile pte_raw_t* raw = (const volatile pte_raw_t*)(const void*)pte;
	return ((*raw) >> PAGE_SHIFT) & PTE_ADDR_VALUE_MASK;
}

static inline void pte_set_address(page_table_entry_t* pte, uint64_t address) {
	volatile pte_raw_t* raw = (volatile pte_raw_t*)(void*)pte;
	uint64_t value = *raw;
	value &= ~PTE_ADDR_MASK;
	value |= (address & PTE_ADDR_VALUE_MASK) << PAGE_SHIFT;
	*raw = value;
}

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5

int32_t  map_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr, ewokos_addr_t physical,
	                uint32_t access_permissions, uint32_t pte_attr);
void  unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr);
ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual);
void free_page_tables(page_dir_entry_t *vm);

void __set_translation_table_base(uint64_t);
void __flush_tlb(void);

void clear_cache(void *start, void *end);

#endif
