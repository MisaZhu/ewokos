# 25 运行时动态加载：静态操作系统上的 `dlopen()`

> 语言: [English](25-dlopen.md) | **中文**
>
> 目标：理解*为什么*一个把所有程序都静态链接的操作系统仍然能提供
> `dlopen()`；看清支撑它的三个部件——一个具备真正 W^X 权限的内核匿名映射窗口、
> 一个缓存同步系统调用，以及用户态的 ELF 动态加载器（`libdl`）加上编译期生成的
> 宿主 ABI 导出表；并跟随一个插件从 `dlopen()` 到函数被调用、再到 `dlclose()`
> 的完整过程。
>
> 本章建立在 [05 内存与 MMU](05-mmu.zh.md)、[07 进程](07-process.zh.md)、
> [08 系统调用](08-syscall.zh.md) 与 [23 缓存维护](23-cache.zh.md) 之上，并与
> [16 库生态总览](16-libs-overview.zh.md)、[17 libc](17-libc.zh.md) 同属库系列。
> 术语集中在[术语表](99-glossary.zh.md)。

前面每一章都假设程序就是一张静态 ELF 镜像：内核把它映射进来，跳到 `_start`，
程序调用的一切都已经链接在里面。这个模型没有*插件*的位置——一个在运行时才被发现
并加载的 `.so` 文件。本章移除这一限制，说明 EwokOS 如何在不放弃静态链接设计的
前提下加入运行时动态加载。

一句话概括：EwokOS **没有**长出一个 `ld.so`。相反，`dlopen()` 是一个普通的库
（`libdl`），它在用户态完成动态链接器工作中插件真正需要的那一部分，架在两个新的
内核原语之上。它在 **AArch64**（`machine.virt`）上实现并通过启动验证；自测程序
`bin/dltest` 在 QEMU 下报告 **94 项检查，0 失败**。

下面每一处代码引用都指向真实文件：

| 层次 | 文件 |
|---|---|
| POSIX 接口头文件 | [`system/basic/libs/dl/include/dlfcn.h`](../../system/basic/libs/dl/include/dlfcn.h) |
| EwokOS 宿主 ABI 扩展 | [`system/basic/libs/dl/include/dl/dl.h`](../../system/basic/libs/dl/include/dl/dl.h) |
| 加载器核心（对象、作用域、生命周期） | [`system/basic/libs/dl/src/dl.c`](../../system/basic/libs/dl/src/dl.c) |
| ELF/动态解析 + 重定位 + W^X 处理 | [`system/basic/libs/dl/src/dl_elf.c`](../../system/basic/libs/dl/src/dl_elf.c) |
| 宿主 ABI 查找（二分 + 注册表） | [`system/basic/libs/dl/src/dl_abi.c`](../../system/basic/libs/dl/src/dl_abi.c) |
| 导出表生成脚本 | [`system/basic/libs/dl/gen_export.sh`](../../system/basic/libs/dl/gen_export.sh) |
| ELF 动态结构体 | [`system/basic/libs/elf/include/elf/elf_dyn.h`](../../system/basic/libs/elf/include/elf/elf_dyn.h) |
| 系统调用号与 ABI 常量 | [`kernel/kernel/include/syscalls.h`](../../kernel/kernel/include/syscalls.h) |
| 系统调用处理 | [`kernel/kernel/src/svc.c`](../../kernel/kernel/src/svc.c) |
| 匿名窗口 + W^X 映射 | [`kernel/kernel/src/mm/mmu.c`](../../kernel/kernel/src/mm/mmu.c)、[`kernel/kernel/include/mm/mmu.h`](../../kernel/kernel/include/mm/mmu.h) |
| libc 的 `mmap`/`mprotect`/`getpagesize` 路由 | [`system/basic/libc/libgloss/compat.c`](../../system/basic/libc/libgloss/compat.c) |
| 自测程序 | [`system/basic/bin/dltest/`](../../system/basic/bin/dltest/) |

---

## 25.1 为什么静态操作系统没有 `ld.so`

