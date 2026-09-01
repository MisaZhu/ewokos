# 05 内存管理与 MMU

> 本章目标：理解虚拟内存与页表，看懂 EwokOS 如何建立内核内存布局，
> 理解物理页分配器（kalloc）与内核堆（kmalloc），
> 以及为什么每个进程都以为自己独占了整个内存。

这是操作系统最核心、也最精妙的一章。请慢慢读。

## 5.1 问题：内存不够分，还不能乱分

树莓派 3B 只有 1GB 内存。上面要跑内核、文件系统、窗口系统、十几个应用。
三个问题必须解决：

1. **隔离**：进程 A 不能读写进程 B 的内存，更不能碰内核内存；
2. **欺骗**：每个进程都以为"内存从 0 开始、全是我的"，编写程序不用关心别人；
3. **灵活**：程序使用的内存不必物理连续，还能按需增长。

解决方案就是 **MMU（内存管理单元）** + **虚拟内存**。

## 5.2 MMU 与页表：翻译机器

CPU 每次访问内存用的地址叫**虚拟地址（VA）**。MMU 是 CPU 里的一个硬件模块，
它在访问真正发生前，把虚拟地址**翻译**成**物理地址（PA）**：

```
程序: ldr x0, [0x1000]          （虚拟地址）
            │
            ▼  MMU 查页表
内存: 物理地址 0x9ABF1000       （真实位置）
```

翻译规则写在**页表（Page Table）**里，由操作系统维护。规则是**分页**的：
虚拟内存被切成固定大小的**页**（EwokOS 用 4KB），页表说明"每一页映射到哪块物理内存，
权限是什么（可读/可写/可执行/是否允许 EL0 访问）"。

这就实现了隔离：进程 A 的页表里，根本没有内核内存的映射——
A 想访问也无路可走（MMU 直接抛异常，交给第 04 章的异常机制处理）。

### AArch64 的四级页表

AArch64 用 **4 级页表**（像目录套目录）翻译 48 位虚拟地址：

```
虚拟地址 48 位:
┌────────┬────────┬────────┬────────┬────────────┐
│ L0 索引 │ L1 索引 │ L2 索引 │ L3 索引 │ 页内偏移    │
│  9 位   │  9 位   │  9 位   │  9 位   │  12 位     │
└────────┴────────┴────────┴────────┴────────────┘
    │         │         │         │          │
    ▼         ▼         ▼         ▼          ▼
  L0表 ──► L1表 ──► L2表 ──► L3表 ──► 4KB物理页内的偏移
```

两个关键寄存器：
- `TTBR0_EL1`：管**低地址空间**（虚拟地址 0 开始，给用户进程用）；
- `TTBR1_EL1`：管**高地址空间**（虚拟地址最高位为 1 的区域，给内核用）。

所以 EwokOS 的约定是：**用户程序在低地址，内核在高地址**。

## 5.3 EwokOS 的内存布局

看 [mmudef.h](../../kernel/platform/aarch64/arch/common/include/mm/mmudef.h)
和 [mmu.h](../../kernel/kernel/include/mm/mmu.h)：

```c
#define KERNEL_BASE  0x4000000000ull   // 256GB 处：内核虚拟地址起点
// MMIO 紧跟在内核可用内存区之后
#define MMIO_BASE    (KERNEL_BASE + MAX_USABLE_MEM_SIZE)
```

整体布局（AArch64）：

```
高地址（TTBR1 管辖，内核专用）
┌──────────────────────────────────────┐
│ MMIO 区    设备寄存器（串口/SD卡/中断）│ ← MMIO_BASE，映射到设备物理地址
├──────────────────────────────────────┤
│ kmalloc 区  内核堆（小对象分配）        │
├──────────────────────────────────────┤
│ 可分配物理内存的直接映射区              │ ← 内核视角的"物理内存窗口"
├──────────────────────────────────────┤
│ 内核页目录区（所有进程的页表都放这）     │
├──────────────────────────────────────┤
│ 内核镜像（代码+数据）                  │ ← KERNEL_BASE
└──────────────────────────────────────┘
低地址（TTBR0 管辖，每个进程各不相同）
┌──────────────────────────────────────┐
│ 进程代码、数据、堆、栈……               │ ← 每个进程自己的视图
└──────────────────────────────────────┘
```

