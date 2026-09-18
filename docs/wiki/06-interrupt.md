# 06 Interrupts and Timers

> Language: **English** | [中文](06-interrupt.zh.md)
>
> Goal: understand the interrupt mechanism and interrupt controllers; see
> how EwokOS responds to hardware events; focus on how the timer interrupt
> drives the whole system's "heartbeat" and process scheduling.
> This chapter is code set #2 on the hands-on main line: gather your courage
> and wade a little deeper.
> Terminology (IRQ/FIQ/GIC/EOI/Timer…) is in the [glossary](99-glossary.md).

## 6.1 Why Interrupts Exist

Suppose there were no interrupts: for the kernel to know "has a key been
pressed", it could only keep querying the device's status register nonstop
(polling). Polling wastes the CPU, and the later you check, the later you
respond.

**Interrupts** let the hardware notify the CPU proactively:

```
The CPU is running process A
        │
SD card finished reading ──► interrupt controller ──► raises an IRQ
        │                       (Interrupt Request) signal at the CPU
CPU hardware automatically: stops A, saves the scene, jumps into the
        kernel's interrupt-handling code
        │
kernel: takes the data, wakes the waiting process
        │
restore the scene (possibly resuming with process B instead)
```

An interrupt is one kind of Ch. 04's "exceptions". The handling framework is
fully reused: vector-table entry → save context → handle in C → restore →
`eret`.

## 6.2 Getting Familiar with Pi Interrupts: Where They Come From

The Pi 3B has around 60+ interrupt sources, all aggregated into the Broadcom
interrupt controller's three status registers (`IRQ_BASIC_PENDING`,
`IRQ_PENDING_1`, `IRQ_PENDING_2`). The ones that matter for this tutorial:

| Source | Number | Handled by | Significance |
|---|---|---|---|
| Timer (ARM generic timer) | BASIC 5 | the kernel | heartbeat and scheduling (this chapter's star) |
| Mini UART | IRQ1 29 | userland `uartd` | serial TX/RX |
| AUX (SPI1/2 shared) | IRQ1 29 | userland drivers | peripherals |
| EMMC (SD card) | IRQ2 30 | userland `sdd` | storage |
| GPIO | IRQ1 49/50 | userland drivers | buttons etc. |

**Getting familiar with interrupts = getting familiar with this table.**
Every time you take on a new device, the first thing is to look up its
interrupt number, then follow the same routine: enable → wait for the
interrupt → handle → clear the flag.

## 6.3 The Basic Method of Interrupt Handling

Whatever the interrupt, the handling method is a fixed four-step template:

```
① Enable:   write the interrupt controller's enable register + open the
            CPU's master switch (DAIF)
② Identify: an interrupt arrives; read the pending register to see which
            device it came from
③ Handle:   do the work (take data / keep accounts / wake processes);
            the faster the better
④ Clear:    write the device's "clear interrupt" register + tell the
            controller "I'm done", or the same interrupt re-enters like mad
```

Compare with the real code in EwokOS's Pi BSP
([irq_pix.c](../../machines/raspix/kernel/bsp/pix/irq_pix.c), excerpt):

```c
// ② Identify: check the status registers one by one
uint32_t pend = get32(IRQ_PENDING_1);
if(pend & (1 << 29)) { /* Mini UART has an interrupt */ }

// ④ Clear: write the device's clear-interrupt bit (AUX as an example)
put32(AUX_MU_IIR, 0x4);   // write 1 to clear the interrupt flag
```

The easiest bug among the four steps is **forgetting step ④**: with the
interrupt flag never cleared, the CPU gets pulled right back by the same
interrupt upon returning — the system appears "frozen". When a real machine
mysteriously freezes after boot, suspect this first.

## 6.4 The Interrupt Controller: the GIC

Modern ARM chips don't wire every device directly into the CPU; they wire
them into a module called the **GIC (Generic Interrupt Controller)**. Its
jobs:

1. Gather all devices' interrupt requests;
2. Arbitrate by priority (when several arrive at once, who goes first);
3. Deliver the chosen interrupt to the right CPU core;
4. Provide registers for the kernel to **enable/mask** an interrupt, query
   which interrupt fired, and signal "handling complete".

GIC versions and registers differ across chips, so EwokOS puts this part in
the platform layer:
[gic.c](../../kernel/platform/aarch64/arch/common/src/gic.c).

The Pi 3B, however, uses Broadcom's own interrupt controller (not a
standard GIC), in
[irq_pix.c](../../machines/raspix/kernel/bsp/pix/irq_pix.c):

```c
void irq_enable_pix(uint32_t irq) {
    if(IRQ_TIMER0 == irq){
        *((uint32_t*)ENABLE_BASIC_IRQS) = (1 << 5);  // write the register to enable the timer interrupt
    }
}
```

This is exactly the routine from Ch. 03: **consult the manual, find the
register, write the memory**. Boards differ (Pi3/Pi4) only in register
addresses, which is why the Pi BSP splits into `pix/` (Pi2/Pi3) and `pi4/`
(Pi4).

## 6.5 The OS's Heartbeat: the Clock Gives It Life

Until now, our kernel has been a "passive" program: if nobody calls it, it
does nothing. The timer interrupt changes everything — **from the moment
the clock is switched on, the system has its own heartbeat, and with it,
"life"**: it wakes up every ~1 millisecond and proactively checks whether a
process should be switched, whether time accounts need bookkeeping, and
whether any sleeping process has come due.

Among all interrupts, the most important is the **timer interrupt** — it
fires at a fixed frequency and is the operating system's source of time.

