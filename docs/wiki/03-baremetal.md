# 03 The First Bare-Metal Program: Making the Pi Talk

> Language: **English** | [中文](03-baremetal.zh.md)
>
> Goal: write a program from scratch that depends on no operating system
> whatsoever and prints "Hello, OS!" over the serial port on a QEMU-emulated
> Raspberry Pi 3B. This is the most primitive form of every OS kernel.
> Terminology (MMIO/UART/GPIO/BSS/ELF…) is in the [glossary](99-glossary.md).

After this chapter you will understand:
- how a program gets loaded and executed after the Pi powers on;
- what MMIO (memory-mapped I/O) is, and how the CPU talks to hardware;
- what a linker script is, and why a bare-metal program must bring its own;
- what EwokOS's very first boot code looks like.

## 3.1 How the Raspberry Pi Boots

When a normal PC boots, the BIOS/UEFI finds a bootloader on disk. The Pi is
different: its boot is orchestrated by the **GPU firmware**:

```
① Power on. The SD card must have a FAT32 partition holding the official
   Pi firmware files (bootcode.bin, start.elf, *.dtb etc.; this repo keeps
   them in tools/bootfs/raspix/)
② The GPU reads config.txt from the SD card
③ config.txt names the kernel file:
   - 64-bit: kernel8.img → loaded at 0x80000, CPU starts in AArch64 mode
   - 32-bit: kernel7.img → loaded at 0x8000, CPU starts in ARMv7 mode
④ The CPU starts executing at the load address — that's our code!
```

So the first job of kernel development is: **produce a binary file
`kernel8.img` that the GPU firmware places at memory address 0x80000, and
the CPU starts running from there**.

The corresponding configuration in the repo is
[config.mk](../../machines/raspix/kernel/config.mk):

```make
ARCH        = aarch64
LOAD_ADDRESS = 0x80000      # kernel load address
QEMU_MACHINE = raspi3b
```

> **Why 0x80000?** No reason — it's simply the Pi firmware's convention
> (0x8000 for 32-bit). In Ch. 05 you'll see that the linker script must
> place the code at this address, or boot dies instantly.

## 3.2 The CPU's First Instruction After Power-On: Assembly Only

Right after power-on: no stack, no C runtime, and possibly several cores
running at once. C cannot work in that state (C needs a stack), so **the
first section of every kernel is written in assembly**.

Create your first kernel project anywhere (outside the repo is fine, e.g.
`~/os/hello`):

```bash
> mkdir -p ~/os/hello && cd ~/os/hello
```

### start.S — the first code the CPU executes

```asm
// start.S: execution on a Pi 3B begins here after power-on (AArch64 mode)
.section ".text.boot"      // a dedicated section the linker script puts first
.global _start             // _start is the entry symbol

_start:
    // ---- Step 1: keep only core 0, park the other three ----
    // A Pi 3B powers on all 4 cores at once. Left alone, all four would run
    // our code simultaneously and trample each other. The mpidr_el1 register
    // holds the current core's number.
    mrs     x1, mpidr_el1   // read the core number into x1
    and     x1, x1, #3      // the number lives in the lowest 2 bits
    cbz     x1, 2f          // if this is core 0, jump to label 2 and continue
1:  wfe                     // other cores: low-power wait (forever)
    b       1b

2:
    // ---- Step 2: set up the stack pointer ----
    // C function calls and local variables rely on a stack. We set the stack
    // top to the kernel image's start address (memory above 0x80000 is ours,
    // nothing lives below it, and the stack grows downward — safe).
    ldr     x1, =_start     // x1 = 0x80000 (decided by the linker script)
    mov     sp, x1          // sp is the stack pointer register

    // ---- Step 3: zero the BSS section ----
    // Uninitialized globals live in BSS, which occupies no space in the
    // image — the file contains no data for them, so they must be zeroed
    // by hand, or their values are random garbage.
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size // number of 8-byte blocks to clear (linker-computed)
3:  cbz     w2, 4f          // done
    str     xzr, [x1], #8   // write 0, advance the address by 8 bytes
    sub     w2, w2, #1
    cbnz    w2, 3b          // not done yet, loop

4:
    // ---- Step 4: enter the world of C ----
    bl      main            // call main
5:  wfe                     // main actually returned? Suspend the CPU
    b       5b
```

