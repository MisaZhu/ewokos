# 23 缓存维护：内核如何保证内存一致性

> 语言: [English](23-cache.md) | **中文**
>
> 本章目标：理解 CPU 缓存为什么让操作系统的工作变难；掌握精确的术语
> （clean/invalidate/flush、PoC/PoU、按 set/way 与按 VA）；逐行读懂 EwokOS
> 的可移植缓存 API 及其 AArch64 / ARMv7 / x86 实现；看清内核**必须**刷新缓存或
> TLB 的四个真实场景——修改映射、切换地址空间、加载代码、把内存交给设备。
>
> 本章是紧邻 [05 内存管理与 MMU](05-mmu.zh.md) 与 [07 进程与调度器](07-process.zh.md)
> 的一次深入剖析。术语（TLB/TTBR/ASID/PoC/PoU…）汇总见 [附录·术语表](99-glossary.zh.md)。

[05](05-mmu.zh.md) 与 [07](07-process.zh.md) 章都悄悄假设了一件事：“内核写下一个
字节，所有人都能立刻看到。”在真实硬件上，这个假设是**错的**，而错的原因就是缓存。
本章要拆掉这个假设，讲清楚 EwokOS 到底做了什么，才能让 CPU、取指单元、页表走查器
（table walker）和 DMA 设备对“内存里到底是什么”达成一致。

下面所有代码引用都指向真实文件：

| 层次 | 文件 |
|---|---|
| 可移植 API（声明） | [`kernel/kernel/include/kernel/system.h`](../../kernel/kernel/include/kernel/system.h) |
| 与平台无关的辅助函数（延时、内核锁、halt） | [`kernel/kernel/src/system.c`](../../kernel/kernel/src/system.c) |
| 按架构的 C wrapper（策略） | `kernel/platform/<arch>/arch/<ver>/system_arch.c` —— [aarch64/v8](../../kernel/platform/aarch64/arch/v8/system_arch.c)、[arm/v7](../../kernel/platform/arm/arch/v7/system_arch.c)、[x86/x64](../../kernel/platform/x86/arch/x64/system_arch.c)、[riscv/rv64](../../kernel/platform/riscv/arch/rv64/system_arch.c) |
| AArch64 汇编（机制） | [`kernel/platform/aarch64/arch/v8/system.S`](../../kernel/platform/aarch64/arch/v8/system.S) |
| ARMv7 汇编 | [`kernel/platform/arm/arch/v7/system.S`](../../kernel/platform/arm/arch/v7/system.S) |
| x86 汇编 | [`kernel/platform/x86/arch/x64/system.S`](../../kernel/platform/x86/arch/x64/system.S) |
| 调用方 | `mm/mmu.c`、`mm/shm.c`、`mm/dma.c`、`proc.c`、`svc.c`、`irq.c` |

> **近期重构（“kernel arch porting friendly”）**：所有缓存 / TLB / 地址空间维护原语
> 过去都挤在一份共享的 `system.c` 里，靠 `#if defined(__aarch64__) / ARM_V7 / …`
> 分支区分架构。如今每个分支都被原样搬进了按架构独立的
> `kernel/platform/<arch>/arch/<ver>/system_arch.c`（由各平台的 `make.rule` 接入
> 构建）。共享的 `kernel/kernel/src/system.c` 只剩与平台无关的辅助函数
> （`_delay*`、`kernel_lock*`、`halt`），**完全不再含按架构的条件编译**——移植到
> 新架构时只需新写一份 `system_arch.c`，不必再碰共享代码。

## 23.1 缓存为什么给内核制造麻烦

现代 CPU 以**缓存行（cache line）**为单位读内存（通常 64 字节），并在一块小而快的
SRAM 缓存里保留一份副本。当你写入时，写通常只落到缓存、**并不落到 DRAM**；这条脏行
（dirty line）会在缓存“想写回的时候”才被写回。对普通程序数据来说，这既看不见也无害——
因为同一个核通过同一个缓存读自己刚写的数据。

一旦出现**第二个观察者**，通过*另一条路径*去看同一块物理内存，问题就来了：

