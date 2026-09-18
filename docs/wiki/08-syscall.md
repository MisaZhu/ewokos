# 08 System Calls

> Language: **English** | [中文](08-syscall.zh.md)
>
> Goal: understand how user programs safely request kernel services. Follow
> a single `getpid()` through the complete "user mode → kernel → user mode"
> journey, and read the syscall table and dispatcher.
> This chapter is code set #5 on the hands-on main line: the wormhole of
> spacetime travel — a user process, living in its own spacetime, hands a
> request through a tiny hole into the kernel's spacetime.
> Terminology (svc/syscall number/ABI…) is in the [glossary](99-glossary.md).

## 8.1 The Point of System Calls: the Gate Only Opens a Crack

Ask "why" first. User programs are locked in a low-privilege cage (Ch. 04):
they can't touch hardware, can't modify page tables, can't touch kernel
memory. But programs must get work done — read files, allocate memory,
create processes. **A contradiction appears: the cage must be solid, yet the
work must get done.**

System calls are the resolution of that contradiction, with three layers of
meaning:

1. **The single safe entrance**: users cannot execute privileged operations
   themselves; they can only "ask". Every request is vetted by the kernel
   one by one (are the arguments legal? is this process entitled?), and the
   kernel is the sole executor. The cage wall has no hole — only a door
   crack with a security check;
2. **A stable abstract interface**: programs write `open()`/`fork()`
   without knowing how the kernel implements them or what hardware model is
   underneath. As long as the interface stays, the kernel implementation
   can evolve freely;
3. **The portability boundary**: different chips enter the kernel with
   different instructions (ARM uses `svc`, x86 uses `syscall`), but the
   C library interface above is identical — EwokOS can support
   ARM32/AArch64/RISC-V/x86 simultaneously precisely because of this
   boundary.

Recall Ch. 04: a user program (EL0) can't touch hardware, can't modify page
tables, can't touch kernel memory. The kernel opens a **controlled door
crack**: the `svc` (Supervisor Call) instruction. Any program may execute
it, but after execution control passes to the kernel, and the kernel
decides what to do.

```
User program:  "I want to print a string"
    │
    │ executes  svc #0          (the one and only key)
    ▼
CPU:  raises a synchronous exception → enters the kernel (EL1)
      → jumps to the corresponding vector-table entry
    ▼
Kernel:  checks the request number and arguments → performs the service
         → writes back the result
    ▼
User program:  picks up the return value and keeps running
```

## 8.2 The User Side: Issuing a System Call

EwokOS's userland C library unifies all system calls into 4 macros
([syscall.h](../../system/basic/libc/libewoksys/ewoksys/include/ewoksys/syscall.h)):

```c
// A unified entry with at most 3 arguments
extern ewokos_addr_t syscall3_raw(int code, arg0, arg1, arg2);

#define syscall3(code, arg0, arg1, arg2) syscall3_raw(code, arg0, arg1, arg2)
#define syscall2(code, arg0, arg1)       syscall3_raw(code, arg0, arg1, 0)
#define syscall1(code, arg0)             syscall3_raw(code, arg0, 0, 0)
#define syscall0(code)                   syscall3_raw(code, 0, 0, 0)
```

The low-level implementation is as short as it gets
([syscall_aarch64.S](../../system/basic/libc/libewoksys/ewoksys/src/syscall_aarch64.S)):

```asm
.global syscall3_raw
syscall3_raw:
    svc #0        ; that's it! Per the ARM calling convention:
    ret           ; x0=number  x1=arg1  x2=arg2  x3=arg3
                  ; the return value comes back in x0
```

Arguments travel in registers: `x0` holds the syscall number, `x1~x3` hold
the arguments, and the result returns in `x0`. Much faster than passing
arguments through memory, and no shared-memory structure is needed.

## 8.3 Syscall Numbers: One Master Table

All numbers are defined in
[syscalls.h](../../kernel/kernel/include/syscalls.h) as a simple enum
(excerpt):

```c
enum {
    SYS_NONE = 0,
    SYS_KPRINT,          // kernel console printing
    // process memory management
    SYS_MALLOC_EXPAND,   // expand the heap
    SYS_FREE,
    // process lifecycle
    SYS_EXEC_ELF,        // load an ELF executable
    SYS_FORK,            // create a child process
    SYS_THREAD,          // create a thread
    SYS_YIELD,           // voluntarily yield the CPU
    SYS_WAIT_PID,        // wait for a child to finish
    SYS_USLEEP,          // sleep
    SYS_EXIT,            // exit
    SYS_SIGNAL_SETUP,    // signals
    SYS_GET_PID,
    // IPC (Ch. 09)
    SYS_IPC_SETUP, SYS_IPC_CALL, SYS_IPC_END, ...
    // shared memory / semaphores / DMA ...
    SYS_CALL_NUM         // total count (about 60)
};
```

Note the count: **only about 60**. Compare with Linux's 300+ — this is the
microkernel showing: the filesystem, networking, and device control all
live outside the kernel and go through IPC (Ch. 09). The kernel keeps only
the things that "cannot be done without privilege".

## 8.4 The Kernel Side: the Dispatcher

