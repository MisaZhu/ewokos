# Appendix: Glossary of Technical Terms

> Language: **English** | [中文](99-glossary.zh.md)
>
> This table collects all abbreviations and technical terms appearing in the
> tutorial. Each entry gives: **full name → meaning → a one-sentence
> explanation → the chapter where it first appears**.
> When you meet an unfamiliar word in the text, come back here; it also
> works as a handbook.

## 1. Hardware and Buses

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **UART** | Universal Asynchronous Receiver/Transmitter | A universal asynchronous transceiver — the "serial port". Sends/receives characters bit by bit at a fixed baud rate; needs no clock line, only TX/RX/GND. | Ch. 02/03 |
| **MMIO** | Memory-Mapped I/O | Memory-mapped I/O. Device control registers are mapped to a memory address range; reading/writing those addresses operates the hardware — ARM's standard way of controlling peripherals. | Ch. 03 |
| **GPIO** | General Purpose Input/Output | General-purpose I/O pins, software-configurable as input or output. The UART's TX/RX are multiplexed from two GPIOs. | Ch. 02 |
| **FIFO** | First In First Out | A first-in-first-out buffer. The UART's TX and RX each carry a small FIFO so the CPU doesn't wait byte by byte. | Ch. 03 |
| **Baud rate** | — | The number of bits a serial port transfers per second. Uniformly 115200 in this tutorial. | Ch. 02 |
| **SD / SDHC** | Secure Digital | Secure Digital cards, the Pi's storage medium, read/written in 512-byte sectors. | Ch. 10 |
| **EMMC** | Embedded MultiMediaCard | The embedded multimedia card controller — the controller the SD card hangs off on the Pi2/Pi3. | Ch. 10 |
| **Mailbox** | — | The hardware message queue between the Pi's CPU and GPU, used to request the framebuffer, query clocks, etc. | Ch. 13 |
| **Framebuffer** | — | A piece of memory storing pixel colors row by row; writing to it displays. | Ch. 13 |
| **DMA** | Direct Memory Access | A hardware engine moving data between memory and devices without going through the CPU. | Ch. 09/13 |
| **Timer** | — | Produces interrupts periodically; the operating system's source of "time". | Ch. 06 |
| **IRQ / FIQ** | Interrupt Request / Fast Interrupt Request | ARM's two classes of hardware interrupts. | Ch. 06 |
| **GIC** | Generic Interrupt Controller | ARM's standard interrupt-dispatch hardware, arbitrating and routing interrupts to the cores. | Ch. 06 |
| **EOI** | End Of Interrupt | The signal that must be sent to the controller after handling an interrupt, or that interrupt won't be reported again. | Ch. 06 |
| **Watchdog** | — | A watchdog timer; resets the system if not fed in time. EwokOS uses it as a backstop for "IPC service hung". | Ch. 09 |

## 2. CPU Architecture and Run States

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **AArch64 / ARMv8** | — | ARM's 64-bit architecture. This tutorial's main target (Pi3/Pi4). | Ch. 01 |
| **EL0~EL3** | Exception Level 0~3 | ARM's four privilege levels. EL0 = user mode, EL1 = kernel (OS), EL2 = virtualization, EL3 = secure monitor. | Ch. 04 |
| **SPSR / ELR** | Saved Program Status Register / Exception Link Register | Automatically save the scene and record the return address when an exception occurs. | Ch. 04 |
| **VBAR** | Vector Base Address Register | Points at the exception vector table. | Ch. 04 |
| **eret** | Exception Return | The exception-return instruction: restores the scene and goes back to the code that triggered the exception (or into user mode). | Ch. 04 |
| **svc** | Supervisor Call | The syscall instruction — the only entrance for user mode to deliberately trap into the kernel. | Ch. 08 |
| **AAPCS64** | ARM Architecture Procedure Call Standard | ARM's 64-bit calling convention: arguments in x0~x7, the return value in x0. | Ch. 03 |
| **LR / FP** | Link Register / Frame Pointer | The link register (x30, holds the return address) / the frame pointer (x29, marks the stack frame). | Ch. 03 |
| **context** | — | The complete register snapshot of an execution flow; the basic unit of scheduling and switching. | Ch. 07 |
| **SMP** | Symmetric Multi-Processing | Multiple peer CPU cores running simultaneously. The Pi3/Pi4 has 4 cores. | Ch. 07 |