1. **取指单元。** I-cache（指令缓存）与 D-cache（数据缓存）是分开的。内核刚写进某个页的
   代码（一次 ELF 加载）还留在 D-cache 里；而 I-cache 里装的仍是这块物理地址*过去*的内容
   ——往往是上一个已死进程、其物理页被回收复用后残留的指令。
2. **页表走查器。** MMU（Memory Management Unit，内存管理单元）要读页表来做地址翻译。
   如果走查器从 DRAM 读页表（“非缓存走查”），而内核更新过的 PTE（Page Table Entry，
   页表项）还以脏行形式滞留在 D-cache 中，走查器就会用一条陈旧的表项去翻译。
3. **DMA 设备 / GPU。** 设备直接读 DRAM。如果 CPU 写了一个缓冲区但那行还在缓存里是脏的，
   设备读到的就是垃圾数据；反过来，当设备写 DRAM 后，CPU 缓存里一条陈旧的干净行稍后被
   淘汰写回，又会把设备的数据覆盖掉。
4. **另一个 CPU 核。** 在 SMP（Symmetric Multi-Processing，对称多处理）下，B 核的缓存可能
   还留着 A 核刚改过的那一行。因此缓存*维护*操作必须**广播**到整个 inner-shareable
   （内部可共享）域，而不能只在本核做。
5. **TLB。** TLB（Translation Lookaside Buffer，地址翻译后备缓冲）不是数据缓存，但属于
   同一类问题：MMU 缓存的是*翻译结果*。你改完一条 PTE 后，旧的翻译还留在 TLB 里，必须
   显式失效掉。

所以“刷新缓存”实际上是五件相互关联的活儿。本章余下部分，就是一张地图：EwokOS 在每种
架构上分别如何完成这五件事。

## 23.2 必须先掌握的术语

ARM（以及本教程）对三个动词和两个“点”的用法非常精确。把它们搞混是隐蔽 bug 的头号来源。

| 术语 | 含义 | 对一条脏行的效果 |
|---|---|---|
| **Clean（清）** | 把脏数据写回下一级，**保留**该行在缓存中 | 数据到达内存，行仍在缓存（现已变干净） |
| **Invalidate（失效）** | **不写回**就直接丢弃该行 | 脏数据**丢失** |
| **Flush（刷）= clean + invalidate** | 既写回**又**丢弃该行 | 数据到达内存，行被移除 |

以及维护操作针对的两个“点”：

- **PoU（Point of Unification，统一点）**——D-cache、I-cache 与 MMU 走查器三者达成一致的
  那个点。把代码 clean 到 PoU，才能让它对取指可见。
- **PoC（Point of Coherency，一致性点）**——*所有*观察者（CPU、DMA、其他核）看到相同字节
  的那个点。把数据 flush 到 PoC，才能让它对设备可见。

最后是*如何寻址*这些缓存行：

- **按 set/way（组/路）**——几何式地遍历每一级缓存的每一行。这是命中**整个**缓存的唯一
  办法；它慢，而且会摧毁工作集（working set）。只在启动和需要“全量发布”时使用。
- **按 VA（虚拟地址）**——一次针对一个地址操作一条缓存行。便宜、精准。热路径都用它。

> 经验法则：**已知范围用按 VA，只有真的要“全部”时才用按 set/way。** EwokOS 近期优化的
> 很大一部分工作，就是把整缓存的 set/way 全量刷写替换成定向的按 VA 操作。

## 23.3 可移植 API

所有调用方都使用同一个可移植头文件
[`kernel/kernel/include/kernel/system.h`](../../kernel/kernel/include/kernel/system.h)，
它声明了一小组与架构无关的接口：

```c
extern void flush_tlb(void);              // 全局发布映射变更
extern void flush_tlb_nosweep(void);      // 仅失效 TLB（描述符已发布）
extern void flush_tlb_addr(ewokos_addr_t);// 失效单个页的 TLB 表项
extern void flush_tlb_asid(uint32_t asid);// 丢弃带某个 ASID 标记的所有 TLB 表项
extern void flush_dcache(void);           // clean+invalidate 整个 D-cache（按 set/way）
extern void invalidate_dcache(void);      // 失效整个 D-cache（不写回）

/* 代码加载一致性：把写入的代码 clean 到 PoU，再丢弃 I-cache */
extern void dcache_clean_code_range(const void* start, uint32_t size);
/* 把经可缓存别名写入的数据推到 DRAM（PoC）并丢弃 */
extern void dcache_flush_range(const void* start, uint32_t size);
extern void invalidate_icache_all(void);  // 丢弃 I-cache（SMP 下广播）
```

