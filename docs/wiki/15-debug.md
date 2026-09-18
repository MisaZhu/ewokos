# 15 Debugging Techniques and the Road Ahead

> Language: **English** | [中文](15-debug.zh.md)
>
> Goal: master the debugging methods essential for kernel development, and
> get a roadmap for what to do after finishing the tutorial. By this point,
> you have walked the entire path from bare metal to a complete operating
> system.

## 15.1 The Three Logging Channels

EwokOS's logging system has three layers (from low to high):

| Channel | Who uses it | Where it goes |
|------|--------|----------|
| kernel `printf`/`kout` | kernel code | writes the UART (serial) directly; always available |
| `klog`/`kout` (userland) | early boot, urgent debugging | goes through the `SYS_KPRINT` syscall to the kernel serial port |
| `slog`/`sout` (userland) | programs in normal operation | written to `/dev/log` (logd's 64KB ring buffer) |

Usage (userland programs):

```c
#include <ewoksys/klog.h>
klog("urgent: %d\n", x);   // straight to the kernel serial port (works even before the log daemon is up)
slog("normal log: %s\n", s); // into /dev/log
```

Once the system is up, you can read the log buffer:

```
> cat /dev/log
```

Rule of thumb: **whichever stage you suspect, add logs at that stage's
entry and exit**. Use `printf` for kernel-level code and `slog` for
userland.

## 15.2 Debugging the Kernel with GDB

QEMU + GDB is the Swiss Army knife of kernel debugging (the environment was
set up in Ch. 02):

```bash
# terminal 1
> cd machines/raspix/kernel
> make debug          # QEMU stops at the first instruction, waiting for GDB

# terminal 2
> make gdb            # connects automatically
```

Common operations:

```
(gdb) b _kernel_entry_c     # break at the kernel's C entry
(gdb) c                     # continue
(gdb) b proc.c:800          # break by file and line
(gdb) bt                    # view the call stack
(gdb) p *proc               # print a struct
(gdb) x/10i $pc             # disassemble at the current position
```

Tips:
- `make asm` generates the complete disassembly `kernel.asm` for checking
  problems against the assembly;
- When the system hangs, attach GDB and press `Ctrl-C`; `bt` shows at a
  glance where it's stuck;
- Breaking near `panic` / `halt` catches the crash's first scene.

## 15.3 Built-in Diagnostic Commands

Handy diagnostic tools after boot:

```
> ps          # process list: pid, state, memory, CPU time
> svcinfo     # registered IPC services
> sysinfo     # system info (memory, hardware)
> df          # the mount table
> dump <pid>  # dump a given process's info
> elfinfo     # inspect an ELF file's structure (a great helper while studying Ch. 07)
> mmio        # read/write MMIO registers directly (for driver debugging)
```

## 15.4 Quick Reference of Common Pitfalls

| Symptom | Cause and fix |
|------|-----------|
| Changed code but behavior didn't change | forgot to `make sd` to repack the image (stressed in chapters 12 and 14) |
| Build error: compiler not found | the toolchain isn't installed / not in PATH (Ch. 02) |
| QEMU shows a black screen | check the `-serial` argument; `Ctrl-A x` to exit |
| No serial output on a real machine | the serial cable isn't cross-connected; `config.txt` lacks `enable_uart=1` |
| Real machine won't boot but QEMU is fine | check whether `kernel8.img` is the latest artifact for that arch; Pi4 and Pi3 have different MMIO |
| Boot hangs halfway | watch the last log line on serial to locate which service failed to start |

## 15.5 The Road Ahead: What to Do Next

You've mastered the main line. Below are directions from easy to hard:

### Entry Level
1. **Add a system call**: add a number in `syscalls.h`, a branch in
   `svc.c`, and a wrapper in `libewoksys` — walk the whole "kernel
   interface extension" flow;
2. **Write your own driver**: e.g. GPIO LED control (`/dev/led`),
   following Ch. 12's device-service routine;
3. **Add argument parsing and colored output to `hello`**, getting familiar
   with shell and terminal capabilities.

### Intermediate Level
4. **Read the network stack** (`system/network/`): from the NIC driver to
   TCP layering — an excellent sample of a complete protocol stack on a
   microkernel;
5. **Study CoW and shared memory** (`kernel/kernel/src/mm/`): write tests
   verifying the copy-on-write behavior after fork;
6. **SMP multicore**: read the secondary-core boot (`slave_core` in
   `boot.S`), IPIs and spinlocks, and try changing the scheduling policy
   (e.g. a priority queue).

### Challenge Level
7. **Implement a new filesystem** (e.g. a simple FAT) as a new mount-point
   service;
8. **Write a new theme or a new app for the window system** (refer to
   `projects/` and `sw.extra/`);
9. **Port to a new board**: write boot code and drivers following any set
   in `machines/` — this is an OS engineer's graduation project.

### Further Reading
- *ARM Architecture Reference Manual (ARMv8-A)* — authoritative but heavy;
  consult as needed;
- The Raspberry Pi BCM2837/BCM2711 peripherals manuals — a must for writing
  Pi drivers;
- Classic textbooks: OSTEP (*Operating Systems: Three Easy Pieces*),
  *Modern Operating Systems*.

## 15.6 Closing Words

Looking back at the road these 15 chapters have walked:

```
Ch. 03   you wrote your first assembly, making the Pi print "Hello, OS!"
Ch. 04   you understood privilege levels — the source of an OS's power
Ch. 05   you understood page tables — how each process's private world is built
Ch. 06   you understood interrupts — the system gained a heartbeat
Ch. 07   you understood scheduling — multiple programs started living "simultaneously"
Ch. 08   you understood system calls — the contract between user and kernel
Ch. 09   you understood IPC — the microkernel's lifeblood
Ch. 10~12 files, booting, apps — the system grew flesh and blood
Ch. 13   graphics — the system opened its eyes
Ch. 14   you lit up your own operating system on real hardware
```

An operating system is no longer a black box. It is a precision machine
assembled from tens of thousands of "consult the manual, write the
register, save the scene, switch the page table" — and you can now read
every one of its gears.

Go write your drivers, your scheduler, your window system.