Notes on the key instructions:
- `mrs`: reads a system register into a general-purpose register;
- `cbz/cbnz`: compare and branch (if zero / if non-zero);
- `wfe`: Wait For Event — the CPU sleeps until an event arrives;
- `bl`: branch with link (a function call); `b`: unconditional branch.

### main.c — driving the UART to print

The serial port (**UART**, Universal Asynchronous Receiver/Transmitter) is
the lifeline of kernel debugging: no screen, no filesystem, but one serial
cable shows you what the CPU is thinking. On the Pi 3B we use the **Mini
UART** (the cut-down UART), which hangs on **GPIO** (General Purpose
Input/Output) pins 14/15.

The hardware manual tells us: on the Pi 3B, all peripheral registers are
mapped into a region of memory starting at physical address `0x3F000000`
(this is **MMIO**, Memory-Mapped I/O — device registers read and written as
if they were memory addresses). **Writing to these "addresses" is operating
the hardware.**

```c
// main.c: initialize the Mini UART and print
typedef unsigned int uint32_t;

// Read/write physical addresses directly. volatile stops the compiler from
// optimizing away these "seemingly useless" accesses
#define MMIO_BASE 0x3F000000
#define reg(r) ((volatile uint32_t*)(MMIO_BASE + (r)))

// --- Mini UART registers (offsets from the BCM2837 manual) ---
#define AUX_ENABLES  0x215004   // peripheral enable
#define MU_IO        0x215040   // TX/RX data
#define MU_IER       0x215044   // interrupt enable
#define MU_IIR       0x215048   // FIFO control
#define MU_LCR       0x21504C   // data width
#define MU_MCR       0x215050
#define MU_LSR       0x215054   // line status
#define MU_CNTL      0x215060   // TX/RX enable
#define MU_BAUD      0x215068   // baud rate

// --- GPIO registers: switch pins 14/15 to UART function ---
#define GPFSEL1      0x200004   // function select for pins 10~19
#define GPPUD        0x200094   // pull-up/down control
#define GPPUDCLK0    0x200098

static void delay(int32_t count) {
    while(count-- > 0)
        ;
}

static void uart_init(void) {
    // 1. Power on the Mini UART peripheral
    *reg(AUX_ENABLES) = 1;

    // 2. Configure the UART: interrupts off, 8 data bits, clear FIFOs
    *reg(MU_IER)  = 0;
    *reg(MU_LCR)  = 3;        // 8 data bits
    *reg(MU_MCR)  = 0;
    *reg(MU_IIR)  = 0xC6;     // clear TX/RX buffers
    *reg(MU_BAUD) = 270;      // 115200 baud (divisor for a 250MHz clock)

    // 3. Configure GPIO 14/15 as UART function (ALT5)
    uint32_t ra = *reg(GPFSEL1);
    ra &= ~(7 << 12);         // clear GPIO14's function bits
    ra |=  2 << 12;           // 010 = ALT5
    ra &= ~(7 << 15);         // clear GPIO15's function bits
    ra |=  2 << 15;
    *reg(GPFSEL1) = ra;

    // 4. Disable pull-up/down on these two pins (timing requires waiting
    //    150 clock cycles first)
    *reg(GPPUD) = 0;
    delay(150);
    *reg(GPPUDCLK0) = (1 << 14) | (1 << 15);
    delay(150);
    *reg(GPPUDCLK0) = 0;

    // 5. Enable transmit/receive
    *reg(MU_CNTL) = 3;
}

static void uart_putc(char c) {
    // Wait for space in the TX buffer (LSR bit 5 = 1 means ready)
    while(!(*reg(MU_LSR) & 0x20))
        ;
    *reg(MU_IO) = c;
}

static void uart_puts(const char* s) {
    while(*s) {
        if(*s == '\n')
            uart_putc('\r');   // a terminal newline needs both \r and \n
        uart_putc(*s++);
    }
}

void main(void) {
    uart_init();
    uart_puts("Hello, OS! I am a bare-metal kernel.\n");
    while(1)
        ;
}
```

This is what a driver looks like in its most primitive form: **consult the
manual → find the register addresses → read and write memory**. Every device
we meet later (SD card, display, interrupt controller) follows this same
pattern.

### kernel.ld — the linker script: telling the linker how to lay out memory

Normal programs are loaded by an OS, so the linker's default layout is fine.
A bare-metal program has nobody to arrange it, so it must declare itself:
**which address the code starts at, and where each section goes**.

