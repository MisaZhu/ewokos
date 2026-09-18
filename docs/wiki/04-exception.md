# 04 CPU Privilege Levels and Exception Handling

> Language: **English** | [中文](04-exception.zh.md)
>
> Goal: understand ARM's 4 exception levels (EL0~EL3), see how EwokOS
> switches levels at boot, understand the "exception" mechanism and the
> vector table, and see how user programs get "kept in line".
> Terminology (EL0~EL3/eret/VBAR/ELR/SPSR…) is in the [glossary](99-glossary.md).

## 4.1 Why Privilege Levels Exist

Last chapter's bare-metal program could read and write any memory and any
device at will. With 10 programs running at once, any program writing to a
wrong address would corrupt the others — or even the hardware.

The ARMv8 CPU provides 4 **Exception Levels**; the higher the level, the
greater the power:

```
┌────────────────────────────────────────────┐
│ EL3  Secure Monitor (secure firmware; unused)  │
│ EL2  Hypervisor (virtualization; passed through briefly at boot) │
│ EL1  OS kernel ← the EwokOS kernel runs here   │
│ EL0  User mode ← all user processes (shell, apps) │
└────────────────────────────────────────────┘
```

The rules are simple:
- **EL0 programs cannot execute privileged instructions** (e.g. modifying
  page tables, operating interrupts) — executing one triggers an exception;
- **EL0 programs cannot access protected resources** (kernel memory, device
  registers) — accessing one triggers an exception;
- Only the EL1 kernel can configure these protection rules (via page tables
  and system registers).

"Exception" is an umbrella term covering four kinds of events:

| Type | Examples |
|------|------|
| Interrupt (IRQ) | Timer expiry, key press, SD card finished reading (Ch. 06) |
| Synchronous exception | Illegal memory access, executing a privileged instruction |
| System call (SVC) | A user program actively requesting kernel service (Ch. 08) |
| System error (SError) | Bus errors and the like |

**When an exception occurs, the CPU automatically does three things**: saves
the return address, switches to a higher level, and jumps to the matching
handler entry in the "exception vector table". All of the kernel's power is
built on this mechanism.

## 4.2 EwokOS Boot: Dropping from EL2 to EL1

When a Pi 3B/4B powers on, the CPU sits at **EL2** (the virtualization
level). An OS kernel belongs at EL1, so the first thing EwokOS's boot code
does is "downgrade".

Look at the start of [boot.S](../../kernel/platform/aarch64/arch/v8/boot.S)
(annotated):

```asm
__entry:
    msr DAIFSet, #0xf        ; mask all exceptions and interrupts (can't be disturbed yet)

    mrs x0, CurrentEL        ; read the current exception level
    cmp x0, #4               ; EL2's encoding is 4 (shifted left by 2)
    beq __el1_entry          ; already at EL1? proceed directly

    ; --- currently at EL2, prepare the downgrade ---
    mov x0, #1
    mov x1, xzr
    bfi x1, x0, #31, #1      ; HCR_EL2.RW=1: EL1 runs in 64-bit mode
    orr x1, x1, #0x2
    msr HCR_EL2, x1          ; write the hypervisor control register

    mov x0, #0x3c5
    msr SPSR_EL2, x0         ; the state "after return": EL1, interrupts off
    adr x2, __el1_entry
    msr ELR_EL2, x2          ; the "return address" = the EL1 entry
    eret                     ; ★ exception return: jump to ELR_EL2, level drops to EL1
```

Key concepts:
- `msr` / `mrs`: write / read system registers;
- `eret` (Exception Return): not an ordinary jump — it switches the
  privilege level to the one saved in `SPSR` and jumps to the address saved
  in `ELR`. Downgrading is "forging an exception return".

After dropping to EL1, the code continues its preparations:

```asm
__el1_entry:
    ; disable alignment checking, disable the MMU (no page tables yet)
    mrs x0, SCTLR_EL1
    bic x0, x0, #1
    msr SCTLR_EL1, x0

    ; allow floating point/NEON (accesses would fault otherwise)
    mrs x0, CPACR_EL1
    mov x1, #0x3
    bfi x0, x1, #20, #2
    msr CPACR_EL1, x0

    msr spsel, #0x1          ; use the EL1-dedicated stack
    adr x0, #_svc_start_stack
    mov sp, x0               ; set the stack

    bl _boot_start           ; enter C: build boot page tables, enable the MMU (Ch. 05)

    ; multicore: secondary cores go to slave_core, the primary continues
    ldr x1, =_kernel_entry_c ; enter the kernel's C entry
    br x1
```