## 3. Memory Management

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **MMU** | Memory Management Unit | The hardware inside the CPU that translates virtual addresses into physical ones. | Ch. 05 |
| **VA / PA** | Virtual / Physical Address | Virtual address / physical address. | Ch. 05 |
| **Page** | — | The smallest unit of memory management; 4KB in EwokOS. | Ch. 05 |
| **Page Table** | — | The translation table recording "virtual page → physical page + permissions", maintained by the kernel. | Ch. 05 |
| **PTE** | Page Table Entry | One mapping record of "virtual page → physical page", with permission and attribute bits. | Ch. 05 |
| **TLB** | Translation Lookaside Buffer | The page-table translation cache; the hardware caches recent translations for speed. | Ch. 05 |
| **TTBR0/TTBR1** | Translation Table Base Register | Page-table base registers. TTBR0 handles low addresses (user), TTBR1 handles high addresses (kernel). | Ch. 05 |
| **SCTLR** | System Control Register | Contains the bits switching the MMU/caches on and off. | Ch. 05 |
| **MAIR** | Memory Attribute Indirection Register | Defines attributes like "cacheable memory / device memory". | Ch. 05 |
| **CoW** | Copy-On-Write | At fork, parent and child share physical pages; only the one who writes first triggers a copy — implemented with page reference counts. | Ch. 05/07 |
| **page reference count** | — | The `_pages_ref` array, recording how many page tables reference each physical page — the basis of CoW. | Ch. 05 |
| **kalloc** | — | The kernel's physical-page allocator, in units of 4KB pages, also managing 1KB small blocks. | Ch. 05 |
| **kmalloc** | — | The kernel heap, allocating small objects of any size, like userland malloc. | Ch. 05 |
| **shm** | Shared Memory | Two processes mapping the same physical page, for zero-copy transfer of big data. | Ch. 09/13 |
| **page fault** | — | The exception triggered by accessing an unmapped virtual page; the kernel uses it to allocate physical pages on demand. | Ch. 05 |

