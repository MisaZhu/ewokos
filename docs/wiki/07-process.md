# 07 Processes and the Scheduler

> Language: **English** | [中文](07-process.zh.md)
>
> Goal: understand that a "process" inside the kernel is just a struct; read
> the complete implementation of context switching and the scheduling
> algorithm; understand the fork / exec / exit lifecycle.
> This chapter carries three milestones of the hands-on main line: "code set
> #4" (the first user-mode process), "code set #6" (process scheduling), and
> "code set #8" (fork).
> Terminology (pid/ELF/fork/exec/ZOMBIE/SMP/context…) is in the
> [glossary](99-glossary.md).

## 7.1 What Is a Process

A program is dead code lying on a disk; a **process** is code in motion: it
has a current execution position (the program counter), data (registers,
stack, heap), and its own memory space.

An analogy: a program is a recipe; a process is one act of cooking with that
recipe — the same recipe can be cooked several times simultaneously (the same
program running as multiple processes), and each act has its own progress,
ingredients, and stove.

So to describe a process, the kernel only needs three things:

```c
// kernel/kernel/include/kernel/proc.h (simplified)
typedef struct st_proc {
    proc_info_t   info;      // identity & state: pid, parent, uid, state, core...
    proc_space_t* space;     // memory space: page tables, heap, shm table, IPC server...
    context_t     ctx;       // context: a snapshot of all registers (the CPU scene)
    ...
} proc_t;
```

Here `context_t` is exactly the register snapshot that was pushed onto the
stack during exception handling in Ch. 04 — **pausing a process = storing the
registers into `ctx`; resuming a process = loading `ctx` back into the
registers**. That is the entire magic of context switching.

These are the **three essentials of a process**, and none can be missing:

| Essential | Question it answers | Without it |
|---|---|---|
| `info` (identity & state) | Who is it? What state is it in? | The kernel can't manage it |
| `space` (memory space) | Where does it live? | No isolation; processes stomp on each other |
| `ctx` (register scene) | How far did it get? | Can't pause or resume |

Processes live in the kernel in a fixed-size **process table** (its size is
set by the `max_proc_num` config), initialized at boot by `procs_init()`.

## 7.2 The OS's First Child: How a Process Is Created

Enough theory — let's watch a real "birth". The last step of kernel startup
([kernel.c](../../kernel/kernel/src/kernel.c)):

```c
load_init_proc();   // create the first user process /sbin/init
```

This single step condenses the **entire procedure** of creating a process —
six steps in total:

```
① Find a free slot in the process table, allocate a pid, set state to READY
② Build the memory space: clone a page table (Ch. 05 §5.8) — the high half
  shares the kernel, the low half stays empty
③ Load the content: read /sbin/init's program image (during boot the
  kernel's built-in mini ext2 driver reads the SD card directly — details
  in Ch. 10) and map it into the low half
④ Set up the context: ctx.pc ← the program entry, ctx.sp ← the stack top,
  spsr ← EL0 (the "five-piece crossing set" from Ch. 04 §4.6)
⑤ Put it into the ready queue
⑥ Wait for the scheduler to pick it; proc_switch restores ctx + eret —
  the child is born, living in user mode
```

Note the chicken-and-egg in step ③: the filesystem hasn't started yet, so
`/sbin/init` is **read off the SD card by the kernel itself**. That is why
the kernel keeps a small, trimmed ext2 reader
([kernel/lib/ext2](../../kernel/lib/ext2)) — solely to give birth to the
first child. Once the child is born (init starts `vfsd`/`sdfsd`), this code
retires with honor; nobody ever uses it to load programs again.

From here on, every new process in the system is born from an **existing
process** via `fork`/`exec`, tracing back up the parent chain all the way to
init — the world of processes, like the world of life, has its own genealogy.

## 7.3 The Process State Machine

Every process has a state
([procinfo.h](../../kernel/kernel/include/procinfo.h)):

```
              fork / wakeup
                 │
                 ▼
   ┌──────► READY (queued, waiting for the CPU)
   │            │ picked by the scheduler
   │            ▼
   │        RUNNING
   │            │
   │            ├── time slice expired ───────┐
   │            ├── waiting for an event       │
   │            │   (wait / sleep)             │
   │            │        ▼                     │
   │            │      WAIT (blocked) ─────────┘ event arrives → wake up
   │            ├── finished normally
   │            │        ▼
   │            └──► ZOMBIE (waiting for the parent to collect the body)
   │                         │ parent calls wait
   └─────────────────────────┴──► UNUSED (freed)
```

## 7.4 The Ready Queue and the Scheduler

Each CPU core has a **ready queue** `_ready_queue[core]`, holding the
processes in the READY state. The scheduler's code is surprisingly short
([schedule.c](../../kernel/kernel/src/schedule.c)):