Note the `SCTLR_EL1` system register — it is EL1's master switch, and its
**M bit turns the MMU on or off**. Chapter 05 flips it.

## 4.3 The Exception Vector Table: the "Switchboard" for Exceptions

Once the kernel is running, it can be interrupted by an exception at any
moment (timer fired, a user program's memory access faulted, a user program
made a system call...). Where does the CPU jump when disturbed? To the
**exception vector table** pointed to by **VBAR_EL1** (Vector Base Address
Register).

AArch64's vector table has **16 entries** (4 sources × 4 exception types),
each entry 128 bytes apart:

```
source \ type    Sync        IRQ        FIQ       SError
────────────────────────────────────────────────────────
current EL, SP_EL0   +0x000   +0x080   +0x100   +0x180
current EL, SP_ELx   +0x200   +0x280   +0x300   +0x380
from a lower EL (EL0)  +0x400   +0x480   +0x500   +0x580  ← user exceptions land here
lower EL in 32-bit mode +0x600   +0x680   +0x700   +0x780
```

The kernel installs the vector table at startup
([kernel.c](../../kernel/kernel/src/kernel.c)):

```c
void _kernel_entry_c(void) {
    ...
    copy_interrupt_table();   // place the interrupt vector table at its assigned memory
    ...
}
// On AArch64, VBAR_EL1 is set to point at this table (done in arch code)
```

## 4.4 Step One of Handling an Exception: Saving the Context

When an exception hits, the user program was "in the middle of something" —
every register is part of the scene. After handling the exception, the
kernel must let the user program **continue from exactly where it stopped**,
so the first job is pushing all registers onto the stack. See the macro in
[interrupt.S](../../kernel/platform/aarch64/arch/common/src/interrupt.S):

```asm
.macro SAVE_IRQ_CONTEXT
    stp q30, q31, [sp, #-32]!   ; save the 32 128-bit FP/vector registers
    ...                          ; (q0~q31, 32 stores in total)
    stp x28, x29, [sp, #-16]!   ; save the 31 64-bit general registers
    ...                          ; (x0~x30)
    mrs x29, fpsr                ; floating-point status register
    mrs x28, fpcr
    stp x28, x29, [sp, #-16]!
    mrs x29, sp_el0              ; the user-mode stack pointer
    stp x29, x30, [sp, #-16]!
    mrs x29, spsr_el1            ; ★ level and state when interrupted
    mrs x28, elr_el1             ; ★ instruction address when interrupted (return point)
    stp x28, x29, [sp, #-16]!
.endm
```

The two most important registers:
- **ELR_EL1**: the instruction address at the moment of interruption. After
  handling, `eret` jumps back here and the program continues seamlessly;
- **SPSR_EL1**: the state at that moment (were we at EL0 or EL1, were
  interrupts masked).

The in-memory layout saved on the stack is exactly the kernel's `context_t`
structure, which the scheduler can operate on directly — this is the basis
of Ch. 07's context switching.

When handling is done, the symmetric `RESTORE_IRQ_CONTEXT` runs: restore
every register, and finally `eret` back to the interrupted program.

## 4.5 The Full Life Cycle of One Exception

Take "a user program accessed unmapped memory" as an example:

```
user mode (EL0): program executes ldr x0, [x1]
        │ x1 points to an illegal address
        ▼
CPU hardware automatically:
   1. ELR_EL1 ← address of the faulting instruction;  SPSR_EL1 ← EL0 state
   2. switch to EL1, jump to the "sync exception from a lower EL" entry (+0x400)
        ▼
vector table code (interrupt.S):
   SAVE_IRQ_CONTEXT          ← save the scene onto the kernel stack
   bl c_interrupt_handler    ← enter C code to judge the exception's cause
        ▼
kernel handling (page fault? illegal access? syscall?):
   - system call: service it by number (Ch. 08)
   - illegal access by a user program: kill the process
   - page fault: allocate a physical page, update page tables (Ch. 05)
        ▼
RESTORE_IRQ_CONTEXT + eret   ← restore the scene, back to user mode
```

## 4.6 A Magical Journey Through Space-Time: Crossing from Kernel Mode to User Mode

Section 4.2 demonstrated the "downgrade": EL2 to EL1. Now for a more crucial
crossing — **departing from kernel mode (EL1) and "launching" a user process
into user mode (EL0)**. This is the OS's "creation ceremony", and Ch. 07
will genuinely use it when creating processes.

Think it through first: the CPU is running in kernel mode; how does it
"become" user-mode execution of a new piece of code? The answer is to run
the exception mechanism in reverse. §4.1 said that when an exception occurs,
the hardware automatically saves `ELR` (return address) and `SPSR` (state
after return), and `eret` restores from those two registers. So — **if the
kernel deliberately fills in those two registers and then executes `eret`,
it has fabricated an "exception return" out of thin air, and the CPU "wakes
up" at the place and level the kernel specified**.