它们**不是**裸指令。每个接口都按架构实现在
`kernel/platform/<arch>/arch/<ver>/system_arch.c` 里（例如
[`aarch64/v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c)、
[`arm/v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)），作为同目录
`system.S` 里汇编原语之上的薄 C wrapper，各自为本架构决定正确行为——包括“是否压根
需要全量 D-cache 刷写”。这些 wrapper 是核心设计决策：**策略（“改一次映射需不需要
整缓存 clean？”）放在按架构的 C 里；只有机制（具体指令）放在汇编里。**共享的
`system.c` 已完全不再参与此事。

## 23.4 `flush_tlb()`：最微妙的一个 wrapper

只要页表被批量改动，就会调用 `flush_tlb()`。它的函数体因架构而天差地别，读它是理解每个
平台内存模型最快的方式。

**AArch64**（[`v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c)）：

```c
void flush_tlb(void) {
    /* 页表通过 D-cache 走查（TCR IRGN/ORGN write-back、inner shareable），
     * 用户内存是 PIPT 一致的，所以发布 PTE 存储只需要 __flush_tlb 里的 dsb。 */
    __flush_tlb();
}
```

**ARMv7**（[`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)）：

```c
void flush_tlb(void) {
    /* 批量建表与板级页表拷贝仍依赖一次整 D-cache 发布；…… */
    flush_dcache();
    __invalidate_icache_all();
    __flush_tlb();
}
```

**x86 / RISC-V / ARMv5 / ARMv6** 各自的 `system_arch.c` 里也有自己的版本（x86 与
RISC-V：`flush_dcache()` + `__flush_tlb()`，在 `KERNEL_SMP` 下再加一次 I-cache 丢弃）。

在 AArch64 上，*整个* D-cache 刷写被**移除**了。为什么这样安全？因为启动代码把
`TCR_EL1`（Translation Control Register，翻译控制寄存器）配成让页表走查器以
**inner-shareable、write-back 可缓存**内存的方式读页表，而用户内存是 **PIPT**
（Physically-Indexed-Physically-Tagged，物理索引物理标记）一致的。因此走查器会*窥探*
（snoop）D-cache，总能看到最新的 PTE。剩下的只需一条数据同步屏障（`dsb`）给存储排序，
然后做 TLB 失效。

在 ARMv7 上，批量路径**保留**整 D-cache 的 clean。原因见 §23.6。

它最终落到的 AArch64 汇编
（[`v8/system.S`](../../kernel/platform/aarch64/arch/v8/system.S)）只有四条指令：

```asm
__flush_tlb:
    dsb  ishst          // 把之前的 PTE 存储发布给走查器
    tlbi vmalle1is      // 失效所有 EL1&0 表项，inner-shareable 广播
    dsb  ish            // 等待失效完成
    isb                 // 重新同步取指
    ret
```

读一下这个助记符：`tlbi vmalle1is` = **TLB Invalidate（TLB 失效）**，“EL1&0 下当前 VMID
的所有表项”，**IS** = 广播到 **I**nner **S**hareable（内部可共享）域。这个 `is` 后缀正是
它在 SMP 下正确的原因。

## 23.5 单页快速路径

批量 `flush_tlb()` 是一门大炮。而大多数映射改动只碰**一个页**，所以 EwokOS 提供了两个
精准 wrapper，被 `proc.c`、`shm.c`、`irq.c` 大量使用。

### `flush_tlb_addr(addr)`——失效单个页

