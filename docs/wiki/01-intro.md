# 01 Meet the Operating System

> Language: **English** | [中文](01-intro.zh.md)
>
> No code in this chapter. The goal is to build a mental model: what an OS is,
> why we need one, the microkernel vs monolithic debate, and how the EwokOS
> repository is organized. After this chapter you should be able to answer:
> "Every piece I'm about to write — where does it finally land in the system?"

## 1.1 What Is an Operating System

You open your phone to order takeout: the screen lights up, an app reads your
location, data flows over the wireless network, results are saved to local
files — while music keeps playing and messages keep arriving in the background.
The app itself can do **none** of these things — WeChat doesn't drive the
screen, Meituan doesn't drive the network card. They know only one thing:
how to **ask**. The one actually doing the work is the operating system.

A one-sentence definition:

> **An operating system is the first program to run on the hardware, and the
> one program that never exits. It manages the hardware and provides services
> to every other program.**

Three keywords: **first** (every other program is started by it), **never
exits** (present from power-on to power-off), **manage + serve** (downward it
drives the hardware, upward it serves the applications).

Think of it as the head steward of a restaurant: assigning cooks to stoves
(**process scheduling**), giving each table its own dining area (**memory
management**), passing words between the dining room and the kitchen
(**inter-process communication**), answering when a guest rings the bell
(**interrupt handling**), and never letting guests walk into the kitchen to
grab a knife themselves (**system call** gatekeeping). Each of these jobs has
a corresponding hands-on chapter in this tutorial — see the roadmap at the
end of this chapter.

## 1.2 Why Do We Need an Operating System

Three scenarios showing the chaos without one:

**Scenario 1: two programs fight over the CPU.** With no rules, either they
queue up (one infinite loop ends everything), or we rely on programs yielding
politely (whoever forgets to yield hogs it; a malicious one never does). The
OS answer: clock hardware periodically interrupts the CPU (timer interrupt),
and OS code decides who runs next — fairness enforced by hardware, not by
politeness. → Ch. 06, 07.

**Scenario 2: two programs step on each other's memory.** Program A keeps its
data at address `0x1000`; program B writes to `0x1000` too, and A's data is
corrupted. The OS answer: give every program its own address-translation
table (page table). A's `0x1000` translates to physical page 5, B's `0x1000`
to page 9 — same "street number", different "houses", and out-of-bounds
access is rejected by the hardware itself. → Ch. 05.

**Scenario 3: programs touching hardware directly.** Hardware registers that
any program can read and write mean any program can break the whole system.
The OS answer: the CPU has two "gears" (privilege levels); dangerous
operations are only allowed in kernel mode, and a user-mode program that
wants something done must request it via a system call. → Ch. 04, 08.

Together, these are the three great values of an OS: **hardware-enforced
fairness instead of politeness, isolation by address translation instead of
sharing, and privilege-level gates instead of no defense at all**.

## 1.3 What Actually Happens After Power-On

When a bare machine (take the Raspberry Pi) is powered on, memory is empty
and the CPU knows only one thing: **start executing instructions from a fixed
location**. There is no software, no files, no "system" to speak of.

To make the machine useful, software is layered up from the bottom:

```
┌──────────────────────────────────────────────┐
│  Your applications (browser, games, terminal…) │  ← user mode
├──────────────────────────────────────────────┤
│  System services (filesystem, window system,   │  ← user mode
│  network stack…)                               │    (EwokOS's choice)
├──────────────────────────────────────────────┤
│  OS kernel (processes, memory, interrupts…)    │  ← kernel mode
├──────────────────────────────────────────────┤
│  Hardware (CPU, RAM, SD card, GPU, UART…)      │
└──────────────────────────────────────────────┘
```

The **OS kernel** is the first layer of software laid onto the hardware. Its
job list is short, but every item is bedrock:

| Duty | Description | Chapters |
|------|------|--------------|
| Bootstrapping | Initializes CPU/memory/devices into a usable state | 03, 04, 05 |
| Memory management | Lets each program believe it owns all of memory, safely | 05 |
| Interrupts & exceptions | Timely response to hardware events (keys, timers, page faults) | 04, 06 |
| Process management | Lets many programs run "at the same time" | 07 |
| System calls | A controlled entry point for everything above | 08 |
| Device drivers | Drives the SD card, UART, display, etc. | 10, 13 |

## 1.4 Bare-Metal Programs vs an Operating System

You can write a program that manipulates hardware directly (say, lighting an
LED) — that's a **bare-metal program**. Its traits:

- only **one flow of execution**, no concept of "multitasking";
- all code can reach all memory and all devices — no protection;
- no filesystem; data is either hard-coded or read/written as raw sectors.

An OS = a bare-metal program + **multitasking** + **memory protection** +
**abstraction layers** (files, processes, pipes, ...). In Ch. 03 you will
first write a bare-metal program; every later chapter adds one capability
layer onto it, until it has grown into an operating system.

