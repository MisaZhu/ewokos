#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include <stdint.h>
#include <ewokos_config.h>
#include <mm/mmudef.h>

#define PAGE_PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PAGE_PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PAGE_PD_INDEX(x)   (((x) >> 21) & 0x1FF)
#define PAGE_PT_INDEX(x)   (((x) >> 12) & 0x1FF)

typedef union {
	struct {
		uint64_t present : 1;
		uint64_t rw : 1;
		uint64_t us : 1;
		uint64_t pwt : 1;
		uint64_t pcd : 1;
		uint64_t accessed : 1;
		uint64_t dirty : 1;
		uint64_t pat : 1;
		uint64_t global : 1;
		uint64_t ignored0 : 3;
		uint64_t Address : 40;
		uint64_t ignored1 : 11;
		uint64_t nx : 1;
	};
	uint64_t value;
} page_table_entry_t;

typedef page_table_entry_t page_dir_entry_t;

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5
#define PTE_ATTR_WRCOMB          6

#define PTE_ATTR_FRAMEBUFFER     PTE_ATTR_WRCOMB

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);
page_table_entry_t* get_page_table_entry(page_dir_entry_t* vm, ewokos_addr_t virtual_addr);
void __set_translation_table_base(uint64_t base);
void __flush_tlb(void);

/* PC 标准 VGA 文本缓冲区 (vgacon: kout 的第二输出汇点, 无串口平台的
 * 内核日志可见性)。VA 选在 PDPT[2] (0x80000000-0xBFFFFFFF) 的空闲 PD 480:
 * clone_kernel_vm 把 PDPT[2] 按引用共享给所有任务页表 (不参与进程退出
 * 回收), 内核与全部用户进程通写同一映射; 该 VA 避开内核镜像/页目录/
 * kmalloc/sys_dma/allocable 直连/mmio 等所有运行时映射区间。物理地址
 * 恒为 PC 标准的 0xB8000。 */
#define X86_VGA_TEXT_VADDR  0xBE000000UL
/* GOP 帧缓冲控制台窗口 (PDPT[2] PD 448 起, 2MB 大页, arch_vm 映射)。
 * 布局约束: PD 0-255=内核 P2V 直映射, 256-287=mmio VA, 272-279=sys_dma VA,
 * 384-447=ramdisk 窗口(128MB), 400=用户态 fb (bsp_fb), 496=VGA 文本 */
#define X86_FB_VA           0xB8000000UL
#define X86_VGA_TEXT_PHYS   0xB8000UL

#endif