在 Linux 上，插件的未定义符号（`printf`，以及主程序自己的函数）之所以能对着
可执行文件解析，是因为可执行文件带有一张 `.dynsym`，由 `-Wl,-E` /
`--export-dynamic` 链接时填充，运行时由动态加载器（`ld.so`）读取。

EwokOS 程序是由 `aarch64-none-elf-ld` 直接产出的完全静态 `ET_EXEC` 链接。对于
`ET_EXEC` 链接，GNU ld **根本不会创建动态节**，所以 `--export-dynamic` 是一个
悄无声息的空操作：没有东西可供加载器读取，也不存在 `ld.so` 去读它。两个后果
塑造了整个设计：

1. 宿主必须**显式发布它的 ABI**——它无法依赖一张自己根本没有的 `.dynsym`（§25.5）。
2. `dlopen()` 必须是**一个普通库**，而不是在 `main` 之前加载的特殊解释器。它运行
   在已经启动的进程内部，使用与其他一切相同的 libc 和系统调用。

于是 EwokOS 保留了静态链接的简单性，把动态加载作为一个*插件设施*加入：
`dlopen()` 加载一个用 `-fPIC -shared` 构建的 `ET_DYN` 对象，对着宿主发布的 ABI
做重定位，并交回一个不透明句柄。

---

## 25.2 三部件设计

```
   宿主 app（静态 ET_EXEC）                   插件（ET_DYN .so，-fPIC）
   ┌─────────────────────────┐             ┌──────────────────────────┐
   │ dlopen("/lib/p.so",…)   │             │  .text  (R+X)            │
   │   │                     │             │  .data  (RW)             │
   │   ▼                     │             │  .init_array/.fini_array │
   │ libdl  ────────────────┼── 加载 ────▶│  .dynsym/.hash/.rela.*   │
   │   │  宿主 ABI 表         │             └──────────────────────────┘
   │   │  (gen_export.sh)    │                      ▲        ▲
   │   ▼                     │                      │        │
   │ mmap/mprotect/cache ────┼── 系统调用 ──────────┘        │
   └─────────────────────────┘                                │
                    │                                          │
                    ▼                                          │
   内核: SYS_MMAP_ANON / SYS_MPROTECT / SYS_MUNMAP             │
         SYS_CACHE_SYNC  ─────────────────────────────────────┘
         DL_WINDOW_* 每进程虚拟地址窗口（W^X）
```

- **内核**提供用户态自己无法做到的两件事：让新写入的指令字节可执行
  （`SYS_CACHE_SYNC`），以及分配带*逐页权限*的匿名内存（`SYS_MMAP_ANON` +
  在 `DL_WINDOW_*` 预留区上的 `SYS_MPROTECT`）。
- **libdl** 在用户态完成 ELF 工作：解析、映射、重定位、运行构造函数、管理对象图
  与引用计数。
- **宿主 ABI 表**替代缺失的 `.dynsym`，让插件的未定义符号有可绑定的目标。

---

## 25.3 内核部件之一——`SYS_CACHE_SYNC`：让新代码可执行

AArch64 的缓存是分离的：经 D-cache 写入的字节，在被清理到*统一点*（PoU）并使
I-cache 失效之前，对取指单元是**不可见**的（见 [23 缓存维护](23-cache.zh.md)）。
因此一个把插件 `.text` `memcpy` 进映射的加载器，必须在调用这些字节之前先把它们
发布出去。

EL0 自己做不到这一点：`SCTLR_EL1.UCI` 保持清零，所以用户态的 `dc civac` /
`ic ivau` 会陷入。`SYS_CACHE_SYNC(vaddr, size)` 是唯一的通道：

```c
/* kernel/kernel/src/svc.c */
static int32_t sys_cache_sync(ewokos_addr_t vaddr, uint32_t size) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL || cproc->space == NULL) return -1;
    if(size == 0) return 0;
    if(!user_ptr_ok(cproc, vaddr, size)) return -1;   /* 绝不允许它越过内核 VA */
    /* 逐页遍历该范围，跳过空洞，经内核别名清理 */
    …
    invalidate_icache_all();                          /* 广播（ICIALLUIS） */
    return 0;
}
```