## 1.5 Microkernel vs Monolithic: The Friendship-Ending Topic

Kernel design has one fundamental disagreement: **how much functionality goes
into kernel mode?** Different answers form two camps.

**Monolithic**: filesystem, drivers, and the network stack are all compiled
into the kernel and run at the highest privilege level together. Linux is
like this. Pros: performance (everything is a direct function call). Cons:
one driver bug can kill the whole system.

**Microkernel**: the kernel keeps only the minimal duties — scheduling,
memory management, interrupt handling, system calls, and inter-process
communication (IPC). Filesystems, drivers, and the network stack all run as
**ordinary user programs**. If a driver crashes, the system survives; just
restart that driver.

This debate is not new. In 1992, Linux's father **Linus Torvalds** and Minix's
author, the operating-system textbook giant **Andrew Tanenbaum**, fought a
historic online battle (the Tanenbaum–Torvalds debate):

- Tanenbaum: microkernels are the future. Monolithic kernels stuff massive
  amounts of code into kernel mode; one driver error crashes the entire
  system — an architecturally obsolete design.
- Torvalds: **performance and practicality** matter more. In a microkernel,
  drivers live in user mode and every hardware access means cross-process
  communication — too costly. Linux runs, works well, and is free; that's
  what counts.

Neither side was wrong; they ranked values differently:

| | Monolithic | Microkernel |
|---|---|---|
| In-kernel calls | Direct function calls, fast | Message passing, a bit slower |
| Reliability | One module crashes, everything dies | One service crashes, only it is affected |
| Security | Huge kernel-mode codebase, big attack surface | Tiny kernel-mode codebase, easy to audit |
| Driver development | Written in-kernel; one landmine = dead machine | Drivers are ordinary programs; crash and retry |
| Portability | Drivers tangled with the kernel, hard to port | Small kernel, easy to port |

That's why the topic "ends friendships" — it's not right vs wrong, it's a
trade-off. Today's phones (Linux/Android) and security chips (often
microkernels) both thrive. And the old "microkernels are too slow" claim was
later defused by the L4 microkernel family's extreme optimizations and by
large-scale commercial use of QNX and Fuchsia.

### Why This Tutorial Teaches the Microkernel

For product builders, the monolithic ecosystem is unbeatable; for **learning
operating systems**, the microkernel has overwhelming advantages:

1. **A kernel small enough to read in full.** The EwokOS kernel core is only
   a few thousand lines, covering exactly the five essential mechanisms of an
   OS. A monolithic kernel has tens of millions of lines; nobody reads that.
2. **A driver crash doesn't kill the machine.** Writing drivers in user mode
   means a bad pointer only kills that one process. During the learning
   phase, this is the watershed between "sense of achievement" and
   "frustration".
3. **You're forced to learn IPC.** In a microkernel, drivers, the filesystem,
   and the window system all cooperate via messages — you'll see the full
   picture of "an OS = a group of processes cooperating through messages".
4. **Your code ends at a microkernel.** This tutorial goes from bare metal
   all the way to a mini multitasking system whose structure matches EwokOS
   exactly.

EwokOS is a **microkernel**. This is the key to understanding every design
in this repository:

```
user mode:  /sbin/init  /sbin/core  /sbin/vfsd  /sbin/sdfsd  /bin/shell
            /drivers/uartd  /drivers/timerd  /drivers/xserverd ...
                 │            │          │            │
                 └────────────┴──── IPC ─┴────────────┘   ← everything by messages
                 ┌────────────────────────────────────┐
kernel mode:     │ scheduler  memory  interrupts  syscalls  IPC │ ← the kernel is only this
                 └────────────────────────────────────┘
```

For example, "reading a file" in EwokOS goes like this:

1. Your application calls `open("/a.txt")`;
2. The library function turns it into an **IPC request** to the filesystem
   daemon `vfsd`;
3. `vfsd` forwards the request to the SD-card filesystem process `sdfsd`;
4. `sdfsd` reads the SD card hardware and returns the data along the same
   path back.

The kernel only moves messages and switches processes the whole time — **it
has no idea what a "file" is**. That mechanism (IPC) is the protagonist of
Ch. 09; it is the microkernel's bloodstream.

## 1.6 The EwokOS Source Map

Open the repository root; the most important directories:

