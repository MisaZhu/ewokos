# EwokOS OS Tutorial: From Zero to Your Own OS on a Raspberry Pi

> Language: **English** | [中文](README.zh.md)

This is a tutorial written for readers with **absolutely no operating-system background**.
We start from "what an OS actually is" and, step by step, read and build every part of
EwokOS with our own hands — ending with a system you helped build, running on a real
Raspberry Pi.

EwokOS is a real, runnable microkernel operating system (ARM32 / AArch64 / RISC-V / x86)
with a modest, clearly structured codebase — an ideal learning vehicle. Every code
walkthrough in this tutorial maps directly onto real source files in this repository;
there are no toy examples.

## Introduction: Operating Systems Are Not That Hard

Many people say OS development is hard. What is actually hard is the *span*: between
"can write hello world" and "can read a real kernel" there is a huge gap. This tutorial
slices that gap into small steps, each with a visible result: first print, first
interrupt, first crossing into userland, first two processes taking turns... You will
gradually realize that an interrupt is just a table of function pointers, virtual
memory is just a translation table, a system call is just a controlled jump, and fork
is just copying a data structure.

The tutorial has a **hands-on main line**: eight milestones ("eight code sets"),
evolving from bare-metal UART printing all the way to a mini microkernel with
multi-processing, communication, and reproduction — each one mapping to the real
EwokOS implementation:

| Set | Milestone | Chapters |
|---|---|---|
| #1 | Bare-metal UART printing: nothing beats seeing it run the first time | [03](03-baremetal.md) |
| — (deep dive) | First contact with ARM instructions and the Pi; don't hesitate, assembly is really simple; mixing C and asm, argument passing; first reading of a device interface (UART) and its registers | [03](03-baremetal.md) |
| #2 | Interrupts and clocks: the heartbeat that brings an OS to life | [06](06-interrupt.md) |
| #3 | Memory and addresses: the meaning and implementation of virtual addresses; ARM execution states and privileges | [05](05-mmu.md), [04](04-exception.md) |
| — (the crossing) | A magical journey through space-time: from kernel mode into userland, and what happens on the way | [04](04-exception.md) (§4.6) |
| #4 | The OS's first child: the three essentials of a process and its creation | [07](07-process.md) (§7.1–7.2) |
| #5 | The wormhole through space-time: system calls; interrupts vs system calls | [08](08-syscall.md) |
| #6 | The hand of God: process run states, timer interrupts, and scheduling | [07](07-process.md) (§7.3–7.5), [06](06-interrupt.md) |
| #7 | The essence of a microkernel: how IPC works, data-exchange efficiency, communication as service | [09](09-ipc.md) |
| #8 | Life multiplies: fork fission, the parent/child parting of ways, copy-on-write | [07](07-process.md) (§7.6) |

## Roadmap

```
Part 1  Getting started (concepts + environment + first bare-metal program + assembly primer)
   │
Part 2  The kernel (exceptions → memory → interrupts → processes → system calls → IPC → Capability)
   │
Part 3  Userland (filesystem → init/shell → writing your own apps)
   │
Part 4  The full system (graphics → building images → flashing a real Pi)
   │
Part 5  The ewokos library series (ecosystem overview → libc → common libs → extra libs)
```

## Table of Contents

### Part 1: Getting Started

| Chapter | Contents | You will gain |
|------|------|----------|
| [01 Meet the Operating System](01-intro.md) | What/why an OS, microkernel vs monolithic, why write a microkernel, the EwokOS source map | A mental model of the whole system |
| [02 Setting Up the Environment](02-setup.md) | Cross toolchain, QEMU, hardware prep (Pi / SD card / USB-TTL wiring), first build and run | Seeing EwokOS boot for the first time |
| [03 The First Bare-Metal Program](03-baremetal.md) | Pi boot process, MMIO, a hand-written UART program; ARM instructions and registers, mixing C and assembly | Code set #1: your very first line of kernel code |

### Part 2: The Kernel

| Chapter | Contents | Source |
|------|------|----------|
| [04 CPU Privilege Levels & Exceptions](04-exception.md) | EL0–EL3, the exception vector table, context saving, crossing from kernel mode to userland | `kernel/platform/aarch64/arch/v8/boot.S`, `interrupt.S` |
| [05 Memory Management & the MMU](05-mmu.md) | The meaning of virtual memory, page tables, the kernel memory layout, physical page allocation | `kernel/kernel/src/mm/` |
| [06 Interrupts & Timers](06-interrupt.md) | Pi interrupt sources, the four-step interrupt handling, the clock heartbeat | `kernel/kernel/src/irq.c`, `interrupt.c` |
| [07 Processes & the Scheduler](07-process.md) | The three essentials of a process, creating the first process, context switch, scheduling, fork and copy-on-write | `kernel/kernel/src/proc.c`, `schedule.c` |
| [08 System Calls](08-syscall.md) | Why system calls exist, `svc #0`, dispatching, syscalls vs interrupts | `kernel/kernel/src/svc.c` |
| [09 IPC: Inter-Process Communication](09-ipc.md) | How IPC works, data-exchange efficiency, synchronous RPC, communication as service | `kernel/kernel/src/ipc.c` |
| [21 The Capability Permission Model](21-capability.md) | Capability tickets, the cnode, kernel checkpoints, mint/grant delegation, the declarative `/etc/cap.json` policy | `kernel/kernel/src/cap.c`, `system/basic/sys/init/cap_policy.c` |