libgloss 把它暴露为 `__clear_cache()`，libdl 在拷贝完每个对象后调用它。在启动日志
里，这正是 `A: ctor ran (.init_array)` 这一行能够出现的原因——插件刚拷贝进来的
构造函数确实执行了。

---

## 25.4 内核部件之二——匿名窗口与 W^X

堆无法表达权限：每个堆页都是 RW+X，而过去在它之上的 `mprotect()` 是空操作，所以
插件代码曾经与 `malloc` 的空闲链表待在同一批页里。本工作的第 6 阶段加入了一个
专用窗口。

`kernel/kernel/include/mm/mmu.h` 在 kmalloc-VM 窗口下方预留了一段每进程 VA 范围，
且**仅在 `#ifdef __aarch64__` 下**：

```c
#define DL_WINDOW_SIZE           (256*MB)          /* 非 aarch64 为 0 */
#define DL_WINDOW_BASE           (KMALLOC_VM_BASE - DL_WINDOW_SIZE)
#define DL_WINDOW_END            (KMALLOC_VM_BASE)
#define DL_WINDOW_SLOT           EWOK_MMAP_ANON_ALIGN   /* 64 KB */
#define DL_WINDOW_SLOTS          (DL_WINDOW_SIZE / DL_WINDOW_SLOT)   /* 4096 */
#define DL_WINDOW_PAGES_PER_SLOT (DL_WINDOW_SLOT / PAGE_SIZE)
```

三个系统调用在其上操作（`kernel/kernel/src/svc.c` →
`kernel/kernel/src/mm/mmu.c`），它们被追加在 `SYS_CALL_NUM` 之前，因此不会移动
任何已有编号：

| 系统调用 | 内核入口 | 行为 |
|---|---|---|
| `SYS_MMAP_ANON(size, prot)` | `dl_window_alloc` | 在每进程槽位图上首次适配，`kalloc_page` + 清零 + 按 `prot` 映射每页；返回窗口 VA，缺失/耗尽时返回 **0** |
| `SYS_MPROTECT(vaddr, size, prot)` | `dl_window_protect` | 对窗口内已映射的整页重设权限，随后 `flush_tlb()` |
| `SYS_MUNMAP(vaddr, size)` | `dl_window_free` | `unmap_page_ref`（引用计数归零时释放页）并清除槽位位 |

有两个细节对正确性至关重要：

- **W^X 是真的。** `dl_map_page` 先按请求的 AP 位映射，然后对不可执行映射调用
  `arch_mark_pte_noexec`。这个顺序是 `set_pte_flags` 逼出来的：在 AArch64 v8 上，
  一个被复用的 PTE 槽会保留上一次映射在那里的 execute-never 位，所以 noexec 这一半
  必须在 `map_page` *之后*施加——与用户栈路径用的是同一套技巧。
- **窗口在退出时释放。** `dl_window_release(space)` 运行在 `proc_funeral` 的
  `TASK_TYPE_PROC` 分支里、`free_page_tables` 之前，所以一个进程死亡时——无论正常
  还是因异常——若仍有对象映射着，也会把每个槽位和页归还给分配器。所有退出路径在
  `space->refs` 归零后都汇入 `proc_funeral`，而 `proc_clone`/`kfork` 通过
  `dl_window_clone` 深拷贝窗口（全新页、权限精确——绝不用写时复制，那会导致缺页
  异常或悄悄破坏 W^X）。

`syscalls.h` 里的 `EWOK_PROT_*` 被定义为与 `<sys/mman.h>` 的 `PROT_*` 相等，所以
libc 可以把 `prot` 直接透传。每个 `dl_window_*` 调用点都由 `#if DL_WINDOW_SLOTS > 0`
守护，因此非 AArch64 内核仍能链接（这些定义会被编译掉，`SYS_MMAP_ANON` 返回 0）。