```
ewokos/
├── kernel/                 Kernel source (platform-independent part)
│   ├── kernel/             Core: processes, scheduling, memory, IPC, syscalls
│   │   ├── include/        Kernel headers (proc structs, syscall numbers...)
│   │   └── src/            Implementation: proc.c, schedule.c, ipc.c, svc.c, mm/...
│   ├── platform/           Platform-specific code (arm / aarch64 / riscv / x86)
│   │   └── aarch64/arch/v8/boot.S   ← the first code the CPU executes
│   ├── lib/                Kernel utility libs (strings, printf, ext2 reading, sconf config)
│   ├── dev/                Device interfaces (a unified abstraction of UART, timer, SD)
│   └── loadinit/           Code that loads the init process from the SD card
│
├── machines/               Board support packages (BSPs) for real hardware
│   ├── raspix/             ★ Raspberry Pi 2B/3B/4B (this tutorial's target)
│   │   ├── kernel/         Pi kernel config and drivers (UART, SD, interrupts)
│   │   └── system/         Pi system build entry and boot scripts
│   ├── miyoo/  orangepi/  raspi5/ ...     other boards
│   └── x86/  virt.riscv/
│
├── machine.virt/           QEMU virtual target (most convenient for development)
│   ├── kernel/             Kernel build for the VM
│   └── system/             System build for the VM
│
├── system/                 Userland code (board-independent)
│   ├── basic/              Base layer: libc, init, shell, core commands, base drivers
│   │   ├── libc/           The C library (libewoksys + libgloss)
│   │   ├── sys/            Core system processes: init, core, vfsd, sdfsd, sessiond
│   │   └── bin/            Command-line tools: ls, cat, shell, ps ...
│   ├── network/            TCP/IP stack and network services
│   ├── gui/                Graphics library and framebuffer drivers
│   ├── xwin/               X-style window system (xserverd, window managers)
│   └── platform/           Per-architecture build rules (make.rule)
│
├── sw.extra/               Optional third-party software (SDL2 etc.)
├── tools/                  SD-card creation scripts and boot firmware files
└── docs/                   Documentation (the tutorial you're reading lives here)
```

Remember one main line: **`kernel/` is the generic kernel,
`machines/raspix/` is the Pi-specific layer, and `system/` is the userland
shared by all boards**. The build system stitches them together.

## 1.7 The EwokOS Boot Panorama

Let's skim the whole boot process; each later chapter expands one step:

```
① Power on the Pi; the GPU firmware reads config.txt from the SD card,
   loads kernel8.img (our kernel) at 0x80000 and jumps to it
        ↓
② boot.S: switch the CPU from privilege level EL2 to EL1, mask interrupts,
   set up a stack, enter C code
        ↓
③ start.c/_boot_start: build boot page tables and turn on the MMU
   (virtual memory)
        ↓
④ kernel.c/_kernel_entry_c: initialize the memory allocator and UART,
   read the kernel config file, set up the process table,
   load the first user process /sbin/init
        ↓
⑤ init: starts core services (core, vfsd, sdfsd), then runs the
   /etc/init.rd boot script, bringing up the UART driver, terminal
   sessions...
        ↓
⑥ You see a shell prompt and can type commands
```

## 1.8 Follow Along: Feel a Complete Boot

No need to understand the details yet — just get the system running
(environment not installed yet? Jump to Ch. 02 and come back):

```bash
> cd machines/raspix/system
> make            # build the kernel + all userland + the SD image (first time ~10–30 min)
> make run        # boot a simulated Pi with QEMU
```

If all goes well, you'll see kernel boot logs, the EwokOS ASCII logo, a
series of service-start messages, and finally a command prompt you can type
into. Try `ls /`, `ps`, `uname`.

**Exiting QEMU**: press `Ctrl-A`, then `x`.

## 1.9 Summary

- An OS is the first software layer on the hardware, managing the machine
  with three moves: scheduling, isolation, and gates;
- A bare-metal program has one flow of execution and no protection or
  multitasking — it is the embryo of an OS;
- Monolithic vs microkernel is a trade-off debate; this tutorial chooses the
  microkernel: a kernel small enough to read, driver crashes that don't kill
  the machine, and forced IPC learning;
- EwokOS is a microkernel: the kernel only does scheduling / memory /
  interrupts / syscalls / IPC; drivers and the filesystem are userland
  processes;
- The source has three big parts: `kernel/` (the kernel), `machines/raspix/`
  (the Pi board layer), `system/` (userland).

The tutorial's hands-on main line (each chapter has you build one link):

```
Ch. 03  Bare-metal UART printing   — nothing beats the first successful run
Ch. 03  ARM instructions + asm/C mixing — don't hesitate, assembly is simple
Ch. 06  Interrupts and clocks      — the heartbeat that brings an OS to life
Ch. 05  Virtual addresses & page tables — from the physical into the virtual world
Ch. 04  Crossing from kernel mode to userland — a magical space-time journey
Ch. 07  Processes                  — the OS's first child
Ch. 08  System calls               — the wormhole through space-time
Ch. 07  Scheduling                 — the hand of God
Ch. 09  IPC                        — the microkernel's essence: communication as service
Ch. 07  fork                       — life multiplies
```

Next chapter: we set up the development environment (software + hardware)
and build with our own hands everything you just saw.