AArch64（[`v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c)）：

```c
void flush_tlb_addr(ewokos_addr_t addr) {
    ewokos_addr_t page = addr >> 12; /* TLBI VA 操作数恒为 VA[55:12]，与粒度无关 */
    __asm__ volatile(
        "dsb ishst\n"
        "tlbi vaae1is, %0\n"          // 全 ASID 形式：用户页是 nG
        "dsb ish\n"
        "isb\n"
        :: "r"(page) : "memory");
}
```

ARMv7（[`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)）：

```c
void flush_tlb_addr(ewokos_addr_t addr) {
    ewokos_addr_t page = addr & ~(ewokos_addr_t)0xfff; // VA[31:12] 原位保留
    __asm__ volatile(
        "dsb\n"
#ifdef KERNEL_SMP
        "mcr p15, 0, %0, c8, c3, 3\n" /* TLBIMVAAIS */
#else
        "mcr p15, 0, %0, c8, c7, 3\n" /* TLBIMVAA */
#endif
        "dsb\n"
        "isb\n"
        :: "r"(page) : "memory");
}
```

其余架构在各自的 `system_arch.c` 里：ARMv5/v6 回退到完整的 `flush_tlb()`（那里没有
逐行发布路径）；RISC-V 发 `sfence.vma addr`；x86 发 `invlpg (addr)`。

这里有两个细节是“承重”的，而且各自都曾是一个真实 bug 的根源：

- **AArch64 的 TLBI VA 操作数是 `addr >> 12`，而不是 `addr >> PAGE_SHIFT`。** ARM 规定该
  操作数恒为 VA[55:12]，*与所配置的页粒度（granule）无关*，硬件自己会抽取正确的位。在
  16 KB 粒度的平台（raspi5）上用 `>> PAGE_SHIFT`（=14）会算出错误操作数，留下陈旧 TLB
  表项 → 一个静默的 translation fault（翻译错误）。永远不要“修好”这个移位。
- **AArch64 使用全 ASID 形式 `vaae1is`**，因为用户页被标记为 `nG`（non-global，非全局）并
  带 ASID 标记；普通的 `vae1is` 形式只会命中 ASID 等于操作数高位零的那些表项，从而全部
  落空。
- 在 **ARMv7 上，操作数是把 VA 低 12 位清零、*原位*保留**——与 AArch64 恰好相反的约定。
  不要把一个平台的写法移植到另一个。

### `flush_tlb_nosweep()`——只失效 TLB

当 `map_page`/`unmap_page` *已经*把改动的描述符发布到 PoC（ARMv7 逐行发布），或走查本身是
PIPT 一致的（AArch64），一个只动了数据映射的调用方就能完全跳过整缓存刷写：

```c
/* aarch64 与 arm/v7 的 system_arch.c： */
void flush_tlb_nosweep(void) {
    __flush_tlb();     // 只做 TLBI；描述符已对走查器可见
}

/* arm/v5、arm/v6、riscv、x86 的 system_arch.c： */
void flush_tlb_nosweep(void) {
    flush_tlb();       // 没有逐行发布路径：回退到全量形式
}
```

`proc.c` 在堆/栈增长与写时复制（COW）重映射时用
它（`proc_expand_mem`、`proc_shrink_mem`、fork COW）。它**仅**对那些没有任何非一致主设备
通过可缓存别名去读的数据映射有效——绝不能用于批量建表拷贝或 exec 代码路径。

## 23.6 ARMv7：可缓存页表走查改造

ARMv7（`raspix`、`machine.virt` arm32）最初以**非缓存**方式走查页表：
`__set_translation_table_base` 把 `TTBR0`（Translation Table Base Register，翻译表基址
寄存器）的 IRGN/RGN/S 位全留为零。这意味着走查器直接从 DRAM 读页表。但内核是通过它的
*可缓存*线性映射（`P2V`）写 PTE 的，于是一条脏 PTE 行可能滞留在 D-cache 中而走查器永远
看不到——这就是为什么每次改映射都需要一次**整 D-cache clean** 作为粗暴的兜底安全网。

