# EwokOS

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/MisaZhu/ewokos)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/commits/main/)
[![GitHub License](https://img.shields.io/github/license/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/blob/main/LICENSE)

EwokOS is a lightweight microkernel operating system aimed at OS learning, system bring-up, and practical experimentation across QEMU and real embedded boards. The project combines a small kernel with a large user-space stack: core services, storage, graphics, an X-like desktop, networking, and board-specific drivers.

## Author

Misa.Z <misa.zhu@gmail.com>

## What This Repository Contains

This repository is the main EwokOS source tree. It contains:

- The microkernel core and architecture layers
- Base userland, libc pieces, shell, and system daemons
- GUI, display, audio, and image libraries
- An X-like window system and desktop applications
- Networking tools and services such as SSH, SCP, Telnet, HTTP, and ping
- Machine-specific ports for QEMU, Raspberry Pi, x86, RISC-V, handhelds, and board variants
- Extra software trees provided through submodules such as `projects/`, `sw.extra/`, and `apps/`

The build is intentionally machine-centric. Instead of one monolithic top-level build, each target under `machine.virt/` or `machines/*/` provides its own kernel and rootfs recipes.

## Latest Progress

Recent work visible in the current branch includes:

- Ext3 ported as the default root filesystem: `sdfsd` now serves journaled, crash-safe ext3, with automatic ext2-compatible fallback when no journal is present; the kernel boot loader reads ext2/ext3 directly
- SD card write performance improvements, including Raspberry Pi SD driver speed-ups, removal of per-write verification, and write-cache flushing tuned for ext3
- Display rotation speed-ups via arch-accelerated `graph_rotate_to_arch` paths (ARM, AArch64, x86) and a faster display daemon
- Larger projects (browser, `macemu`, `minivmac`, `nesemu`, `soft3d`, `video`, `saver`) moved into the new `projects/` tree
- `macemu` HiDPI mode support
- Simplified, rewritten Makefiles across the `system/` build trees

These items come directly from the latest commits in the current checkout and reflect active development rather than historical roadmap text.

## Architecture Overview

EwokOS follows a microkernel-oriented design:

- The kernel handles low-level memory management, scheduling, interrupts, IPC, semaphores, signals, and process state
- User-space services implement most system behavior, including boot orchestration, service discovery, filesystems, display management, and networking
- Graphics is built on framebuffer/display services plus an X-like window system
- Networking is implemented through user-space drivers and daemons
- Machine ports provide the BSP, boot glue, storage drivers, interrupt/timer setup, and deployment flow for each target

### Core Runtime Flow

At a high level:

1. The kernel boots and initializes MMU, IRQ, DMA/shared memory, scheduler state, and platform services.
2. The kernel launches `/sbin/init`.
3. `init` starts core user-space services such as `core`, `vfsd`, and storage helpers.
4. Machine-specific init scripts bring up console, GUI, network, and X sessions.
5. When X is enabled, the default desktop session launches window manager and desktop tools.

## Highlights

- Microkernel core with scheduling, VM/MMU support, IPC, signals, semaphores, and SMP-aware code paths
- User-space services including `init`, `core`, `vfsd`, `sdfsd`, `displaymand`, `xserverd`, `netd`, `sshd`, `telnetd`, and `httpd`
- Native GUI and desktop stack with framebuffer rendering, themed window managers, and graphical apps
- Networking tools including `ssh`, `scp`, `ping`, `host`, HTTPS test tooling, and WebSocket test tooling
- Multiple machine ports for QEMU, Raspberry Pi families, x86, RISC-V, handheld devices, and special-purpose boards
- Ext3 root filesystem image generation in-tree as the default (journaled, with ext2-compatible fallback); FAT32 support is also part of the broader stack

## Directory Structure

```text
ewokos/
├── README.md
├── LICENSE
├── kernel/
│   ├── dev/                  # Common kernel device headers
│   ├── kernel/               # Kernel core: proc, IPC, VM, scheduler, syscalls
│   ├── lib/                  # Kernel-side helper libraries and ext2/ext3 readers
│   ├── loadinit/             # Early userspace loading helpers
│   └── platform/             # ARM, AArch64, RISC-V, x86 architecture code
├── system/
│   ├── basic/                # Base userland, shell, core services, libc pieces
│   ├── gui/                  # Display, graphics, fonts, audio/image helpers
│   ├── network/              # Network tools, daemons, and related libraries
│   ├── platform/             # Userland platform build rules
│   └── xwin/                 # X-like window system, libs, apps, and WMs
├── machine.virt/
│   ├── kernel/               # Recommended QEMU virt kernel target
│   └── system/               # Recommended QEMU virt rootfs target
├── machines/
│   ├── docs/                 # SD card notes, screenshots, target-specific docs
│   ├── clockwork/            # Clockwork Pi PicoCalc target
│   ├── lego.ev3/             # LEGO Mindstorms EV3 port
│   ├── miyoo/                # Miyoo handheld port
│   ├── orangepi/             # Orange Pi port
│   ├── raspi5/               # Raspberry Pi 5 port and add-ons
│   ├── raspix/               # Raspberry Pi family ports and many overlays
│   ├── versatilepb/          # ARM versatilepb QEMU target
│   ├── virt.riscv/           # QEMU RISC-V virt target
│   ├── x2lite.rk3128/        # RK3128 target
│   └── x86/                  # x86 PC-style target
├── projects/                 # Larger standalone projects: browser, macemu,
│                             # minivmac, nesemu, soft3d, video, saver
├── sw.extra/                 # Extra apps, SDL-based software, alternate WMs
└── apps/                     # Additional apps and libraries (submodule)
```

### Key Source Directories

- `kernel/kernel/src/`: scheduler, process model, IPC, MMU, IRQ, semaphore, signal, syscall dispatch
- `kernel/platform/`: architecture-specific boot, interrupt, and MMU code
- `system/basic/sys/`: `init`, `core`, `vfsd`, and rootfs/filesystem daemons
- `system/basic/bin/`: shell and command-line tools
- `system/gui/`: display stack, graphics libraries, font and image support
- `system/xwin/`: X client libraries, input helpers, window managers, and desktop apps
- `system/network/`: network commands, libraries, and daemons
- `machines/*/kernel/`: board support packages and per-target boot logic
- `machines/*/system/`: per-target rootfs packaging, drivers, config, and init scripts
- `machines/*/3rd/`: board overlays and third-party device integrations

## Supported Architectures

| Architecture | Tool Prefix | Notes |
|-------------|-------------|-------|
| ARM32 | `arm-none-eabi-` | Used by several classic ARM and embedded targets |
| AArch64 | `aarch64-none-elf-` | Used by `machine.virt`, Raspberry Pi 5, and modern ARM64 ports |
| RISC-V 64 | `riscv64-unknown-elf-` | Used by `machines/virt.riscv/` |
| x86 / x86_64 | target-specific toolchain flow | Implemented in the dedicated `machines/x86/` port |

## Target Support Matrix

Support below is based on the checked-in source tree and build recipes in this checkout. Hardware validation depth can vary by board.

| Target Path | Architecture | Typical Use | Current Support Level | Notes |
|------------|--------------|-------------|-----------------------|-------|
| `machine.virt/` | ARM32 / AArch64 | QEMU bring-up and development | Recommended, most complete | SMP, ext3 rootfs, VirtIO block/net/input/sound, 9P host share |
| `machines/raspix/` | ARM family | Raspberry Pi family | Strong, board-focused | Broad add-on ecosystem under `3rd/`, Wi-Fi, camera, audio, LCD/touch overlays |
| `machines/raspi5/` | AArch64 | Raspberry Pi 5 | Active and advancing | WLAN, USB host, fan control, NVMe FS daemon, LCD HAT integrations |
| `machines/x86/` | x86 / x86_64 | PC-style QEMU target | Strong | Full machine-local kernel and system recipes with GUI/X stack |
| `machines/virt.riscv/` | RISC-V 64 | QEMU RISC-V virt | Good kernel and desktop bring-up | Base, GUI, and X flow present; network integration is lighter than `machine.virt` |
| `machines/versatilepb/` | ARM32 | QEMU ARM versatilepb | Stable classic target | Useful for smaller ARM bring-up and testing |
| `machines/miyoo/` | ARM32 | Handheld target | Active port | Custom drivers for audio, graphics, storage, and handheld-specific setup |
| `machines/orangepi/` | ARM / AArch64 family | Orange Pi board work | In-tree port | Kernel, system, and board packaging files are present |
| `machines/lego.ev3/` | ARM32 | LEGO Mindstorms EV3 | In-tree port | Includes EV3-specific drivers and packaging flow |
| `machines/x2lite.rk3128/` | ARM32 | RK3128 target | In-tree port | Kernel and system recipes are present |
| `machines/clockwork/picocalc/` | ARM target | Clockwork Pi PicoCalc | Specialized target | Separate board package with custom kernel/system setup |
| `machines/raspix/3rd/clockwork/uconsole/` | console/display environment | Clockwork uConsole support files | Specialized support tree | `raspix` overlay focused on device/display and board support assets |

## Board and Peripheral Support

The tree already includes a large amount of board-specific and peripheral work.

### Storage and Filesystems

- Ext3 root filesystem image generation is the default build flow, with journaling for crash safety; cards without a journal are served in ext2-compatible mode
- The kernel boot loader can read ext2/ext3 directly to load init
- FAT32 support has also been ported into the stack
- SD and MMC support exist across many machine BSPs, with recent write-path speed-ups (faster Raspberry Pi SD transfers, removed per-write verification)
- Raspberry Pi 5 includes `nvmefsd` in its system drivers

### Display and Input

- Framebuffer-based display stack
- X-like desktop window system
- Multi-display work is actively landing
- USB keyboard and mouse support on QEMU and x86-style targets
- Touch controller integrations under Raspberry Pi add-on trees such as `gt911` and `xpt2046`
- Many LCD HAT and Waveshare-style overlay trees for Raspberry Pi boards

### Networking

- User-space networking stack and daemons
- `ssh`, `scp`, `ping`, `host`, `telnet`, `https_test`, and `ws_test`
- `sshd`, `telnetd`, `httpd`, and machine-specific network drivers
- Raspberry Pi WLAN work with Broadcom-related sources under `machines/raspix/` and `machines/raspi5/`

### Graphics and Desktop

- Framebuffer rendering, 2D helpers, fonts, PNG/JPEG/GIF/SVG/TGA support
- Arch-accelerated graphics paths, including scaling and rotation (`graph_rotate_to_arch`) for ARM, AArch64, and x86
- X client libraries in both C and C++ forms
- Desktop apps such as `xterm`, `xread`, `ximg`, `xlog`, `xapps`, `clock`, `cards`, and `mine`
- Alternate window managers and demos in `sw.extra/`

### Extra Software

- `projects/` holds the larger standalone projects: a web browser, `macemu` and `minivmac` Macintosh emulators, `nesemu`, the `soft3d` software 3D stack, `video`, and `saver`
- `sw.extra/` contains demos and larger apps such as `doom`, `calculator`, `calendar`, and SDL-related pieces
- `apps/` may hold additional applications and libraries depending on your checkout
- Some installations depend on submodules being checked out locally

## Project Components

### Kernel

The kernel source under `kernel/` includes:

- Process scheduler and ready queues
- IPC, kernel event queues, and semaphore handling
- Signal delivery and syscall dispatch
- MMU, DMA, shared memory, and kernel allocators
- SMP-related support
- Architecture-specific boot and interrupt handling

### Base User Space

The base system under `system/basic/` includes:

- Shell, login flow, and standard command-line tools
- Core services such as `init`, `core`, and `vfsd`
- Filesystem helpers and rootfs daemons
- Basic libc and utility libraries (see [C Library, Standards, and Bundled Libraries](#c-library-standards-and-bundled-libraries))

### GUI and X Stack

The graphics stack is split between `system/gui/` and `system/xwin/`:

- Display, font, graph, and audio/image support libraries
- X client libraries and input helpers
- Desktop apps and window managers
- Machine-specific display configs and init scripts

### Network Stack

The network tree under `system/network/` provides:

- Interactive tools such as `ssh`, `scp`, `ping`, and `host`
- Network daemons such as `sshd`
- Additional services and drivers wired in through machine-specific system trees

## C Library, Standards, and Bundled Libraries

EwokOS ships its own C library, `libewoksys`, instead of relying on a hosted libc. It provides ISO C standard headers plus a POSIX-like API subset, layered over the cross-toolchain runtime (`newlib` + `libgloss`), with `openlibm` for math and `softfloat` for software floating point on ARM32.

The default link group is declared per architecture in `system/platform/<arch>/make.rule`:

```make
# AArch64 / x86
EWOK_LIBC = --start-group -lewoksys -lc -lgloss --end-group
# ARM32 additionally links the software FP runtime
EWOK_LIBC = --start-group -lewoksys -lc -lgloss -lsoftfloat --end-group $(LIBGCC)
```

### libc Source Layout

- `system/basic/libc/libewoksys/` — the native EwokOS C library: standard headers, POSIX-like implementations, and the EwokOS system API (IPC, VFS, devices, shared memory, processes, logging)
- `system/basic/libc/libgloss/` — toolchain syscall glue (`_open`, `_close`, `_read`, `_write`, `_lseek`, `_stat`, `_fstat`, `_sbrk`, `_kill`, `_fork`, `_wait`, `_getpid`, `_gettimeofday`, `_isatty`, ...)
- `system/basic/libc/softfloat/` — software floating-point / `aeabi` runtime helpers used on ARM32 (`-mfloat-abi=softfp`)

### Supported Standards

**ISO C (C90/C99 subset)** — provided by the `libewoksys` headers:
`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<strings.h>`, `<ctype.h>`, `<math.h>`, `<assert.h>`, `<errno.h>`, `<setjmp.h>`, `<time.h>`, `<float.h>`, `<inttypes.h>`, `<stdarg.h>`, `<stddef.h>`, `<wchar.h>`, `<malloc.h>`.

**POSIX-like API** — a practical subset oriented at embedded and system programming:

| Area | Headers | Representative APIs |
|------|---------|---------------------|
| Process & files | `<unistd.h>`, `<fcntl.h>`, `<dirent.h>`, `<sys/stat.h>`, `<sys/wait.h>` | fork/exec family, `getpid`/`getuid`/`getgid`, `dup`/`dup2`/`dup3`, `pipe`, `getcwd`/`chdir`, `fsync`, `truncate`/`ftruncate`, `sysconf`, `daemon`, `wait` |
| Memory | `<sys/mman.h>`, `<malloc.h>` | `mmap`/`munmap` with `PROT_*` / `MAP_*` flags |
| Signals | `<signal.h>` | signal handlers and delivery |
| Threads | `<pthread.h>`, `<semaphore.h>` | mutexes, condition variables, rwlocks, barriers, keys, `pthread_once`, attributes, cancel/detach/join, POSIX semaphores |
| Terminal, I/O multiplexing & scheduling | `<termios.h>`, `<sched.h>`, `<poll.h>`, `<sys/select.h>` | termios control, `poll`/`select` readiness, `sched_yield` |
| Shared memory & IPC | `<sys/shm.h>`, `<sys/ipc.h>` | `shmget`/`shmat`/`shmdt`/`shmctl` |
| System info & scatter/gather I/O | `<sys/ioctl.h>`, `<sys/uio.h>`, `<sys/utsname.h>`, `<sys/resource.h>`, `<sys/statvfs.h>`, `<sys/time.h>`, `<sys/times.h>` | `ioctl`, `readv`/`writev`, `uname`, resource/`statvfs` queries, time-of-day |
| Misc utilities | `<getopt.h>`, `<glob.h>`, `<fnmatch.h>`, `<libgen.h>`, `<err.h>` | argument parsing, globbing, pattern matching, path helpers |

**BSD Sockets** — via the `socket` library (`system/network/libs/socket`):
`<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`, `<netinet/if_ether.h>`, `<netinet/ether.h>`, with `socket`, `bind`, `listen`, `accept`, `connect`, `send`/`recv`, `sendmsg`/`recvmsg`, `shutdown`, `socketpair`, `getsockname`/`getpeername`, `gethostbyname`, and `inet_addr`/`inet_ntoa`/`inet_pton`/`inet_ntop`.

**C++** — `libcxx` (C++ runtime support) plus `libewokstl`, a lightweight STL subset:
`string`/`wstring`, `vector`, `list`, `deque`, `queue`, `stack`, `set`, `map`, `unordered_set`, `unordered_map`, `algorithm`, `iterator`, `functional`, `memory`, `tuple`, `utility`, `iostream`/`sstream`/`fstream`, and the header-only `object++` (`UniObject`) helper.

> The libc favors a compact, embedded-appropriate subset. A few hosted-libc features are intentionally absent or simplified — for example `vprintf` is not provided; use `vfprintf(stdout, ...)` instead.

### Commonly Bundled Libraries

Beyond libc, the tree builds a set of static libraries grouped under `system/*/libs`:

| Library | Location | Purpose |
|---------|----------|---------|
| `ewoksys` | `system/basic/libc/libewoksys` | Native EwokOS system API: IPC, VFS/vdevice, processes, shared memory, DMA, logging (`slog`/`klog`), timers, sessions |
| `openlibm` | `system/basic/libs/openlibm` | Math library (`-lopenlibm`) |
| `zlib` | `system/basic/libs/zlib` | Deflate compression (`-lz`) |
| `tinyjson` | `system/basic/libs/tinyjson` | Minimal JSON parser/builder |
| `sd` / `ext2` / `ext3` / `fat32` | `system/basic/libs/*` | Storage and filesystem readers/writers |
| `elf` | `system/basic/libs/elf` | ELF parsing/loading |
| `gpio` / `usb` | `system/basic/libs/*` | GPIO and USB helper libraries |
| `socket` | `system/network/libs/socket` | BSD sockets client API |
| `wolfssl` | `system/network/libs/wolfssl` | TLS/SSL |
| `libwebsockets` | `system/network/libs/libwebsockets` | WebSocket client/server |
| `libtinyhttpsc` | `system/network/libs/libtinyhttpsc` | Tiny HTTPS client |
| `ntpc` | `system/network/libs/ntpc` | NTP client |
| `graph` / `g2dclient` | `system/gui/libs/*` | 2D framebuffer graphics and 2D-acceleration client |
| `display` / `displayman` | `system/gui/libs/*` | Display service client libraries |
| `font` / `freetype` / `libiconbuf` | `system/gui/libs/*` | Font rendering (bitmap + FreeType) |
| `libpng` / `libjpeg` / `libgif` / `libtga` / `libsvg` | `system/gui/libs/*` | Image codecs |
| `libogg` / `libvorbis` / `minimp3` | `system/gui/libs/*` | Audio codecs |
| `textgrid` / `gterminal` | `system/gui/libs/*` | Text grid and terminal emulation |
| `keyb` / `mouse` | `system/gui/libs/*` | Keyboard and mouse input clients |
| `x` / `x++` / `widget++` | `system/xwin/libs/*` | X-like window system client libs (C, C++, widget toolkit) |

The graphics and window-system groups are also declared in `make.rule` as `EWOK_LIB_GRAPH` and `EWOK_LIB_X`, so applications can link them as a unit.

## Host Requirements

At minimum you need:

- `make`
- A matching cross toolchain for the architecture you want to build
- QEMU for emulator targets
- `e2tools` and `mke2fs` or `e2fsprogs` for ext2/ext3 image creation

### Typical Linux Packages

```bash
sudo apt-get install make qemu-system-arm qemu-system-aarch64 qemu-system-misc e2tools e2fsprogs
```

On macOS, install the same categories of tools with Homebrew or another package manager, plus the cross compilers required for the target architecture.

## Quick Start with `machine.virt`

`machine.virt/` is the easiest place to start because it runs entirely in QEMU and has the most complete out-of-the-box development flow.

### 1. Build the Root Filesystem

```bash
cd machine.virt/system
make x
make sd
```

This stages the root filesystem under `system/build/virt/rootfs/` and creates `root_aarch64.img` under `system/build/virt/`. The image is ext3 by default; pass `FS=ext2` to `make sd` for an ext2 image instead.

### 2. Build the Kernel

```bash
cd machine.virt/kernel
make
```

The default `machine.virt` configuration is:

- `ARCH=aarch64`
- `SMP=yes`
- `PAGE_SIZE=16k`

Recent kernel work also adds `64k` page support for AArch64-oriented ports.

### 3. Run in QEMU

```bash
cd machine.virt/kernel
make run
```

The current QEMU recipe includes:

- RAM framebuffer output
- VirtIO block storage
- VirtIO network
- VirtIO tablet and keyboard devices
- VirtIO sound
- 9P host sharing mounted as `hostshare`
- Optional host forwarding through `QEMU_HOSTFWD` or `QEMU_NETDEV`

### 4. Log In

Default users from `system/basic/etc/passwd`:

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
make gui      # framebuffer GUI stack
make x        # full desktop stack
make sd       # generate ext3 rootfs image (FS=ext2 supported)
make clean
```

### `machine.virt/kernel`

```bash
make          # build the kernel image
make asm      # generate kernel assembly listing
make run      # boot QEMU with GUI
make run-headless
make run-gui  # same as make run (explicit GUI form)
make run-gui-hvf  # GUI run accelerated with QEMU HVF (host CPUs)
make runasm   # boot with instruction tracing
make debug    # QEMU paused with GDB server on :26000
make debugasm
make gdb
```

### Common Variants

```bash
# Build a 32-bit ARM virt rootfs and kernel
cd machine.virt/system && make ARCH=arm x && make ARCH=arm sd
cd ../kernel && make ARCH=arm && make ARCH=arm run

# Force a specific QEMU display backend
QEMU_DISPLAY_OPTS=cocoa make run

# Customize host forwarding
QEMU_HOSTFWD='hostfwd=tcp:127.0.0.1:2022-:22' make run
```

## Working with Other Targets

Most machine ports follow the same pattern:

1. Build the target root filesystem in `machines/<target>/system`
2. Generate the ext2/ext3 image or board-specific storage image
3. Build the kernel in `machines/<target>/kernel`
4. Run in QEMU or deploy to SD card, eMMC, or board storage as appropriate

For SD-card-oriented flows, see `machines/docs/make_sd.md`.

## Source Reading Guide

If you want to understand the codebase quickly, start here:

1. `kernel/kernel/src/` for scheduler, process state, IPC, signals, and VM
2. `kernel/platform/` for boot, interrupts, and MMU differences by architecture
3. `system/basic/sys/` for `init`, `core`, and `vfsd`
4. `system/gui/` and `system/xwin/` for graphics, desktop, and X client APIs
5. `system/network/` for daemons, tools, and protocol support
6. `machine.virt/` for the simplest full boot path
7. `machines/raspix/` and `machines/raspi5/` for the most active hardware bring-up work

## Documentation

- [DeepWiki](https://deepwiki.com/MisaZhu/ewokos)
- `machine.virt/` and `machines/*/` for target-specific build files
- `kernel/platform/` and `system/platform/` for architecture and build rules
- `machines/docs/` for SD card notes, board docs, and screenshots

## Notes on Submodules

This repository may rely on submodules depending on your checkout state. The current `.gitmodules` file references trees such as:

- `sw.extra`
- `machines`
- `projects`

If a directory mentioned in the documentation is missing locally, initialize submodules before building or exploring the full software set.

## Contributing

Contributions are welcome. Bug reports, platform notes, board bring-up findings, documentation improvements, and pull requests are all useful.

## License

This project is licensed under the terms in `LICENSE`.
