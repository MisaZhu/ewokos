# EwokOS

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/MisaZhu/ewokos)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/commits/main/)
[![GitHub License](https://img.shields.io/github/license/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/blob/main/LICENSE)

EwokOS is a lightweight microkernel operating system for learning OS internals, experimenting with user-space system services, and bringing the same software stack to QEMU and real embedded hardware.

## Author

Misa.Z <misa.zhu@gmail.com>

## Overview

The project combines a small microkernel with a user-space runtime that includes core system services, a virtual file system daemon, graphics and windowing, networking, and a growing collection of apps. The repository is organized around platform-specific `Makefile` builds instead of a single top-level build system, so each target machine ships with its own kernel and root filesystem recipes.

## Highlights

- Microkernel design with process scheduling, MMU support, copy-on-write, signals, semaphores, and message-based IPC
- User-space system services such as `init`, `core`, `vfsd`, `sessiond`, `displaymand`, `xserverd`, `netd`, `sshd`, `telnetd`, and `httpd`
- Native GUI stack with framebuffer graphics, an X-like window system, themable window managers, and desktop apps
- Integrated networking with TCP/IP services plus SSH, SCP, Telnet, ping, host lookup, HTTPS test tooling, and WebSocket test tooling
- Multiple hardware ports spanning QEMU `virt`, Raspberry Pi families, RISC-V, x86, handhelds, and board-specific variants
- Ext2 root filesystem image generation built directly from the repository tree

## Supported Architectures

- ARM 32-bit via `arm-none-eabi-*`
- ARM 64-bit / AArch64 via `aarch64-none-elf-*`
- RISC-V 64-bit via `riscv64-unknown-elf-*`
- x86 support in a dedicated machine port

## Main Targets

| Path | Typical Use |
|------|-------------|
| `machine.virt/` | Recommended QEMU `virt` target for first-time builds |
| `machines/raspix/` | Raspberry Pi 1/2/3/4 family and related add-on variants |
| `machines/raspi5/` | Raspberry Pi 5 bring-up and native board support |
| `machines/virt.riscv/` | QEMU RISC-V virt target |
| `machines/x86/` | x86 PC-style target |
| `machines/clockwork/picocalc/` | Clockwork Pi PicoCalc |
| `machines/orangepi/` | Orange Pi target |
| `machines/miyoo/` | Miyoo handheld target |
| `machines/lego.ev3/` | LEGO Mindstorms EV3 target |
| `machines/versatilepb/` | ARM versatilepb target for QEMU |
| `machines/x2lite.rk3128/` | RK3128-based board target |

Additional board-specific overlays and device integrations live under `machines/*/3rd/`.

## Project Layout

```text
ewokos/
├── kernel/              # Kernel core, architecture code, and low-level libraries
├── system/              # Base userland: libc, commands, services, GUI, networking, X stack
├── sw.extra/            # Extra apps, libraries, alternate X window managers
├── apps/                # Additional app and library collection
├── machine.virt/        # QEMU virt kernel and rootfs build for ARM/ARM64
└── machines/            # Hardware- and board-specific ports
```

### Key Subsystems

- `kernel/kernel/src/`: scheduler, IPC, process management, memory management, IRQ handling, SMP support
- `system/basic/`: shell, login/session flow, `init`, `core`, `vfsd`, file utilities, libc pieces
- `system/gui/`: framebuffer, display service, graphics/audio helpers
- `system/xwin/`: X-like client libraries, drivers, window managers, desktop apps
- `system/network/`: networking drivers, libraries, CLI tools, and daemons
- `sw.extra/` and `apps/`: extra demos, browsers, emulators, SDL- and widget-based software

## Host Requirements

At minimum you need:

- A matching cross toolchain for the architecture you want to build
- `make`
- QEMU for emulator targets
- `e2tools` and `mke2fs` / `e2fsprogs` for ext2 image creation

### Toolchain Prefixes Used by the Build

| Architecture | Tool Prefix |
|-------------|-------------|
| ARM32 | `arm-none-eabi-` |
| AArch64 | `aarch64-none-elf-` |
| RISC-V 64 | `riscv64-unknown-elf-` |

### Typical Packages

On macOS, install the required cross toolchains, `qemu`, and ext2 tooling with Homebrew or your preferred package manager.

On Debian/Ubuntu-like systems, install:

```bash
sudo apt-get install make qemu-system-arm qemu-system-aarch64 qemu-system-misc e2tools e2fsprogs
```

You also need the appropriate cross compiler packages or locally installed toolchains that provide the prefixes listed above.

## Quick Start with `machine.virt`

`machine.virt` is the easiest target to boot because it runs entirely in QEMU and defaults to AArch64 with SMP enabled.

### 1. Build the Root Filesystem

```bash
cd machine.virt/system
make x
make sd
```

This produces a staged root filesystem under `system/build/virt/rootfs/` and an ext2 image named `root_aarch64.ext2` under `system/build/virt/`.

### 2. Build the Kernel

```bash
cd machine.virt/kernel
make
```

The default config builds an AArch64 image with:

- `ARCH=aarch64`
- `SMP=yes`
- `PAGE_SIZE=16k`

### 3. Run in QEMU

```bash
cd machine.virt/kernel
make run
```

The QEMU setup for `machine.virt` currently includes:

- RAM framebuffer output
- VirtIO block storage for the root filesystem image
- VirtIO network
- VirtIO keyboard and tablet input
- VirtIO sound
- 9P host sharing mounted with the tag `hostshare`
- User-mode network forwarding for `127.0.0.1:2222 -> 22` and `127.0.0.1:2323 -> 23`

### 4. Log In

Default users are defined in `system/basic/etc/passwd`:

| User | Password |
|------|----------|
| `root` | `root` |
| `misa` | `misa` |
| `guest` | empty |

## Useful Build Targets

### `machine.virt/system`

```bash
make basic    # base userland only
make network  # base userland + networking
make gui      # adds framebuffer GUI components
make x        # full base desktop stack (default)
make sd       # generate ext2 root filesystem image
make clean
```

### `machine.virt/kernel`

```bash
make          # build the kernel image
make asm      # generate kernel assembly listing
make run      # boot QEMU normally
make runasm   # boot QEMU with instruction trace output
make debug    # start QEMU paused with a GDB server on :26000
make debugasm # debug + instruction trace
make gdb      # connect GDB to :26000
```

### Common Variants

```bash
# Build a 32-bit ARM virt kernel/rootfs
cd machine.virt/system && make ARCH=arm x && make ARCH=arm sd
cd ../kernel && make ARCH=arm && make ARCH=arm run

# Override display backend selection if needed
QEMU_DISPLAY_OPTS=cocoa make run

# Customize QEMU user networking
QEMU_NETDEV='user,id=net0,hostfwd=tcp:127.0.0.1:2022-:22' make run
```

## Boot and Runtime Flow

At a high level the system boots like this:

1. Kernel startup initializes MMU, IRQ, DMA/shared memory, process state, and platform services.
2. The kernel launches `/sbin/init`.
3. `init` starts user-space core services such as `core`, `vfsd`, and storage/filesystem helpers, then executes init scripts.
4. When the X stack is enabled, the default session starts `xwm_ewok`, `statusbar`, and `xlauncher`.

The default X session script lives in `system/xwin/etc/x/xinit.rd`.

## Userland Components

### Base Commands

The base image currently builds utilities including:

- `shell`, `login`, `session`, `ls`, `ps`, `pwd`, `cat`, `cp`, `rm`, `mkdir`
- `mount`, `grep`, `head`, `more`, `vi`, `kill`, `whoami`, `chmod`, `chown`, `chgrp`
- `date`, `json`, `sysinfo`, `svcinfo`, `uname`, `elfinfo`, `mmio`, `rx`

### Network Commands and Services

Networking-related binaries currently include:

- CLI tools: `ipconfig`, `ping`, `host`, `https_test`, `telnet`, `ssh`, `scp`, `ws_test`
- Daemons and drivers: `netd`, `httpd`, `sshd`, `telnetd`, and platform-specific network drivers

### Desktop Apps

The built-in X desktop currently includes apps such as:

- `xterm`, `xread`, `xlog`, `xfinder`, `ximg`, `xprocs`, `xcores`, `xfonts`
- `xapps`, `xipconfig`, `clock`, `cards`, `mine`, `SndPlayer`, `xDemo`, `wDemo`, `xtheme`, `xwm_theme`

### Extra Apps

Additional software under `sw.extra/` and `apps/` includes:

- Graphics and demo apps: `3ddemo`, `Fireworks`, `calculator`, `calendar`, `doom`, `gears`, `imgui_demo`, `matrix`, `waterdrops`
- Browser and multimedia experiments: `xBrowser`, `VideoPlayer`
- Extra app packs and examples: NES emulator, `minivmac`, `kimi_chat`, `xDraw`, screen saver examples
- Alternate window managers: `mac1984`, `openlook`, and `opencde`

## Working with Other Targets

Most machine ports follow the same pattern:

1. Build the target root filesystem in `machines/<target>/system`
2. Generate the ext2 or board-specific image for that machine
3. Build the kernel in `machines/<target>/kernel`
4. Run in QEMU or deploy to SD/storage media for real hardware

For SD-card-oriented workflows, see `machines/docs/make_sd.md`.

## Documentation

- [DeepWiki](https://deepwiki.com/MisaZhu/ewokos)
- Platform-specific build settings in `machine.virt/` and `machines/`
- Architecture rules in `kernel/platform/` and `system/platform/`

## Contributing

Contributions are welcome. Issues, bug reports, platform notes, and pull requests are all useful.

## License

This project is licensed under the terms in `LICENSE`.

## Source Reading Tips

Start with `kernel/kernel/src/` for the scheduler, process model, IPC, and memory code, then move to:

- `system/basic/sys/` for `init`, `core`, and `vfsd`
- `system/xwin/libs/x/` for the graphics/windowing client stack
- `system/network/` for services and protocol support
- `machine.virt/` for the easiest end-to-end boot path