```c
int32_t schedule(context_t* ctx) {
    uint32_t core = get_core_id();
    proc_t* idle_proc = _cpu_cores[core].idle_proc;
    if(idle_proc != NULL) {
        idle_proc->info.state = WAIT;      // the idle process normally sleeps
    }

    proc_t* next = proc_get_next_ready();  // take the next process off the ready queue
    if(next == NULL && idle_proc != NULL)
        next = idle_proc;                  // nobody to run → run idle (spin)

    if(next != NULL) {
        next->info.state = RUNNING;
        proc_switch(ctx, next, false);     // ★ switch over
        return 0;
    }
    halt();                                // not even an idle proc = something is very wrong
}
```

`proc_get_next_ready()` picks a process from the queue by priority and
round-robin rules (EwokOS supports `SYS_PROC_PRIORITY` for adjusting
priorities).

**Who calls `schedule()`?** Recall Ch. 06: every timer interrupt calls it.
So scheduling happens on two occasions:
1. **Time slice expiry** (the timer interrupt — forced);
2. **Voluntary yield**: blocking (waiting for data), `SYS_YIELD`, exiting.

**The idle process**: each core has a special process that does nothing but
loop and sleep. It guarantees "there is always something to run", and it is
also the basis for measuring "CPU idle time".

## 7.5 Context Switching: Reading proc_switch Line by Line

Look at `proc_switch` in [proc.c](../../kernel/kernel/src/proc.c)
(simplified):

```c
void proc_switch(context_t* ctx, proc_t* to, bool quick){
    proc_t* cproc = get_current_proc();     // the current process

    // ① Save the current process's register scene
    memcpy(&cproc->ctx, ctx, sizeof(context_t));

    // ② Swap page tables (if the two processes don't share a memory space)
    //    Writing TTBR0: from now on you "see" the new process's memory world
    if(cproc != to && cproc->space != to->space) {
        page_dir_entry_t *vm = to->space->vm;
        set_translation_table_base(V2P(vm));   // the page-table switch from Ch. 05!
    }

    // ③ If the new process has pending interrupts/signals/IPC requests,
    //    "hijack" its context to the corresponding handler (details in §7.7)
    ...

    // ④ If the old process can still run, put it back on the ready queue
    if(cproc->info.state == RUNNING) {
        cproc->info.state = READY;
        queue_push(&_ready_queue[cproc->info.core], cproc);
    }

    // ⑤ After returning, the exception exit path restores registers from
    //    to->ctx — the new process starts running
}
```

The key is step ⑤: `proc_switch` is called on the exception-handling path.
It places `to->ctx` where the scene-restoring code will pick it up, and the
moment `eret` executes, the CPU "wakes up" with the new process's registers
and page table — **for the new process, it simply continues from the
instruction where it was paused**, completely unaware.

## 7.6 A Process's Life: Life Reproduces

The first process in §7.2 was a special case, hand-crafted by the kernel.
Normally, processes **reproduce like living things**: `fork` spawns a child
identical to itself, `exec` turns the child into someone else, and `exit`
takes the final bow. This corresponds to "code set #8" on the hands-on main
line.

### fork: Process Fission

`fork()`'s semantics are brutally simple: **copy yourself, get an identical
duplicate**. Code, data, stack, open files, which line is executing — all
the same. Even "the next instruction to execute" is the same. So how do
parent and child tell each other apart?

The convention hides in the **return value**: `fork()` returns once in the
parent and once in the child —

```c
int32_t pid = fork();
if(pid == 0) {
    // I'm the child: fork() returned 0
} else if(pid > 0) {
    // I'm the parent: fork() returned the child's pid
} else {
    // negative: creation failed
}
```

One function, two worlds; identities told apart by the return value. This is
one of the most famous designs in operating-system history.

### The Parting of Parent and Child

Once fission completes, parent and child are **two fully independent
processes**:

- Each has its own register scene, each is scheduled on its own; neither
  waits for the other;
- Each has an independent memory view — afterwards, one side writing its own
  variables has no effect whatsoever on the other;
- The only remaining ties are two kinship records: the child's `parent`
  points to the father (for body collection and signal delivery), and the
  father's `children` list holds the child.

The typical division of labor is "the parent waits for a result while the
child works", or pairing with `exec` — the child morphs into a new program
the moment it's born (this is exactly how the shell in Ch. 11 executes
commands: fork a child, the child immediately execs into `ls`, and the
parent `wait`s to collect the body).

### fork's Implementation: Copy-on-Write

The naive approach copies all of the parent's memory for the child — slow
and wasteful: many children `exec` right after birth, throwing away the
memory that was just copied.

EwokOS uses **Copy-on-Write (CoW)**. The `kfork()` flow in
[proc.c](../../kernel/kernel/src/proc.c):