为什么内核放高地址？**切换进程时只需要换低半部分的页表，
高半部分（内核）所有进程共享**——第 07 章的 `clone_kernel_vm` 就是这么干的。

## 5.4 启动第一关：启动页表

CPU 上电时 MMU 是关的，虚拟地址 = 物理地址。但内核代码是按
"高地址（KERNEL_BASE）"编译的，不开 MMU 就跑不起来。这是经典的鸡生蛋问题，
EwokOS 的解法：**先用最简单的页表开 MMU，再换正式页表**。

看 [start.c](../../machines/raspix/kernel/bsp/start.c) 的 `_boot_start`：

```c
void _boot_start(void) {
    boot_pgt_init();
    set_boot_pgt(0, 0, 64*MB, 0);                  // ① 低地址恒等映射 64MB
    set_boot_pgt(KERNEL_BASE, 0, 64*MB, 0);        // ② 内核高地址也映射前 64MB
    switch(cpu_part()){                             // ③ 按 CPU 型号映射设备寄存器
        case ARM_CPU_PART_CORTEX_A72:               //    Pi4
            set_boot_pgt(MMIO_BASE, PIX4_MMIO_PHY, PIX_MMIO_SIZE, 1);
            break;
        default:                                    //    Pi2/Pi3
            set_boot_pgt(MMIO_BASE, PIX3_MMIO_PHY, PIX_MMIO_SIZE, 1);
            break;
    }
    load_boot_pgt((ewokos_addr_t)startup_page_dir); // ④ 装载页表、打开 MMU
}
```

解读：
- ① 让"虚拟地址 0~64MB = 物理地址 0~64MB"，这样当前正在跑的代码切换页表后不会断；
- ② 让内核能按编译时的高地址（KERNEL_BASE）访问同一块物理内存；
- ③ 把设备物理地址（Pi3 是 `0x3F000000`）映射到高地址的 MMIO 区，
  注意最后一个参数 `is_dev=1`：**设备内存不能缓存**，必须每次真实读写；
- ④ `load_boot_pgt` 在 [boot.S](../../kernel/platform/aarch64/arch/v8/boot.S) 里：

```asm
load_boot_pgt:
    ldr x1, =MAIR1VAL
    msr mair_el1, x1     ; 定义内存属性（普通内存可缓存 / 设备内存不缓存）
    msr ttbr0_el1, x0    ; 装入页表基地址
    msr tcr_el1, x0      ; 配置页表格式（4KB 页、48 位地址……）
    mrs x0, sctlr_el1
    ldr x1, =SCTLREL1VAL
    orr x0, x0, x1       ; M=1（开 MMU）+ C=1（开数据缓存）+ I=1（开指令缓存）
    msr sctlr_el1, x0
    ret                  ; 从这一刻起，所有内存访问都要经过翻译了
```

## 5.5 正式内核页表

启动页表只是应急。`_kernel_entry_c` 里随即建立正式的、完整的内核页表
（[kernel.c](../../kernel/kernel/src/kernel.c)）：

```c
static void set_kernel_vm(page_dir_entry_t* vm) {
    memset(vm, 0, PAGE_DIR_SIZE);
    // 内核镜像
    map_pages(vm, KERNEL_BASE, _sys_info.phy_offset, V2P(KERNEL_IMAGE_END),
              AP_RW_D, PTE_ATTR_WRBACK_ALLOCATE);
    // 内核页目录区
    map_pages(vm, KERNEL_PAGE_DIR_BASE, ...);
    // 内核堆
    map_pages(vm, KMALLOC_BASE, ...);
    // MMIO（注意权限：只有内核可访问，设备属性不缓存）
    map_pages_size(vm, _sys_info.mmio.v_base, _sys_info.mmio.phy_base,
                   _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);
}
```

`map_pages` 就是"写页表"：逐级分配/填写 L0~L3 表项。
页表项（PTE）里除了物理地址，还带属性位：

| 属性 | 作用 |
|------|------|
| `AP_RW_D` | 只允许内核（特权级）读写，EL0 访问即异常 |
| `AP_RW_RW` | 内核和用户都能访问 |
| `PTE_ATTR_WRBACK` | 普通内存，启用缓存 |
| `PTE_ATTR_DEV` | 设备内存，禁止缓存、禁止乱序 |