改造（[`kernel/platform/arm/make.rule`](../../kernel/platform/arm/make.rule) 里的
`-DARM_V7` 宏仍用于把 v7 的汇编与 v5/v6 区分开，而 C 策略如今住在专属的
[`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c) 而非共享代码里）
把走查变成可缓存、inner-shareable，与 AArch64 完全一致：

```asm
__set_translation_table_base:
    dsb
    mcr p15, 0, r0, c2, c0, 0     // TTBR0 = base（base 已 OR 上 0x0B）
    isb
    ...
```

那个 `0x0B` 是 `IRGN=write-back（bit0）| S=shareable（bit1）| RGN=write-back（bit3）`。有了
它，走查器就能跨核窥探一致的 D-cache。`boot.S` 的 `load_boot_pgt` 也置相同的位，保证启动
页表走查属性一致。

即便如此，ARMv7 仍**保留**切换时的整 D-cache 刷写。
`set_translation_table_base_asid()` 里的注释把原因说得很直白：

```c
/* 保留切换时的整 D-cache 刷写：在 ARMv7 上它对非一致 DMA/图形
 * doorbell 排序是承重的（去掉它会在真机上把 X 冻死；只做 I-cache
 * 或只放一条裸 dsb 的变体都不能替代）。 */
flush_dcache();
__set_translation_table_base_asid(tlb_base, asid);
```

这是一条用真机换来的硬事实：在 ARMv7 上这次刷写*不是*为了 TLB 或走查器的正确性——而是让
非一致 DMA 和 VC4 图形 doorbell 正常工作的关键。去掉它会在真机上把 X 窗口系统挂死。

同一次改造中 ARMv7 获得的按 VA 构件
（[`v7/system.S`](../../kernel/platform/arm/arch/v7/system.S)）：

```asm
__dcache_flush_poc_range:            // 把一段 VA 范围 clean+invalidate 到 PoC
    ...  mcr p15, 0, r0, c7, c14, 1  // DCCIMVAC，行长取自 CTR.DminLine

__dcache_clean_pou_range:            // 把代码 clean 到 PoU
    ...  mcr p15, 0, r0, c7, c11, 1  // DCCMVAU

__invalidate_icache_all_is:          // 广播式 I-cache 失效
    mcr p15, 0, r0, c7, c1, 0        // ICIALLUIS
```

ARMv5/v6（[`v5/system_arch.c`](../../kernel/platform/arm/arch/v5/system_arch.c)、
[`v6/system_arch.c`](../../kernel/platform/arm/arch/v6/system_arch.c)）保持原有语义：
非缓存走查、`flush_tlb()` 内做整缓存维护、`dcache_flush_range` /
`dcache_clean_code_range` 是*空*实现。

## 23.7 x86：MESI 让大部分工作变得多余

x86 硬件天生缓存一致（MESI 协议 + PCIe 窥探）。DMA 设备的访问会自动窥探 CPU 缓存，所以
ARM 需要的软件 clean/invalidate 那套舞步在 x86 上**根本不存在**。EwokOS 在
[`kernel/platform/x86/arch/x64/system.S`](../../kernel/platform/x86/arch/x64/system.S)
中如实反映了这一点：`__flush_dcache_all` / `__invalidate_dcache_all` **不是** `wbinvd`，而是
被刻意降级为一条普通的 `mfence` 屏障。

为什么不用 `wbinvd`？因为 `wbinvd` 会刷*整个*缓存——开销巨大——而且在虚拟机里会触发一次
昂贵的 VM-exit。对一致内存而言，一条屏障就够了。x86 *仍然*需要的是：

- **TLB**：改映射后重载 `CR3`（或对单页用 `invlpg`）——x86 不会自动失效 TLB。
- **I-cache / 存储排序**：内核写代码处放 `mfence`。
- **改 MTRR/PAT**：`x86_pat_init` 仍会真发一次 `wbinvd`，因为修改内存类型寄存器确实需要
  一次全量刷写。

## 23.8 内核到底在哪儿调用这些

只有看到调用它们的四个场景，这些 wrapper 才有意义。

### (a) 修改一次映射——`mmu.c`、`shm.c`

`map_page` / `unmap_page` 写一条 PTE。随后每个调用方都要失效受影响的翻译。`shm.c` 把它
限定到单个页：

```c
unmap_page(_kernel_info.kernel_vm, addr);
flush_tlb_addr(addr);   // 把失效范围限定到这一个页
```

批量重映射（`kmalloc_vm.c` 里的 kmalloc VM 竞技场、`dma.c` 里的 DMA 对端拆除）仍用完整的
`flush_tlb()`。

### (b) 切换地址空间——`set_translation_table_base_asid()`

这在**每次进程切换**时运行。在 AArch64 上它几乎免费：

```c
if(asid != 0 && asid < asid_limit()) {
    __set_translation_table_base_asid(tlb_base, asid);  // 完全不做 TLBI
    return;
}
```

每个地址空间都带自己的 **ASID**（Address Space Identifier，地址空间标识符），且其用户 PTE
是 `nG`，所以把 `TTBR0` 载入 `base | (asid<<48)` 就是切换的*全部* TLB 工作——上一个空间的
表项只是不再匹配、并保持温热。不做任何失效。只有 ASID 装不进该核宽度（或 `asid==0`）的
空间才回退到本核全量失效。`asid_limit()` 读 `ID_AA64MMFR0_EL1.ASIDBits`，从而同时支持
8 位与 16 位 ASID 的核，而不是想当然。

当一个 ASID 被*回收*（pde 槽被复用）时，`flush_tlb_asid()` 会在所有核上丢弃旧持有者的带标
表项，让复用的编号绝不会透过残留解析到旧主人。

### (c) 加载代码 / `exec`——`proc.c`

这是 I-cache 场景。当内核把一段 ELF 段拷进一个全新的（复用的）物理页时，必须让新指令对
取指可见：

```c
if(exec)
    dcache_clean_code_range((const void*)kaddr, chunk);  // 把代码 clean 到 PoU
...
invalidate_icache_all();   // 丢弃 I-cache（SMP 下广播）
```

注意**顺序**：先 clean 数据侧，*再*失效 I-cache。反过来会让一条陈旧行在两步之间重新填充。
而且在 SMP 下失效必须广播（`__invalidate_icache_all_is`，`ICIALLUIS` / `IC IALLUIS`），因为
新进程可能被调度到*另一个*核上，那个核仍持有旧页的指令。

### (d) 把内存交给设备——`shm.c`、`dma.c`

这是 DMA/共享内存场景。连续共享内存被映射**两次**：一份 CPU 用来初始化它的可缓存别名，和
一份设备（或非一致主设备）读取的 `NOCACHE` 别名。在设备碰它之前，CPU 的脏行必须到达 DRAM
并被丢弃：

```c
/* 把可缓存别名 clean 出去到 DRAM 并丢弃这些行，否则稍后一次淘汰
 * 会覆盖 GPU/CPU 通过下面的 NOCACHE 别名写入的内容 */
dcache_flush_range((void*)P2V(paddr), pages * PAGE_SIZE);
```

`shm.c` 里就记录了一个微妙的推论：**设备共享内存对进程可见的那份映射本身必须是
`NOCACHE`。** 过去切换时的整 D-cache clean 意外掩盖了一个可缓存的连续映射；一旦在 AArch64
上移除了每次切换的刷写，这种别名问题就必须通过把两份视图都设为非缓存来正确修好。这正是
§23.10 所警告的那类“隐藏的副作用”。

## 23.9 SMP：为什么 `is` 后缀至关重要

在单核上，`tlbi vmalle1` 和 `ic iallu` 就够了。在 SMP 上不够：本核运行的维护操作会让*其他*
核的 TLB 和 I-cache 保持陈旧。ARM 提供了 **inner-shareable 广播**形式，EwokOS 在
`#ifdef KERNEL_SMP` / 按架构下选择它们：

| 操作 | 本核形式 | 广播形式（SMP 下使用） |
|---|---|---|
| 失效全部 TLB | `tlbi vmalle1` | `tlbi vmalle1is` |
| 按 ASID 失效 TLB | — | `tlbi aside1is` |
| 失效 I-cache | `ic iallu` | `ic ialluis` |
| ARMv7 全 TLB | `TLBIALL (c8,c7,0)` | `TLBIALLIS (c8,c3,0)` |
| ARMv7 I-cache | `ICIALLU (c7,c5,0)` | `ICIALLUIS (c7,c1,0)` |

屏障也与域配对：广播操作用 `dsb ish`（inner-shareable），
`__flush_tlb_local` 里的仅本核回退用 `dsb nsh`（non-shareable）。

## 23.10 用血泪换来的教训（优化前请先读）

这些都是 EwokOS 开发过程中真实发生过的 bug，每一个都值得你警惕：

1. **永远不要把 AArch64 的 TLBI VA 移位从 `addr >> 12` 改成 `addr >> PAGE_SHIFT`。** 该操作数
   定义为 VA[55:12]、与粒度无关；在 16 KB/64 KB 页上那个“显而易见的修法”会算出错误地址，
   留下静默的陈旧 TLB 表项和启动时的 translation fault。
2. **删除任何全局 cache/TLB 维护之前，先清点它的副作用。** exec 路径*依赖*了那个过去搭在旧
   SMP `flush_tlb()` 里顺带做的 I-cache 失效。当 `flush_tlb()` 被瘦身成纯 TLBI 后，I-cache 丢弃
   必须在 `proc.c` 里显式补回（`dcache_clean_code_range` + `invalidate_icache_all`），否则新进程
   会在陈旧指令上崩溃。
3. **在 ARMv7 上，切换时的整 D-cache 刷写对非一致 DMA/图形是承重的**——不是为了 TLB 正确性。
   去掉它会在真机上把窗口系统冻死。只做 I-cache 或只放一条裸 `dsb` 的变体都不能替代。
4. **QEMU 不建模缓存一致性。** 它没有真正的 L1/L2/I-cache，把别名当作恒一致，直接从 RAM 走查
   页表，也不建模 ASID 保留语义。QEMU 适合做*启动冒烟测试*（证明一条新指令编码不是
   UNDEFINED、内核还能起来），但花屏类的别名 bug 和陈旧指令崩溃**只能**在真机上抓到——跑
   `xwin` + VC4 g2d 看别名，跑密集 fork/exec 看 I-cache 广播。
5. **可缓存 ↔ 非缓存别名是一条正确性不变量，不是性能选择。** 如果一个物理帧同时能经由可缓存
   和非缓存两条映射到达，那么在信任非缓存侧之前，必须先把可缓存侧 flush 到 PoC——否则 CPU 会
   读到陈旧的 DRAM，或者稍后一次淘汰会冲掉设备的写入。

## 23.11 小结

- 缓存让“写一次、处处可见”不再成立。内核必须在五个观察者之间主动维护一致性：D-cache、
  I-cache、页表走查器、DMA 设备、以及其他核。
- **clean** 写回并保留；**invalidate** 不写回直接丢弃；**flush** 两者都做。代码针对 **PoU**，
  设备数据针对 **PoC**。优先用**按 VA** 而非**按 set/way**。
- EwokOS 把*策略*放在按架构的 C wrapper 里
  （`kernel/platform/<arch>/arch/<ver>/system_arch.c`），只把*机制*放在同目录的按架构
  汇编（`system.S`）里；共享的 [`system.c`](../../kernel/kernel/src/system.c) 只剩与
  平台无关的辅助函数，不再含任何架构 `#if`。
- AArch64 和（改造后的）ARMv7 以**可缓存 + inner-shareable** 方式走查页表，于是 `flush_tlb()`
  坍缩成一条广播 TLBI 加一条 `dsb`。ARMv7 仍在切换时保留整 D-cache 刷写以照顾 DMA/图形。x86
  靠 MESI 几乎什么都不需要——`wbinvd` 被降级成了 `mfence`。
- 四个真实调用点：改映射（`flush_tlb_addr` / `flush_tlb_nosweep`）、切地址空间（ASID，AArch64
  上不做 TLBI）、加载代码（`dcache_clean_code_range` + `invalidate_icache_all`）、交给设备
  （`dcache_flush_range`）。
- 在 SMP 上，一律使用**广播（`is`）**形式。

**下一步：** 理解了内存一致性之后，[15 调试与进阶](15-debug.zh.md) 会讲当一致性 bug 真的漏到
真机上时，如何挂上 GDB、如何读日志栈。