---

## 25.5 宿主部件——静态二进制所没有的 ABI 表

插件会调用 `printf`，也许还有宿主自己的 `host_callback`。两者都不在插件里。在
Linux 上它们会对着可执行文件的 `.dynsym` 解析；这里宿主用两种方式发布它们（见
[`dl/dl.h`](../../system/basic/libs/dl/include/dl/dl.h)）：

**1. 编译期生成的表。** [`gen_export.sh`](../../system/basic/libs/dl/gen_export.sh)
对 `EWOK_LIBC` 点名的归档（`libewoksys.a`、`libc.a`、`libgloss.a`）执行 `nm`，
产出一个按 `strcmp` 排序的 `(名字, 地址)` C 数组：

```c
extern const char SYM_printf[] __asm__("printf");   /* 真实汇编名，唯一的 C 标识符 */
…
const dl_symbol_t __ewok_dl_abi[] = {
    { "printf", (uintptr_t)(void *)SYM_printf },
    …
};
const uint32_t __ewok_dl_abi_count = …;   /* machine.virt 上约 730 个 */
```

这些引用被产出为**强引用，而非弱引用**——这正是整个机制的关键。一个未定义的*弱*
符号不会让 `ld` 抽取归档成员，所以弱表只会导出宿主自己恰好用到的东西。强引用则迫使
`ld` 在 app 链接 `-ldl` 时精确拉入插件可能调用的那些成员。`__asm__("printf")`
标签技巧让每个声明获得符号真实的汇编名，同时保持 C 标识符唯一，从而避免与 GCC 的
`printf` 内建冲突。`dl_abi.c` 对排好序的表做二分查找。

**2. 运行时注册表。** `dl_host_publish(name, addr)` / `dl_host_unpublish(name)`
让应用导出它*自己拥有*的符号（它的回调、一棵 widget 树、任何来自 `EWOK_LIBC` 之外
的库如 libm 的东西），无需改动构建系统。`dl_host_lookup(name)` 先查注册表，再查
静态表。两者落入同一作用域，排在任何已加载对象之前，恰如 Linux 插件看到主程序的
位置。

该表带版本：`EWOK_DL_ABI_VERSION`（编译期烙进插件的宏）与 `dl_host_abi_version()`
（运行中宿主的值，本身就在核心表里），因此插件可以拒绝一个过旧的宿主，而不是去解
引用一个意料之外的 NULL。

---

## 25.6 libdl——解析、映射、重定位

[`dl_elf.c`](../../system/basic/libs/dl/src/dl_elf.c) 是加载器的引擎。

**解析。** 它校验 `ET_DYN` 头，遍历程序头找 `PT_LOAD`/`PT_DYNAMIC`，读取动态表。
`DT_HASH` 是**强制**的：SysV 哈希表的 `nchain` 是推导 `.dynsym` 长度的唯一途径，
所以只有 GNU 哈希的对象会被拒绝。每个按文件提供的大小（`DT_SYMENT`、`DT_RELAENT`）
跨步访问的结构体都在 [`elf_dyn.h`](../../system/basic/libs/elf/include/elf/elf_dyn.h)
里做了大小编译期断言——尤其 `Elf64_Sym` 恰好是 **24 字节**（`st_name` 是
`Elf64_Word`，不是 64 位）；这一个字段搞错会把后面每个字段都挪到错误的字节上，并在
很久以后表现为一个虚假的“undefined symbol”。

**映射。** 整个加载跨度从窗口映射进来，每个 `PT_LOAD` 段被 `memcpy` 到它的
`p_vaddr`（会检查 ELF 同余式 `(base + p_vaddr) mod PAGE_SIZE == p_offset mod
PAGE_SIZE`；AArch64 `-shared` 产出 `p_align = 0x10000`，所以基址按 64 KB 对齐以
匹配槽位粒度）。`DT_TEXTREL` 会被拒绝——插件必须是位置无关的。