### Before the crossing, the kernel sets five things up

```
① Draw the user's world: build the new process a page table (the Ch. 05
   clone), mapping its code, data, and stack into the lower half —
   permission-marked "EL0 accessible"
② Load its world: write the new page-table base into TTBR0_EL1
   (swap out the lower-half translation dictionary)
③ Choose the wake-up place: ELR_EL1 ← the user program's entry address
④ Choose the wake-up posture: SPSR_EL1 ← the EL0 state encoding
   (e.g. 0x000, M[3:0]=0 means EL0t); SP_EL0 ← the user stack top
⑤ Empty its pockets: zero the general registers
   (so kernel data can't leak to the user)
```

Then one instruction lights the fuse:

```asm
eret          ; the CPU reads SPSR/ELR, drops to EL0, and starts executing
              ; at the user entry address — crossing complete!
```

From this moment on, that code is a "user process": it can't touch hardware,
can't modify page tables, and anything dangerous it attempts triggers an
exception the kernel catches. **The crossing is a one-way ticket** — the
only door back is the system call (Ch. 08).

### What Actually Happens During the Crossing

Slow down the CPU's internal actions at the instant of `eret`:

```
state before eret                        state after eret
─────────────────                        ─────────────────
level:  EL1 (kernel)          ───────►   level:  EL0 (user)
PC:     the kernel's eret     ───────►   PC:     the user entry in ELR_EL1
stack:  SP_EL1 (kernel stack) ───────►   stack:  SP_EL0 (user stack)
tables: lower half already the ──────►   translation live: the user sees
        new process's                    only its own pages
IRQs:   decided by SPSR bits  ───────►   restored per SPSR
```

One `eret` swaps four things: **level, code, stack, and the memory view**.
That is the full meaning of "space-time travel" — the process henceforth
lives in its own space-time, until the next exception (interrupt / system
call / fault) drags it back into the kernel.

### Where It Hides in EwokOS

EwokOS has no dedicated "start a user process" instruction; this crossing is
folded into **the unified context-switch path**: the kernel prepares a
`context_t` for the new process (a register scene, including the fabricated
`elr`, `spsr`, and stack pointer), and `proc_switch` restores that context
and executes the usual `eret` — to the CPU, "starting a new process for the
first time" and "resuming an old process that was switched out" travel the
exact same code path. Chapter 07 covers this unified design.

## 4.7 What All This Has to Do with "an Operating System"

Now we can answer a few fundamental questions:

1. **Why can't user programs read or write hardware?**
   Device registers are mapped in the kernel's page tables (Ch. 05); an EL0
   access triggers a synchronous exception and is intercepted by the kernel.
2. **How does a user program ask the kernel to work?**
   Execute the `svc` instruction → synchronous exception → enter the kernel
   → the kernel services the request by the number in a register. That is
   the system call, the protagonist of Ch. 08.
3. **Why doesn't a crashing program drag the system down?**
   Every exception passes through the kernel's hands first. The offending
   program is killed; the kernel and all other processes keep running.

## 4.8 Exercises

1. Open [boot.S](../../kernel/platform/aarch64/arch/v8/boot.S) and read the
   downgrade from `__entry` to `__el1_entry` line by line;
2. Set a breakpoint on `_kernel_entry_c` in GDB (Ch. 02's `make debug` +
   `make gdb`), and after booting inspect the privilege-level registers with
   `p $ELR_EL1` (or `info registers`);
3. Think: what if the vector table itself were overwritten by a user
   program? (Answer: the vector table's memory is writable only by the
   kernel — that's exactly what Ch. 05's page-table permissions solve.)

## 4.9 Summary

- ARM has four exception levels EL0~EL3: user programs at EL0, the kernel at
  EL1;
- At boot, EwokOS drops from EL2 to EL1 by "forging an exception return"
  (`eret`);
- The exception vector table is the kernel's single front door for all
  events: 16 slots arranged by source and type;
- The core exception-handling motion: save the scene (including
  `ELR/SPSR`) → handle in C → restore the scene → `eret`;
- Running `eret` in reverse (filling `ELR/SPSR/TTBR0/SP_EL0`) completes the
  crossing from kernel mode to user mode — the ceremony of creating a user
  process. The crossing is one-way; the only door back is the system call.

Next chapter: memory management and the MMU — giving every process its own
"private memory" while the kernel can still reach the hardware. It is the
most exquisite design in an operating system.
