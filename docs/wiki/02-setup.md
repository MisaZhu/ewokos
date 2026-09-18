# 02 Setting Up the Development Environment

> Language: **English** | [中文](02-setup.zh.md)
>
> Goal: install all software tools, prepare the hardware, and compile & run
> EwokOS for the first time. Supports macOS and Linux (Ubuntu/Debian).
> Everything is done from the command line.
> The software part is mandatory; the hardware part can be skipped for now
> (QEMU covers all experiments through Ch. 13) and revisited before the
> real-hardware flashing in Ch. 14.

## 2.1 Which Software Tools Do We Need

Developing an OS for the Raspberry Pi on your own computer involves three
concepts:

1. **Cross Compiler**: your computer is x86 or ARM Mac, while the target
   program must run on an ARM Pi, so you need "a compiler that generates ARM
   code".
   - `arm-none-eabi-gcc`: generates 32-bit ARM code
   - `aarch64-none-elf-gcc`: generates 64-bit ARM (AArch64) code ← this
     tutorial's main line
   - `none` means "bare metal": programs built by these compilers depend on
     no operating system at all (exactly what we want).
2. **QEMU**: a hardware emulator that can conjure up a Raspberry Pi
   (`raspi3b`/`raspi2b`) on your computer, saving you from reflashing SD
   cards over and over.
3. **Filesystem tools**: `mke2fs` (formats ext2/ext3), `e2cp` (copies files
   into an image).

## 2.2 macOS Installation

```bash
# QEMU and filesystem tools
> brew install qemu e2tools e2fsprogs

# ARM's official cross toolchain (contains both arm-none-eabi and aarch64-none-elf)
> brew install --cask gcc-arm-embedded
```