```
/* kernel.ld */
ENTRY(_start)                    /* the entry symbol is _start */

SECTIONS
{
    . = 0x80000;                 /* start at the GPU-agreed load address */
    .text.boot : { *(.text.boot) } /* boot assembly must come FIRST! */
    .text :  { *(.text) }        /* the rest of the code */
    .rodata : { *(.rodata) }     /* read-only data (string constants etc.) */
    .data : { *(.data) }         /* initialized globals */

    . = ALIGN(8);                /* 8-byte alignment */
    __bss_start = .;             /* define a symbol for start.S to reference */
    .bss : { *(.bss COMMON) }    /* uninitialized globals */
    __bss_end = .;
}

__bss_size = (__bss_end - __bss_start) >> 3;   /* how many 8-byte units */
```

> Putting `.text.boot` first is crucial: the first 8 bytes of the file must
> be the instructions entering `_start`, because the GPU firmware jumps to
> the start of the loaded image.

### Makefile — turning three files into a kernel image

```make
CC      = aarch64-none-elf-gcc
OBJCOPY = aarch64-none-elf-objcopy
CFLAGS  = -O2 -ffreestanding -nostdinc -nostdlib -Wall \
          -march=armv8-a -mgeneral-regs-only

all: kernel8.img

kernel8.elf: start.S main.c kernel.ld
	$(CC) $(CFLAGS) -T kernel.ld -o kernel8.elf start.S main.c

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

run: kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio

clean:
	rm -f *.elf *.img
```