**重定位。** `R_AARCH64_RELATIVE`（1027）修正 `.init_array`/`.fini_array` 及其他
基址相对指针，`R_AARCH64_GLOB_DAT`（1025）填数据 GOT 槽，`R_AARCH64_JUMP_SLOT`
（1026）填 PLT 槽。未定义符号通过遍历搜索作用域解析（§25.7）。绑定总是急切的，所以
一个无法解析的符号会在 `dlopen()` 时带着可读的 `dlerror()` 失败，而不是在首次调用
时才失败。

**保护（W^X 处理）。** 在 `__clear_cache` 之后，`dl_elf_protect` 把每个记录的
`PT_LOAD` 按各自权限重新 `mprotect`——text 变为 R+X，data 变为 RW——使用来自
`getpagesize()` 的真实内核页大小（`machine.virt` 上是 16 KB，它现在经 `sys_info`
报告真实值，而不是硬编码的 4096）。

**init/fini。** EwokOS 自己的 `_start` 从不运行 `.init_array`，所以 `dl_run_init`
是唯一会调用插件构造函数的东西——按依赖顺序（依赖在前），卸载时析构函数按被依赖者
在前的顺序运行。

---

## 25.7 对象图、作用域与搜索路径

[`dl.c`](../../system/basic/libs/dl/src/dl.c) 管理已加载对象。每个 `dlopen` 句柄
递增一次引用计数；同一 soname 打开两次返回同一对象。`DT_NEEDED` 条目会在依赖对象
被重定位之前递归加载，从而跨对象符号得以解析。裸名被当作 soname，沿
`DL_DEFAULT_DIRS` = `/lib:/usr/lib:/system/lib` 搜索（EwokOS rootfs 里没有
rpath）；绝对路径原样使用，且 `/tmp/libfoo.so` 绝不会仅因基名相同就与
`/lib/libfoo.so` 混同。

符号查找为每个请求者构建作用域：先宿主 ABI，再 `RTLD_GLOBAL` 对象，再请求者自己的
依赖树。`dlopen(NULL)` 返回宿主作用域的句柄，于是 `dlsym` 可以查宿主符号。`dladdr`
把一个地址映射回它所在的对象与最近的符号；对宿主代码它返回 0，因为静态宿主不属于
任何对象。

---

## 25.8 libc 路由

`system/basic/libc/libgloss/compat.c` 把内存调用路由到新系统调用，并带回退，因此在
没有窗口的目标上也不会崩：

- `getpagesize()` 从 `sys_info` 返回真实内核页大小。
- `mmap(NULL, len, prot, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)` 调用
  `SYS_MMAP_ANON`；返回 0 表示“无窗口”，libc 回退到旧的零填充堆模拟。一个注册表
  记住哪些地址来自窗口，以便 `munmap` 正确路由。
- `mprotect`/`munmap` 对窗口地址调用 `SYS_MPROTECT`/`SYS_MUNMAP`，否则空操作 /
  释放堆。

---

## 25.9 构建并加载一个插件

插件用 `-fPIC` 编译、用 `-shared -nostdlib`（`make.rule` 里的 `EWOK_SO_FLAGS`）
连同 soname 链接；宿主链接 `EWOK_LIB_DL = -ldl $(EWOK_LIBC)`（`-ldl` 在前，好让它
的强引用拉入插件可能调用的归档成员）。插件安装进 rootfs 的 `/lib`——树里没有别处
往那放东西，所以 `bin/dltest/Makefile` 创建它，`make sd` 用 `e2cp -d` 打包它。

```c
/* plugin.c —— 一个最小对象 */
#include <stdio.h>                 /* printf 对着宿主 ABI 表解析 */
int demo_entry(int x) { return x + 1; }
__attribute__((constructor)) static void init(void) { printf("demo: ctor\n"); }
```

```c
/* host.c */
#include <dlfcn.h>
void* h = dlopen("/lib/libdemo.so", RTLD_NOW);
int (*fn)(int) = (int (*)(int))dlsym(h, "demo_entry");
if(fn) printf("demo_entry(41) = %d\n", fn(41));
dlclose(h);
```