```
① Find a free slot in the process table, allocate a new pid, record the
  parent-child relation
② Copy the parent's page-table structure — but only copy the "tables",
  not the "pages": both sides' page-table entries point to the same batch
  of physical pages, and both are marked read-only
③ Bump the physical pages' reference counts (Ch. 05's _pages_ref)
④ The child's ctx is copied straight from the parent's; only fork's
  return value is changed to 0
⑤ The child enters the ready queue and waits to be scheduled
```

At this point parent and child truly are "identical", sharing all physical
memory. The split happens on the first write:

```
The child writes to some page → the page-table entry is read-only
→ an exception fires → the kernel sees it's a CoW page
→ allocates a fresh physical page, copies the content, points the writer's
  page-table entry at the new page and restores writability
→ returns to user mode; the write re-executes and succeeds
```

**Copying is deferred until the moment it's truly needed — and only the
page being written is copied.** For the program, everything is transparent;
for the kernel, the cost of one `fork` drops from "copying tens of MB of
memory" to "copying a few pages of page tables".

> By the way: the `_pages_ref` reference count also prevents memory leaks
> here — when either side exits and frees its memory, pages whose reference
> count is greater than 1 must not be truly freed; only the count drops.

### exec: Loading a New Program

The `SYS_EXEC_ELF` system call replaces the current process's memory space
with the content of an **ELF file** (the standard format for executables):
parse the ELF header → map the code and data segments into the process's
memory → write the entry address into `ctx.pc`. A shell executing a command
is the combination of `fork()` + `exec()` (Ch. 11).

### exit: The Final Bow

A process cannot be destroyed immediately upon exit — its parent may still
want to read its exit code. So it first becomes a **ZOMBIE**, keeping the
minimal information; only after the parent collects the body via `wait()` is
it freed. If the parent dies first, the child is adopted by `init`, which
takes care of the body collection (one of the reasons init must exist
forever).

## 7.7 The Microkernel's Advanced Trick: Context Hijacking

Step ③ of `proc_switch` hides a mechanism that runs through the whole book.
When the kernel needs some process to "cut in line and execute" a piece of
code (for example: a hardware interrupt has arrived and a driver process
must handle it, or an IPC request has arrived and a server must process it),
it does not wake up a new thread — instead it **directly rewrites the
context that process is about to resume with**:

```c
// Example: injecting one hardware-interrupt handling into a process:
to->ctx.gpr[0] = interrupt_no;              // argument 1: the interrupt number
to->ctx.gpr[1] = data;                      // argument 2: extra data
to->ctx.pc = to->ctx.lr = entry;            // ★ program counter points at the handler
to->ctx.sp = a freshly allocated stack;     // give the handler a clean stack
```

The next time that process is scheduled, it "inexplicably" starts executing
from the `entry` function — as if it had itself called
`entry(interrupt_no, data)`. Once handling is done (calling
`SYS_INTR_END`/`SYS_IPC_END`), the kernel restores the process's original
saved context — the process resumes its previous work, unaware and
unscathed.

**All of a microkernel's drivers and servers are "woken up" by this
mechanism.** You will see it again in Ch. 09 when we discuss IPC.

## 7.8 Multicore (SMP)

The Pi 3B/4B has 4 cores. EwokOS's approach:

- Each core has one ready queue and one idle process;
- Cores kick each other with **IPIs (inter-processor interrupts)**: "hey,
  new task / time to schedule";
- Shared data structures are protected by spinlocks (`mcore_lock`).

The line `kernel: start cores ... 0 1 2 3` in the kernel boot log is the
main core waking the 3 secondary cores.

## 7.9 Exercises

1. Boot the system, run `ps`, and explain each process's state against the
   state machine in §7.3;
2. Run two `sleep 10` background tasks plus a busy task like `graphbench`,
   and watch them run "simultaneously" — flawless under 1024 switches per
   second;
3. Read all of [schedule.c](../../kernel/kernel/src/schedule.c) (only 35
   lines), then locate `proc_switch` in
   [proc.c](../../kernel/kernel/src/proc.c) and find the three steps "save
   the scene / swap page tables / put back on the queue".

## 7.10 Summary

- A process's three essentials = identity info + memory space + register
  scene (`proc_t`);
- The first process is crafted by the kernel's six-step procedure
  (load_init_proc); all later processes are reproduced by existing ones;
- The state machine: READY → RUNNING → WAIT/ZOMBIE → freed;
- Scheduling = taking the next process off the ready queue; preemption is
  triggered by the timer interrupt;
- Context switching = save the old `ctx`, swap page tables, restore the new
  `ctx`, `eret`;
- fork uses copy-on-write, exec replaces the entire memory space, exit
  turns into a zombie first;
- A microkernel injects interrupt/IPC requests into target processes via
  "context hijacking".

Next chapter: how user programs knock on the kernel's door — system calls.