Flag by flag:
- `-ffreestanding -nostdlib`: no standard library (we're the standard
  library's parents);
- `-mgeneral-regs-only`: no floating-point/NEON instructions (simplifies
  boot; unlocked in Ch. 04);
- `-T kernel.ld`: use our linker script;
- `objcopy -O binary`: strips the ELF down to raw binary (the GPU
  understands binary, not ELF);
- QEMU `-serial null -serial stdio`: the Pi 3B has two UARTs — the first
  (PL011) goes nowhere, the second (Mini UART) lands on your terminal.

## 3.3 Reading Code Set #1: First Contact with ARM Instructions and the Pi

Before the code runs, let's systematically walk through the "new language"
we just used. **Don't hesitate — assembly really is simple**: AArch64 has
fewer than 20 commonly used instructions, and a bare-metal kernel needs only
half of those.

### ARM CPU Basics: Reading the Registers for the First Time

Inside the CPU sits a small set of **registers** — scratch paper dozens of
times faster than memory. Their division of labor in AArch64 mode:

| Register | Role | Where you'll meet it |
|---|---|---|
| `x0` ~ `x30` | 31 general-purpose 64-bit registers; `w0`~`w30` are their low 32 bits | All computation and argument passing |
| `sp` | Stack pointer: address of the current stack top | start.S's first practical instruction sets it |
| `pc` | Program counter: which instruction is executing | It changes on branches/exceptions |
| `xzr`/`wzr` | The always-zero register: reads as 0, writes are discarded | `str xzr,[x1]` when clearing BSS |
| `x30`(lr) | Link register: `bl` saves the return address here | Function calls |
| `x29`(fp) | Frame pointer: marks the current function's stack frame | Reading call stacks while debugging |

Another class is the **system registers** (accessed with `mrs`/`msr`), and
an OS deals almost exclusively with these few:

| System register | Role | Chapter |
|---|---|---|
| `mpidr_el1` | Which core am I | this chapter (parking secondary cores) |
| `CurrentEL` | Current privilege level (EL0/1/2) | Ch. 04 |
| `VBAR_EL1` | Address of the exception vector table | Ch. 04 |
| `ELR_EL1` / `SPSR_EL1` | Return address / saved state on exception | Ch. 04 |
| `TTBR0_EL1` / `TTBR1_EL1` | Page table base addresses (where the VM translation tables live) | Ch. 05 |
| `SCTLR_EL1` | Master switches: MMU, caches, ... | Ch. 05 |

> The `_el1` suffix means "the register as seen from EL1 (the kernel)". The
> same names with `_el0`/`_el2` are the other privilege levels' versions.
> This naming rule runs through the whole book.

### The Calling Convention: How Arguments Are Passed

Once a C function is compiled to assembly, argument passing follows a fixed
convention (AAPCS64):

```
Arguments 1~8:  x0, x1, x2, x3, x4, x5, x6, x7
Return value:   x0
Callee must preserve (restore after use): x19 ~ x28
Free to clobber: x0 ~ x18 (the caller's own responsibility)
```

You only need to recognize this convention for now; in Ch. 04 you'll pass
arguments to C functions from assembly with your own hands, and Ch. 08's
system calls reuse the exact same rules (x0 = call number, x1~x3 =
arguments).

### Every Instruction Used in This Chapter

| Instruction | Meaning | Used here for |
|---|---|---|
| `mov x1, x2` | assignment | `mov sp, x1` |
| `ldr x1, =symbol` | load a symbol's address / a constant into a register | taking the addresses of `_start`, `__bss_start` |
| `str x1, [x2]` | store x1 at address x2; `[x1], #8` advances the address by 8 afterwards | clearing BSS |
| `and/orr` | bitwise AND / OR | extracting the core number, setting GPIO function bits |
| `sub w2, w2, #1` | subtraction | loop counting |
| `cbz/cbnz x1, label` | branch if zero / non-zero | loops and branches |
| `b label` / `bl label` | branch / call (saves the return address) | `bl main` |
| `wfe` | sleep waiting for an event | parking secondary cores |
| `mrs x1, sysreg` | read a system register | reading the core number |

### See It with Your Own Eyes: Disassemble Your Kernel

The best way to "read code" is to look at what the compiler really
generated:

```bash
> aarch64-none-elf-objdump -d kernel8.elf | less
```

Find the `<main>:` section and compare it with `main.c`: you'll see that
`*reg(MU_IO) = c` became two instructions — `ldr` (compute the address) +
`str` (write to memory). **"Driving hardware" ultimately means reading and
writing addresses**, and assembly lays this bare.

## 3.4 Run It!

```bash
> make run
Hello, OS! I am a bare-metal kernel.
```

**Congratulations — you've written your first kernel.** What just happened:

1. QEMU simulated the Pi firmware and loaded `kernel8.img` at `0x80000`;
2. The CPU started at `_start`: parked the extra cores, set the stack,
   cleared BSS, entered `main`;
3. `main` configured GPIO and the UART per the manual, pushing data to the
   physical registers character by character;
4. The data traveled across the virtual serial wire onto your terminal.

## 3.5 Reading Against the Repo: How EwokOS Does It

Looking back at the repository now, you'll find the same structure, just
more complete:

| This chapter's hand-written code | EwokOS counterpart |
|-------------|----------------|
| `start.S` | [boot.S](../../kernel/platform/aarch64/arch/v8/boot.S) — also masks interrupts, sets the stack, parks secondary cores, but additionally handles the EL2→EL1 privilege switch (Ch. 04) |
| linker script `kernel.ld` | [mkos.lds.S](../../machines/raspix/kernel/mkos.lds.S) — the address comes from `config.mk`'s `LOAD_ADDRESS` |
| `uart_init/uart_putc` | [mini_uart.c](../../machines/raspix/kernel/lib/bcm283x/src/mini_uart.c) — almost identical to yours |
| `main()` | [_kernel_entry_c](../../kernel/kernel/src/kernel.c) — initializes memory, UART, the process table, loads init |

Look at `mini_uart.c` in particular — the only difference from our code is
the register base address:

```c
// Hand-written version: physical address 0x3F000000 hard-coded
#define MMIO_BASE 0x3F000000

// EwokOS version: accesses via the virtual address after MMU mapping (Ch. 05)
#define UART_BASE (_sys_info.mmio.v_base | UART_OFFSET)
```

Now savor the EwokOS kernel's "main function"
([kernel.c](../../kernel/kernel/src/kernel.c), excerpt):

```c
void _kernel_entry_c(void) {
    memset(_bss_start, 0, ...);        // same as your start.S: clear BSS
    sys_info_init();                   // collect hardware info (RAM size, MMIO base)
    init_kernel_vm();                  // build kernel virtual memory (Ch. 05)
    uart_dev_init(19200);              // initialize the UART (the thing you just wrote!)
    kout_str("\n=== ewokos booting ===\n\n");
    kmalloc_init();                    // kernel memory allocator
    sd_init();                         // SD card driver (Ch. 10)
    load_kernel_config();              // read the config file
    irq_init();                        // interrupts (Ch. 06)
    procs_init();                      // process table (Ch. 07)
    load_init_proc();                  // load the first user process /sbin/init (Ch. 11)
    timer_set_interval(0, _kernel_config.timer_freq);  // the timer heartbeat
    __irq_enable();                    // enable interrupts
    halt();                            // the kernel enters the scheduling loop
}
```

The entire kernel boot flow is these dozen-odd lines. Over the next ten
chapters we conquer them one by one.

## 3.6 Don't Hesitate, Assembly Is Simple: Mixing C and Assembly

Many people get stuck at "I don't dare write assembly". Let's break that
with a small experiment: **rewrite `uart_putc` in pure assembly and call it
from C**. This is exactly how assembly and C cooperate in EwokOS (the
kernel's `syscall` path and context switching are written this way).

### Step 1: an assembly `uart_putc`

Create `uart_asm.S`:

```asm
// uart_asm.S: implement uart_putc(char c) in assembly
// Per the calling convention, the first argument c arrives in x0

#define MMIO_BASE 0x3F000000
#define MU_IO     0x215040        // data register
#define MU_LSR    0x215054        // status register

.global uart_putc_asm
uart_putc_asm:
    mov     x9, x0                  // x0 holds the argument; save it first
    ldr     x10, =(MMIO_BASE + MU_LSR)
1:  ldr     w11, [x10]              // read line status
    and     w11, w11, #0x20         // isolate the "TX buffer empty" bit
    cbz     w11, 1b                 // not empty? keep waiting
    ldr     x10, =(MMIO_BASE + MU_IO)
    str     w9, [x10]               // write the character into the data register
    ret                             // return (jump to the address saved in x30)
```

Line by line, there's nothing new: `mov`, `ldr`, `str`, `and`, `cbz`, `ret`
— all instructions this chapter has already seen. **Assembly programming is
just three things: shuffling registers around + reading/writing addresses +
branching.**

### Step 2: declare and call it from C

An assembly function is just an "ordinary function" to C — declare it and
call:

```c
extern void uart_putc_asm(char c);   // declaration: the implementation is in a .S file

void main(void) {
    uart_init();
    uart_putc_asm('A');              // C calling assembly
    uart_putc_asm('\r');
    uart_putc_asm('\n');
    while(1);
}
```

Add `uart_asm.S` to the Makefile's compile list:

```make
kernel8.elf: start.S main.c uart_asm.S kernel.ld
	$(CC) $(CFLAGS) -T kernel.ld -o kernel8.elf start.S main.c uart_asm.S
```

`make run`; seeing `A` means you've completed your first C/assembly mixed
program.

### The Argument-Passing Mechanism, Strung Together

The complete chain of that round trip is:

```
C:    uart_putc_asm('A')
  ↓   compiler emits: mov w0, #65     (1st argument → x0)
  ↓                   bl uart_putc_asm (call; return address saved to x30)
asm:  fetch 'A' from x0, do the work, ret
  ↓   ret jumps back to the C code x30 points to, execution continues
```

With multiple arguments (e.g. `int add(int a, int b)`), it's `a→x0, b→x1`,
and the return value comes back in `x0`. Ch. 08's system calls reuse this
very channel unchanged.

> Compare with the real code: EwokOS's system-call entry
> [syscall_aarch64.S](../../system/basic/libc/libewoksys/ewoksys/src/syscall_aarch64.S)
> is only two instructions — `svc #0` + `ret` — even shorter than what you
> just wrote. Assembly isn't scary; it's merely direct.

## 3.7 Exercises

1. Change the printed text to your name;
2. Write a function that prints numbers (hint: divide by 10, take the
   remainder);
3. Read characters from `reg(MU_IO)` and build an echo program (hint:
   `MU_LSR & 0x01` means data was received);
4. Rewrite `uart_puts` in assembly too (argument: the string address in x0,
   process byte by byte).

## 3.8 Summary

- The Pi's kernel is loaded by GPU firmware: the 64-bit file is
  `kernel8.img`, placed at memory address `0x80000`;
- A kernel's first section must be assembly: park secondary cores, set the
  stack, clear BSS, then enter C;
- Hardware is accessed through MMIO: reading/writing specific physical
  addresses = operating hardware registers;
- The linker script decides the memory layout of code and data;
- Assembly is "register shuffling + reading/writing addresses + branching";
  C and assembly pass arguments through x0~x7;
- EwokOS's boot code has the same structure as this chapter's, just with
  privilege-level switching and the MMU added.

Next chapter: we dive into CPU privilege levels — why user programs can't
touch hardware directly, and how the kernel catches "exceptions". This is
the foundation of system calls and security.