之后 `set_translation_table_base(V2P(_kernel_info.kernel_vm))`
把新页表装入 `TTBR1_EL1`，旧页表光荣退役。

## 5.6 物理页分配器：kalloc

有了页表，还需要有人管理"哪些物理页空闲"。EwokOS 有两层分配器：

1. **`kalloc`**（[kalloc.c](../../kernel/kernel/src/mm/kalloc.c)）：
   以 4KB 物理页为单位，专门给**页表本身**分配内存（因为给普通内存分配页表时，
   普通内存管理器可能还没就绪）；
2. **`kmalloc`**（[kmalloc.c](../../kernel/kernel/src/mm/kmalloc.c)）：
   内核堆，分配任意大小的小对象（结构体、字符串），类似用户态的 `malloc`。

`_kernel_entry_c` 中的初始化顺序体现了这一点：

```c
kmalloc_init();              // 先开最小内核堆，才能创建各种数据结构
...
init_allocable_mem();        // 把剩余物理内存全部纳入管理
```

内核用 `_pages_ref` 数组记录每个物理页的引用计数——
这是 `fork()` 实现**写时复制（CoW）**的基础（第 07 章）。

## 5.7 进程的独立内存：clone_kernel_vm

每个新进程需要自己的页表。看
[内核的 clone_kernel_vm](../../kernel/kernel/src/kernel.c)（AArch64 版）：

```c
static void clone_kernel_vm(page_dir_entry_t* vm) {
    uint32_t kernel_l1_base = PAGE_ROOT_INDEX(KERNEL_BASE);
    memset(vm, 0, PAGE_DIR_SIZE);

    // 高半部分（内核）：直接复制内核页表指针——所有进程共享同一份内核映射
    for(uint32_t i = kernel_l1_base; i < PAGE_DIR_NUM; i++)
        vm[i] = _kernel_info.kernel_vm[i];

    // 低半部分（用户）：空的，等待进程自己的代码/数据/堆/栈映射进来
    ...
}
```

于是每个进程的世界是：

```
进程 A 的页表                      进程 B 的页表
┌──────────────────────┐          ┌──────────────────────┐
│ 低半部：A 的代码/数据  │          │ 低半部：B 的代码/数据  │  ← 互不可见
├──────────────────────┤          ├──────────────────────┤
│ 高半部：内核（共享）   │ ════════ │ 高半部：内核（共享）   │  ← 同一份
└──────────────────────┘          └──────────────────────┘
```

进程切换时只需切换 `TTBR0`（低半部页表基址），内核部分纹丝不动。

用户程序 `malloc` 时，库函数通过系统调用让内核在低半部映射新页（第 08 章的
`SYS_MALLOC_EXPAND`）；缺页异常也会由内核按需分配物理页。

## 5.8 动手练习

1. 打开 [mmu.h](../../kernel/kernel/include/mm/mmu.h)，
   找出 `KERNEL_BASE`、`MMIO_BASE`、`KMALLOC_BASE` 的定义，画出你自己的内存布局图；
2. 启动系统后观察启动日志里 `show_config()` 打印的地址范围，
   与你的图对照（`kernel image`、`kmalloc`、`mmio_base` 各是哪些地址）；
3. 思考题：如果两个进程的某页虚拟地址映射到**同一块物理内存**，能做什么？
   （答案：共享内存——EwokOS 的 `shm` 机制，第 13 章图形系统会用到。）

## 5.9 本章小结

- MMU 依据页表把虚拟地址翻译成物理地址，页表由内核维护；
- AArch64 用 4 级页表，`TTBR0` 管用户低地址，`TTBR1` 管内核高地址；
- EwokOS 内核位于 `KERNEL_BASE=0x4000000000`，MMIO 紧随其后；
- 启动时先用最小页表开 MMU，再建立正式内核页表；
- `kalloc` 管物理页，`kmalloc` 管内核堆；
- 每个进程共享内核高半部页表，拥有私有低半部——隔离与效率兼得。

下一章：中断与定时器——让内核拥有"时间"，能够及时响应外部事件。