## 4. Processes and Scheduling

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **process** | — | A running program instance = code + data + context. | Ch. 07 |
| **pid / uid / uuid** | process/user id, universal unique id | Process number / user number / globally unique identifier (pids get reused; uuids don't). | Ch. 07 |
| **READY/RUNNING/BLOCK/SLEEP/ZOMBIE** | — | The process state machine: ready / running / blocked / sleeping / zombie. | Ch. 07 |
| **fork** | — | Duplicates the current process into a child, returning twice (the parent gets the child's pid, the child gets 0). | Ch. 07 |
| **exec** | — | Replaces the current process's code and data with a new program. | Ch. 11 |
| **scheduler** | — | Decides which process gets the CPU next. EwokOS uses simple round-robin. | Ch. 07 |
| **context switch** | — | The procedure of saving one process's registers and loading another's. | Ch. 07 |
| **zombie** | — | A process that has exited but not yet been reaped by its parent; occupies a process-table slot. | Ch. 07 |
| **context hijacking** | — | In single-task IPC mode, the kernel directly borrows the service process's main context to execute a callback. | Ch. 09 |

## 5. IPC and the Microkernel

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **IPC** | Inter-Process Communication | The microkernel's core mechanism. | Ch. 09 |
| **RPC** | Remote Procedure Call | IPC's "request-reply" model is a localized RPC. | Ch. 09 |
| **proto_t** | protocol | EwokOS's IPC data-packing structure: variable-length, self-describing, copyable across processes. | Ch. 09 |
| **ipc_task_t** | — | One IPC request's slot in the kernel, with caller info, arguments, and state. | Ch. 09 |
| **ipc_server_t** | — | A service process's IPC state: the entry address, the task-slot array, the wait queue, the thread pool. | Ch. 09 |
| **call_id** | — | The IPC command number, identifying which function of the service is being called. | Ch. 09 |
| **IPC_LAZY / IPC_NON_RETURN** | — | Call modifier bits: lazy scheduling / no return value needed (one-way notification). | Ch. 09 |
| **IPC_MULTI_TASK / MULTI_CORE** | — | The multithreaded/multicore service flags, deciding that requests are handled concurrently with a thread pool. | Ch. 09 |
| **single-task service** | — | A service where the kernel hijacks the main context to handle requests serially (like most drivers). | Ch. 09 |
| **thread pool** | — | A set of worker threads a multithreaded service hatches on demand and reuses. | Ch. 09 |
| **microkernel** | — | The kernel keeps only minimal functionality (scheduling/memory/interrupts/IPC); everything else lives in userland. | Ch. 01 |
| **monolithic kernel** | — | The traditional architecture where drivers and filesystems are compiled into the kernel (like Linux). | Ch. 01 |
| **userland driver** | — | A driver running as an ordinary process, serving others via IPC. | Ch. 01/09 |
| **capability** | — | A ticket of "object type + rights bits + object identifier"; a process may perform the corresponding privileged operation only by holding it — the microkernel's fine-grained permission mechanism. | Ch. 21 |
| **cnode** | capability node | Each process's capability table (64 slots); the kernel checks tickets slot by slot on privileged operations. | Ch. 21 |
| **CAP_ROOT** | — | The root-authority master ticket; holding it passes every capability check directly. Only issued to kernel processes and trust boundaries like login. | Ch. 21 |
| **mint / grant / revoke** | — | The three syscalls for minting / forwarding / revoking capabilities; minting is centralized in CAP_ROOT, forwarding can only attenuate, and what's given is a copy. | Ch. 21 |
| **cap.json** | — | The declarative permission policy under `/etc`: init pours it into the kernel, and the kernel automatically grants caps by matching cmd on every exec. | Ch. 21 |
| **Principle of Least Privilege** | — | Every component gets only the minimum permissions necessary for its job, so the damage from a failure is also fenced into the minimum range. | Ch. 21 |

## 6. Filesystems

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **VFS** | Virtual File System | The abstraction layer unifying "everything is a file" (path → service). | Ch. 10 |
| **inode** | index node | A file's "dossier": size, permissions, data-block pointers — not including the filename. | Ch. 10 |
| **ext2 / ext3** | Second/Third Extended filesystem | The classic Linux filesystem formats. EwokOS uses them to organize the SD card. | Ch. 10 |
| **superblock** | — | A filesystem's global metadata: block size, inode count, the magic number, etc. | Ch. 10 |
| **block group** | — | The management unit ext2 divides a disk into; each contains bitmaps, an inode table, and data blocks. | Ch. 10 |
| **block bitmap / inode bitmap** | — | Bit arrays marking which blocks / inodes are occupied. | Ch. 10 |
| **directory entry** | — | A "filename → inode number" record inside a directory file. | Ch. 10 |
| **sector** | — | The smallest unit of block-device read/write; 512 bytes for SD cards. | Ch. 10 |
| **block** | — | The smallest unit of filesystem read/write; often 1KB/4KB for ext2. | Ch. 10 |
| **MBR / GPT** | Master Boot Record / GUID Partition Table | Two disk partition-table formats, recording where partitions start. | Ch. 10 |
| **mount point** | — | Attaching a filesystem service to a node of the directory tree. | Ch. 10 |
| **vfsd / sdfsd / ramfsd** | — | The router / the SD-ext2 service / the RAM-disk service. | Ch. 10 |

## 7. Graphics and Windows

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **graph_t** | — | The canvas structure: a pixel buffer (often shared memory) + width/height + a clip region. | Ch. 13 |
| **g2d** | 2D Graphics Engine | 2D graphics acceleration, handing fills/bitmap copies to a dedicated engine. | Ch. 13 |
| **blt / blit** | Block Transfer | Copying a block of pixels from a source canvas to a destination canvas. | Ch. 13 |
| **compositing** | — | The process of stacking multiple window canvases by layer into the final screen image. | Ch. 13 |
| **xserverd** | — | The X-style window server: maintains the window tree, composites, and dispatches input events. | Ch. 13 |
| **xwm** | X Window Manager | The window manager — an independent userland process handling title bars, borders, dragging, and themes. | Ch. 13, 20 |
| **mechanism vs policy** | — | Stable rules (compositing/dispatch) stay in the server; changeable looks (decorations/themes) go to the replaceable xwm. | Ch. 20 |
| **wsr / winr** | Workspace / Window Rect | The workspace rect (where the app draws) / the window outer-frame rect (including title bar, borders, shadow; computed by xwm). | Ch. 20 |
| **frame_g** | Frame Graph | Each window's decoration-layer shared-memory canvas, on which xwm draws the title bar/buttons/borders. | Ch. 20 |
| **frame areas (hot zones)** | — | The hit-test rectangles for the title bar, close/maximize/minimize buttons, and resize corner — computed by xwm, cached by xserverd. | Ch. 20 |
| **theme** | — | A window-appearance style pack (ewokwm, mac1984, openlook, etc.), chosen by `XTHEME`. | Ch. 13 |
| **ARGB** | Alpha-Red-Green-Blue | 32 bits per pixel: 8 bits of alpha + three color channels. | Ch. 13 |
| **font rendering** | — | Turning character outlines into pixel rasters, provided by the fontd service. | Ch. 13 |

## 8. General Concepts

| Term | Full name | Meaning and explanation | Appears in |
|---|---|---|---|
| **ELF** | Executable and Linkable Format | The program binary format common to Linux/Unix. | Ch. 07 |
| **BSS** | Block Started by Symbol | The uninitialized global-data segment; zeroed at load time and taking no space in the image. | Ch. 03 |
| **linker script (lds)** | — | The config file telling the linker where in memory each section goes. | Ch. 03 |
| **cross-compiling** | — | Compiling on one kind of machine (e.g. a Mac) a program that runs on another (ARM). | Ch. 02 |
| **toolchain** | — | The collection of cross-compiler + assembler + linker (e.g. aarch64-none-elf-gcc). | Ch. 02 |
| **QEMU** | Quick Emulator | The open-source hardware emulator, running the system without a real machine. | Ch. 02 |
| **rootfs** | Root Filesystem | The root filesystem image, containing all of the system's files, flashed into the SD card. | Ch. 12 |
| **init** | — | Process number 1 (0 in EwokOS), the ancestor of all user processes, responsible for bringing up the whole system. | Ch. 11 |
| **shell** | — | The command-line interpreter, reading user input and executing commands. | Ch. 11 |
| **spinlock** | — | A busy-waiting mutex, suited to the kernel's short critical sections (especially under SMP). | Ch. 07/09 |
| **critical section** | — | A code segment only one execution flow may enter at a time. | Ch. 07 |
| **atomic operation** | — | An operation that cannot be interrupted — either fully done or not done at all. | Ch. 07 |

---

> **Usage tip**: on your first read-through, when you meet an abbreviation,
> guess the gist and keep going; after finishing a chapter, come back and
> check this table — the terms will be internalized quickly. Key terms are
> also annotated in place at their first appearance in each chapter.