### Part 3: Userland

| Chapter | Contents | Source |
|------|------|----------|
| [10 Filesystems & Storage](10-filesystem.md) | SD card driver, ext2, the VFS daemon | `kernel/lib/ext2/`, `system/basic/sys/vfsd/` |
| [11 Boot Flow: init and the shell](11-init.md) | The init process, `.rd` boot scripts, terminal sessions | `system/basic/sys/init/init.c` |
| [12 Writing Your Own Apps](12-apps.md) | The userland build system; write a command of your own and run it | `system/basic/bin/` |

### Part 4: The Full System

| Chapter | Contents | Source |
|------|------|----------|
| [13 Graphics & the Window System](13-graphics.md) | Framebuffer, the graphics library, an X-style window system | `system/gui/`, `system/xwin/` |
| [20 The xwm Window Manager](20-xwm.md) | Mechanism/policy separation, the xwm↔xserverd IPC protocol, shared-memory decoration rendering, writing your own WM | `system/xwin/xwm/`, `system/xwin/libs/x/src/xwm.c` |
| [22 The xserverd Compositor: Rendering Flow & Dirty Regions](22-xserverd-render.md) | The shared-memory UPDATE handshake, two-level dirty flags, `x_repaint` step by step, `draw_win` composite paths, dirty-rect packing and flush, cursor/drag fast paths | `system/xwin/drivers/xserverd/` |
| [14 Images & Flashing to a Pi](14-raspi-image.md) | SD card images, partitioning, flashing, booting real hardware | `tools/makesd.sh`, `tools/bootfs/` |
| [15 Debugging & the Road Ahead](15-debug.md) | GDB debugging, the logging stack, what to do next | — |

### Part 5: The EwokOS Library Ecosystem (the ewokos series)

A standalone "series" dedicated to the EwokOS libraries: from libc (the C runtime)
to the assorted common libraries. When you want to write a real application, or
wonder "which library do I use for X", look here.

| Chapter | Contents | Source |
|------|------|----------|
| [16 Library Ecosystem Overview](16-libs-overview.md) | Why the libraries are self-built, the five-layer map, the directory layout, `EWOK_LIBC`/`EWOK_LIB_GRAPH`/`EWOK_LIB_X` linking, the build & install flow | `system/platform/*/make.rule` |
| [17 The EwokOS libc](17-libc.md) | The C-runtime puzzle: newlib + libgloss + libewoksys + openlibm + softfloat; the full call chain of `printf`; errno | `system/basic/libc/` |
| [18 Common Libraries](18-libs.md) | Quick reference for base/graphics/window/network libraries; an "I want to do X → use library Y" table | `system/*/libs/` |
| [19 Extra Libraries](19-extra-libs.md) | Optional libraries in `sw.extra` (SDL2/curses) and `projects` (litehtml/cglm/ferox/portablegl/imgui/ffmpeg/widget++ extended widgets) | `sw.extra/`, `projects/` |

### Appendix

| Chapter | Contents |
|------|------|
| [99 Glossary: A Quick Terminology Reference](99-glossary.md) | Every abbreviation and term: full English name, meaning, first-appearance chapter |

## Hardware Requirements

- Any **Raspberry Pi 2B / 3B / 3B+ / Zero 2W / 4B** (supported by `machines/raspix` in this repository)
- An SD card of at least 4 GB
- A USB-to-TTL serial cable (for watching boot logs — highly recommended)
- No Pi? The entire tutorial can be done in the QEMU emulator.

## How to Use This Tutorial

1. **Read in order.** Each chapter assumes you have understood all previous ones.
2. **Do every "follow along".** All code can be compiled and verified directly.
3. **Read against the source.** Every file link points at a real file in this
   repository — open it side by side.
4. When you hit an unfamiliar abbreviation or term, consult the
   **[99 Glossary](99-glossary.md)** first — it collects the full English names
   and one-line explanations of every term; key terms are also annotated where
   they first appear in the text.
5. Don't look up every ARM instruction in the manual; the tutorial explains the
   important ones in place.

## Conventions

- Lines starting with `> ` are commands typed into a terminal
- Comments inside code blocks are part of the tutorial — read them carefully
- The main line is **AArch64 (64-bit Raspberry Pi)**; 32-bit differences are
  called out separately

Happy learning. By the time you finish every chapter, your understanding of
"how computers actually work" will never be the same.