### Setting the Frequency

The last step of kernel startup
([kernel.c](../../kernel/kernel/src/kernel.c)):

```c
timer_set_interval(0, _kernel_config.timer_freq);  // default 1024 times/second
__irq_enable();                                    // open the master interrupt switch
```

`timer_freq` comes from the config file `/etc/kernel/kernel.conf` (config
files are covered in Ch. 11) and defaults to 1024 — one timer interrupt
**every ~0.98 ms**. That is the kernel's heartbeat.

The Pi implementation
([timer.c](../../machines/raspix/kernel/bsp/timer.c)) writes ARM's generic
timer registers directly:

```c
inline void write_cntv_tval(uint32_t tval) {
    __asm volatile("msr CNTV_TVAL_EL0, %0" : : "r" (tval) : "memory");
}
// Write a countdown value; when it counts down to 0, an interrupt fires
```

### What Happens When the Interrupt Arrives

The interrupt entry point is in [irq.c](../../kernel/kernel/src/irq.c); the
core logic:

```c
// Timer interrupt handling (simplified):
timer_clear_interrupt(0);   // ① clear the flag and reload the next countdown
...
schedule(ctx);              // ② take the chance to schedule! switch to the next process
```

The key design: **every timer interrupt is an opportunity for a "forced
switch"**. Process A is running, its time slice expires, the interrupt
breaks it, and before returning the kernel calls `schedule()` to switch to
process B. That way no process can hog the CPU — this is how **preemptive
multitasking** is realized (scheduler details in Ch. 07).

The kernel also keeps accounts inside the interrupt (accumulating system
uptime and the CPU time each process has burned) — that data is exactly
what the `ps` command displays.

## 6.6 The Layers of Kernel Interrupt Handling

EwokOS's interrupt code comes in three layers with clear duties:

```
① platform assembly layer  interrupt.S          save/restore all registers (Ch. 04)
        │
② board layer      machines/raspix/bsp/  decide which interrupt (read the board's
        │                                  interrupt registers); clear the flag,
        ▼                                  EOI (End Of Interrupt)
③ generic kernel   kernel/kernel/src/    irq.c: dispatch to the concrete handler
   layer           irq.c / interrupt.c   - timer → scheduling
                                         - other hardware IRQs → forwarded to the
                                           user process that registered them
```

Layer ③ has a microkernel specialty: hardware interrupts can be **forwarded
to userland driver processes**
([interrupt.c](../../kernel/kernel/src/interrupt.c)):

```c
int32_t interrupt_setup(proc_t* cproc, uint32_t interrupt,
                        ewokos_addr_t entry, ewokos_addr_t data);
int32_t interrupt_send(context_t* ctx, uint32_t interrupt);
```

The flow: a driver process (e.g. `uartd`) calls `SYS_INTR_SETUP` to tell the
kernel "IRQ xx belongs to me"; when the interrupt arrives, the kernel wakes
the driver process (as if it received an event); the driver handles it and
calls `SYS_INTR_END` so the kernel can tell the hardware "done". **The code
handling hardware interrupts runs in user mode** — this is the microkernel's
essence, and the reason "a driver crash doesn't take down the system".

## 6.7 Masking Interrupts: the Kernel's Self-Protection

When the kernel modifies shared data (like the ready queue), its worst fear
is being interrupted mid-way and scheduled off — leaving the data in an
inconsistent state. Hence this common pattern in kernel code:

```c
__irq_disable();      // mask interrupts (write DAIF / CPSR)
... critical-section operations ...
__irq_enable();       // unmask interrupts
```

`kprintf` does the same when writing to the UART (to keep output from
multiple cores/processes from interleaving):

```c
// kernel/lib/src/kprintf.c (illustrative)
__irq_disable();
mcore_lock(&_kout_spin);   // SMP: also take the spinlock (multiple cores writing at once)
... write the UART ...
mcore_unlock(&_kout_spin);
__irq_enable();
```

> Interrupts must not stay masked for long! While they're masked, every
> event is frozen. That's why interrupt handlers should be as short as
> possible — heavy work is left for kernel threads or userland drivers to
> chew on slowly.

## 6.8 Exercises

1. Boot the system under `machines/raspix/system`, run `ps`, and observe
   each process's `cpu` time statistics — they all come from the timer
   interrupt's bookkeeping;
2. Read the `IRQ_TIMER0` branch in [irq.c](../../kernel/kernel/src/irq.c)
   and confirm the "timer interrupt → `schedule()`" path;
3. Food for thought: what happens if the timer frequency is set to 1 (once
   per second)? (Hint: a process's time slice becomes 1 second, and
   interactive programs get visibly laggy. Change `timer_freq` in
   `/etc/kernel/kernel.conf` and experience it for real.)

## 6.9 Summary

- Interrupts let hardware proactively break into the CPU, replacing
  inefficient polling;
- The four-step interrupt template: enable → identify → handle → clear
  (forgetting to clear = freeze);
- Interrupt controllers (GIC / Broadcom's controller) gather and dispatch
  interrupts; the kernel enables/masks them via registers;
- The timer interrupt beats at `timer_freq` (default 1024) times per second,
  driving preemptive scheduling — the moment the system "comes alive";
- Interrupt handling has three layers: assembly saves the scene → the board
  layer identifies the interrupt → the generic kernel dispatches it;
- A microkernel forwards hardware interrupts to userland driver processes;
- Critical sections are protected by masking interrupts / spinlocks — but
  keep them short.

Next chapter is the kernel's heart: processes and the scheduler.