After the `svc` exception enters the kernel and the assembly has saved the
context, it ultimately calls
[the kernel's dispatcher](../../kernel/kernel/src/svc.c) (simplified):

```c
static inline void _svc_handler(int32_t code, ewokos_addr_t arg0,
                                ewokos_addr_t arg1, ewokos_addr_t arg2,
                                context_t* ctx) {
    switch(code) {
    case SYS_EXIT:
        sys_exit(ctx, arg0);
        return;
    case SYS_KPRINT:
        sys_kprint((const char*)arg0, arg1);   // print the string the user gave
        return;
    case SYS_GET_PID:
        ctx->gpr[0] = sys_getpid(...);          // result goes into x0 → back to the user
        return;
    case SYS_IPC_CALL:
        ...                                     // IPC (Ch. 09)
    ...
    }
}
```

Three key points:

1. **The return-value convention**: write the result into `ctx->gpr[0]`
   (i.e. the `x0` register at scene-restoring time). Success returns 0 or a
   positive number (like a new pid); failure returns a negative number
   (like `-1`);
2. **Safety**: before the kernel dereferences a pointer argument from the
   user, it must confirm the pointer actually lies within that process's
   address space — otherwise one malicious program could make the kernel
   read or write arbitrary memory;
3. **SMP protection**: dispatch is serialized with `kernel_lock()` before
   and after (read-only query-type syscalls take a fast path without the
   lock — see `svc_is_query_fastpath`).

## 8.5 The Complete Journey: the Life of a `getpid()`

Now string the whole chain together:

```
① application code:  pid = getpid();
② C library (libc):  syscall0(SYS_GET_PID)
③ assembly:          x0 = SYS_GET_PID; svc #0
④ CPU hardware:      switch to EL1, jump to the vector table's
                     synchronous-exception entry
⑤ interrupt.S:       SAVE_IRQ_CONTEXT (save all registers)
⑥ svc.c:             _svc_handler(code=SYS_GET_PID, ctx)
                     ctx->gpr[0] = the current process's pid
⑦ interrupt.S:       RESTORE_IRQ_CONTEXT + eret
⑧ application code:  picks up the pid from x0 and continues
```

The cost of one system call = one exception round-trip. This is why a
microkernel must control the number of calls (merge what can be merged, use
shared memory instead of copying where possible — Ch. 13's graphics system
will demonstrate this).

## 8.6 Interrupts vs. System Calls: Same and Different

By now you'll notice: the paths of a system call (Ch. 08) and an interrupt
(Ch. 06) are almost identical — both are "save the scene → handle in the
kernel → restore the scene → `eret`". This is no coincidence: **they are
two uses of the very same exception mechanism**:

| | System call (SVC) | Interrupt (IRQ) |
|---|---|---|
| Who initiates | **The program itself**: executes `svc` | **Hardware**: a device raises the interrupt line |
| When it happens | At a deliberate moment, when the program wants something | Random; the program is unprepared |
| Intent | Request a service ("do work for me") | Report an event ("something happened") |
| Vector-table entry | Lower-level sync exception (+0x400) | Lower-level IRQ (+0x480) |
| Can it be masked | No (the program needs it itself) | Yes (mask interrupts / mask bits) |
| After return | Must continue exactly as before (carrying the result) | May resume with a different process |

What's the same is the **channel**: the hardware mechanism for entering and
leaving the kernel, the scene saving, the `eret` return — all reuse the
same code. What differs is the **semantics**: one is "please come in"
(voluntarily passing security), the other is an "alarm" (passively
responding).

Once you understand this, the operating system's core picture is complete:
**the kernel never does anything on its own initiative — it only wakes at
two kinds of moments: broken into by hardware (interrupts), or knocked on
by programs (system calls). It wakes, does the job, then goes back and lets
some process keep running**. The scheduler is merely "on the way back,
conveniently deciding who runs".

## 8.7 Error-Handling Conventions

- A failed system call uniformly returns a negative number: `-1` for
  general failure; special scenarios have dedicated values (e.g.
  `IPC_ERROR_RETRY = -1` asks the caller to retry);
- The C library layer translates negatives into the standard `errno`
  ([errno.h](../../system/basic/libc/libgloss/errno.h)), and user programs
  check it the POSIX way:

```c
int fd = open("/a.txt", O_RDONLY);
if(fd < 0) {
    // errno has been set by the library function
    slog("open failed: %d\n", errno);
}
```

## 8.8 Exercises

1. Trace a system call with GDB: set a breakpoint on `_svc_handler` in
   `svc.c`; after boot, any process executing a syscall will stop there.
   `p code` shows the number — look it up in `syscalls.h`;
2. Count how many system calls happened behind the boot log line
   `init: /sbin/core [ok]` (hint: `fork`, `SYS_EXEC_ELF`, `SYS_IPC_*`…);
3. Food for thought: why may `SYS_KPRINT` be callable by any process, while
   `SYS_MEM_MAP` (mapping device memory) should only be given to trusted
   driver processes? How should the kernel tell them apart? (Hint: look at
   the `proc->info.uid` checks in `svc.c`.)

## 8.9 Summary

- The point of system calls: the cage must be solid yet work must get done
  — a door crack with security + a stable abstraction + a portability
  boundary;
- `svc #0` is the single gate from user mode into the kernel; the number
  and arguments travel in registers;
- The syscall table has only about 60 entries — the microkernel pushes
  every other service out to IPC;
- The dispatcher `_svc_handler` switches on the number and writes results
  into `ctx->gpr[0]`;
- System calls and interrupts share the exception channel; they differ only
  in initiator (voluntary request vs. hardware event);
- Failures return negative numbers; the C library translates them into
  `errno`.

Next chapter: the microkernel's true core mechanism — IPC. Without it,
files, windows, and networking simply don't exist.