If brew's cask version doesn't meet your needs, you can also download
`arm-gnu-toolchain-*-mac-*-aarch64-none-elf.pkg` and
`arm-gnu-toolchain-*-mac-*-arm-none-eabi.pkg` from the
[Arm Developer website](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

## 2.3 Linux (Ubuntu/Debian) Installation

```bash
> sudo apt install qemu-system-arm qemu-system-misc e2tools e2fsprogs

# 32-bit ARM toolchain
> sudo apt install gcc-arm-none-eabi

# 64-bit AArch64 bare-metal toolchain (aarch64-none-elf-gcc)
# Ubuntu's repos don't carry the bare-metal variant; download it from Arm:
#   https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# After downloading arm-gnu-toolchain-*-x86_64-aarch64-none-elf.tar.xz:
> sudo tar xf aarch64-none-elf.tar.xz -C /opt/
> export PATH=$PATH:/opt/arm-gnu-toolchain-*-aarch64-none-elf/bin
# Adding the previous line to ~/.bashrc is recommended
```

## 2.4 Verifying the Toolchain

```bash
> aarch64-none-elf-gcc --version
> arm-none-eabi-gcc --version
> qemu-system-aarch64 --version
```

Success = all three commands print a version number.

## 2.5 Getting the Source & Build Conventions

Get the EwokOS source (if you haven't) and place it at the recommended path:

```bash
> mkdir -p ~/work && cd ~/work
> git clone <EwokOS repository URL> ewokos
> cd ~/work/ewokos
```

EwokOS's Makefiles locate the source root via the `ewokos` environment
variable, **whose default is exactly `~/work/ewokos`**. Keeping the repo
there is the most convenient; if you put it elsewhere, run before every
build:

```bash
> export ewokos=/your/path/ewokos
```

### Choosing a Build Target

The repository has several buildable targets:

| Directory | Description |
|------|------|
| `machine.virt/` | QEMU `virt` machine, VirtIO peripherals, fast boot — **first choice for daily development** |
| `machines/raspix/` | Raspberry Pi 2B/3B/4B, runs both in QEMU and on real hardware |

Get the full flow working with `machine.virt` first, then build `raspix`.

## 2.6 Follow Along: First Build and Run

```bash
> cd machine.virt/system
> make            # full build: kernel + userland + SD image; ~10–30 min the first time
> make run        # start QEMU
```

You'll see boot logs similar to this (excerpt):

```
=== ewokos booting ===

kernel: init kernel malloc     ... [OK]
kernel: init sd                ... [OK]
kernel: load kernel config     ... [OK]
-----------------------------------------------------
 ______           ______  _    _   ______  ______
(  ___ \|\     /|(  __  )| \  / \ (  __  )(  ___ \
...
  machine              virt
  arch                 aarch64-4k
  cores                4
  ...
-----------------------------------------------------
kernel: loading init ... [ok]
init: /sbin/core           [ok]
init: /sbin/vfsd           [ok]
init: /sbin/sdfsd          [ok]
...
# Once the prompt appears, try these commands:
> ls /
> ps
> uname -a
```

Every little thing happening on the screen right now has a chapter that will
dissect it later.

Building the Raspberry Pi target:

```bash
> cd machines/raspix/system
> make          # defaults to HW=raspix ARCH=aarch64 (see make.inc)
> make run      # QEMU simulating a Pi 3B
```

> Tip: exit QEMU with `Ctrl-A` then `x`.

## 2.7 Understanding What the Build System Does

EwokOS builds with **layered recursive Make**; for now you only need the flow:

```
machines/raspix/system/Makefile            ← entry point (chooses the board)
  ├── ../kernel/Makefile                   ← builds the kernel → kernel8.img
  ├── system/platform/aarch64/make.rule    ← defines the compiler and flags
  ├── system/basic/                        ← C library, init, shell, commands
  ├── system/network/                      ← network stack
  ├── system/gui/                          ← graphics library and display drivers
  ├── system/xwin/                         ← window system
  └── the sd target                        ← packs all artifacts into an ext3 image
```

Each layer only depends on the layer below it. `make.rule` defines things
like (see [system/platform/aarch64/make.rule](../../system/platform/aarch64/make.rule)):

```make
CC = aarch64-none-elf-gcc
CFLAGS = -march=armv8-a -ffreestanding ...
```

`-ffreestanding` means a "freestanding environment": the compiler may not
assume any standard library exists — because we are the ones **building**
that environment.

## 2.8 Debugging Prep: GDB

Kernel-level bugs often manifest as "just hangs / reboots"; breakpoint
debugging is the lifeline then. In two terminals:

```bash
# Terminal 1: start in debug mode (halts on the first instruction, waiting for GDB)
> cd machines/raspix/kernel
> make debug

# Terminal 2: start GDB and auto-connect
> make gdb
```

Common GDB commands: `b <function>` (breakpoint), `c` (continue), `s`
(step), `p <variable>` (print).

## 2.9 Hardware Preparation (Optional but Strongly Recommended)

QEMU emulates nearly everything, but **seeing a system you wrote boot on a
real Raspberry Pi** is a completely different experience — and the final goal
of this tutorial. You need three things:

### ① A Raspberry Pi

| Model | Support | Notes |
|---|---|---|
| Raspberry Pi 3B / 3B+ | ★ Recommended | EwokOS mainline support, most documentation |
| Raspberry Pi 4B | Supported | Faster; peripheral base addresses differ slightly (code already adapted) |
| Raspberry Pi 2B | Supported | The 32-bit route (ARCH=arm) |
| Raspberry Pi Zero 2W | Supported | Cheap and tiny; same core as 3B |

### ② A MicroSD Card (8 GB or larger)

The system image is about 512 MB, so 8 GB is plenty. Flashing is covered in
detail in Ch. 14.

### ③ A USB-to-TTL Serial Cable (Important!)

When the Pi runs our own system, **the screen and keyboard may not work** —
all boot logs and the command line go through the serial port. The serial
cable is the lifeline of real-hardware debugging.

Buying tip: it **must be a 3.3V-level TTL cable** (commonly PL2303 / CP2102 /
FT232 chips). A 5V RS232 cable can fry your Pi. The wires are usually black
(GND), white/green (RX), green/white (TX).

Wiring (against the 40-pin GPIO header):

```
USB-TTL cable     Raspberry Pi GPIO
─────────────     ─────────────────
GND   (black) ───→  Pin 6  (GND)
RX    (recv)  ───→  Pin 8  (GPIO14, the Pi's TX)
TX    (xmit)  ───→  Pin 10 (GPIO15, the Pi's RX)
VCC   (red)   ───→  Leave unconnected! (the Pi uses its own power)
```

> Two things to note: **RX goes to the other side's TX** (crossed); and the
> **red VCC wire stays unconnected** — the Pi is powered separately over USB.

Open a serial terminal on your computer (baud **115200**, 8N1):

```bash
# macOS (find the device name with ls /dev/cu.* first)
> screen /dev/cu.usbserial-xxxx 115200     # exit: Ctrl-A then K

# Linux
> sudo minicom -D /dev/ttyUSB0 -b 115200    # or picocom
```

Software and hardware are now ready. Chapter 14 will flash the image into an
SD card, plug in the serial cable, and watch your own OS boot on real
hardware.

## 2.10 Summary

- A cross compiler lets us build Pi-executable code on our own computer;
- QEMU runs the whole system without flashing a card;
- The real-hardware trio: a Pi (3B recommended), an SD card, a 3.3V USB-TTL
  serial cable;
- Build entry points: `machine.virt/system` (development) or
  `machines/raspix/system` (Raspberry Pi);
- You have now booted a complete EwokOS once.

Next chapter: we throw away everything ready-made and write our own
bare-metal program, starting from the very first line of assembly.