与 glibc/musl 的刻意偏差都在
[`dlfcn.h`](../../system/basic/libs/dl/include/dlfcn.h) 里有记录：`RTLD_LAZY`
被接受但绑定总是急切的；`dlsym()` 拒绝 `RTLD_NEXT`（`RTLD_DEFAULT` 可用，并搜索
全局作用域）；`dlerror()` 是每进程的，而非每线程的。

---

## 25.10 自测程序：“94 项检查，0 失败”证明了什么

[`bin/dltest`](../../system/basic/bin/dltest/) 被写成*证据*而非演示。它打印一个可被
机器 grep 的结论，并在 `machine.virt` 上于启动时运行。它随附的插件包括：
`libdlplug_a.so`（演练每一种重定位类型、`.init_array`/`.fini_array`、一次对象内
PLT 调用、一次宿主回调，以及 `dl_host_lookup`）、`libdlplug_b.so`（对 A 的
`DT_NEEDED`、跨对象 JUMP_SLOT 与 GLOB_DAT）、`libdlplug_bad.so`（一个无法解析的
符号，用以证明错误路径），以及 `libdlplug_m1..m8.so`（同一份源码，各用不同的
`PLUG_ID` 编译）。

通过的运行证明了：

- **缓存同步有效**——`A: ctor ran (.init_array)` 在 `dlopen` 期间打印。
- **宿主 ABI 表能解析** 来自插件的 `printf`/`puts`/`malloc`，注册表能双向解析
  `host_callback`。
- **三种重定位类型都以正确的偏移施加**，依赖被加载，soname 去重为单一共享实例，
  init/fini 顺序为被依赖者在前。
- **W^X 被强制执行**——单独的 `wxfault` 调用向插件只读 text 写入，触发真实的缺页
  异常（`ESR 0x9200004f` = 权限故障，level 3，写），内核杀掉该进程。
- **多对象 + 释放**——第 14 节同时打开八个不同对象（八个不同的加载基址，各自运行
  自己重定位后的代码与数据），全部关闭（`dl_object_count()` 回到 0），再重新打开
  一组新的；只有当窗口槽位*及其*后备页真的被释放时才会成功。
- **退出释放**——一个 `leak` 调用把四个对象保持映射并不 `dlclose` 就退出；紧随其后
  （以及在触发异常的 `wxfault` 之后）运行的完整套件仍然通过，这就是 `proc_funeral`
  在“泄漏式正常退出”和“异常退出”两种情况下都回收了窗口、且没有破坏分配器的证据。

---

## 25.11 局限

- W^X 窗口**仅限 AArch64**；其他架构回退到堆模拟（RWX，无逐页权限）。`dlopen`
  在这些架构上依然可用——x86 的 dltest 套件测试通过——只是插件运行在 RWX 堆里、
  没有内核强制的 W^X，因此 `wxfault` 模式会报告 `W^X is NOT enforced`，而不是触发
  权限缺页异常。
- 没有惰性绑定，没有拷贝重定位（插件读取宿主*变量*会被拒绝；宿主通过 `dlsym` 读
  插件变量则可用），没有 `RTLD_NEXT` 插桩链，插件没有 TLS。
- `dlerror()` 是每进程的，所以两个线程并发失败时可能看到彼此的消息。

---

## 另见

- [05 内存与 MMU](05-mmu.zh.md) —— 窗口嵌入其中的 VA 布局
- [07 进程](07-process.zh.md) —— `proc_funeral` 与退出时释放
- [08 系统调用](08-syscall.zh.md) —— 四个新系统调用如何被派发
- [23 缓存维护](23-cache.zh.md) —— PoU、清理/失效，为什么需要 `SYS_CACHE_SYNC`
- [16 库生态总览](16-libs-overview.zh.md) —— `libdl` 在库栈中的位置
- [99 术语表](99-glossary.zh.md)
