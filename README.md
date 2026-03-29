# EwokOS

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/MisaZhu/ewokos)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/commits/main/)
[![GitHub License](https://img.shields.io/github/license/MisaZhu/ewokos)](https://github.com/MisaZhu/ewokos/blob/main/LICENSE)

A lightweight microkernel operating system designed for educational purposes and embedded systems.

## Author

Misa.Z <misa.zhu@gmail.com>

## Overview

EwokOS is a microkernel-based operating system that demonstrates core OS concepts including memory management, process scheduling, inter-process communication, and device drivers. It is designed to be simple, modular, and easy to understand for learning operating system internals.

## Supported Platforms

EwokOS currently supports the following architectures and platforms:

### Architectures
- **ARM 32-bit** (ARMv5, ARMv6, ARMv7)
- **ARM 64-bit** (ARMv8/AArch64)
- **RISC-V 64-bit** (RV64)

### Main Platforms
| Platform | Architecture | Status | Description |
|----------|-------------|--------|-------------|
| `machine.virt` | ARM64/ARM | ✅ Supported | QEMU virt machine (recommended for beginners) |
| `machine.raspix` | ARM | ✅ Supported | Raspberry Pi 1/2/3/4, 5 (planned) |

### Third-Party Hardware (machine.3rd)
| Platform | Architecture | Status | Description |
|----------|-------------|--------|-------------|
| `clockwork.picocalc` | ARM | ✅ Supported | Clockwork Pi PicoCalc (RK3506) |
| `clockwork.uconsole` | ARM | ✅ Supported | Clockwork Pi uConsole |
| `versatilepb` | ARM | ✅ Supported | QEMU versatilepb |
| `lego.ev3` | ARM | ✅ Supported | LEGO Mindstorms EV3 |
| `miyoo` | ARM | ✅ Supported | Miyoo handheld console |
| `orangepi` | ARM64 | ✅ Supported | Orange Pi boards |
| `virt.riscv` | RISC-V | ✅ Supported | QEMU RISC-V virt machine |
| `raspix` variants | ARM | ✅ Supported | Various RPi LCD HATs (Waveshare, etc.) |

## System Features

### Kernel Features
- **Microkernel Architecture** - Minimal kernel with services running in user space
- **MMU Support** - Memory Management Unit for virtual memory
- **SMP Multi-Core** - Symmetric Multi-Processing support (up to 4 cores)
- **Copy-on-Write** - Efficient memory sharing between processes
- **Multi-Process** - Multiple processes with preemptive scheduling
- **Multi-Threading** - Thread support within processes
- **IPC** - Inter-Process Communication via message passing
- **Virtual File System** - "Everything is a file" philosophy
- **Signal Handling** - POSIX-like signal mechanism
- **Semaphore** - Synchronization primitives

### Device Support
- **Framebuffer** - Graphics display support via framebuffer device service
- **UART** - Serial communication via UART device service
- **SD Card** - SD card storage support
- **USB** - USB device support
- **VirtIO** - VirtIO devices for QEMU (block, network, input, 9P)
- **Network** - TCP/IP networking stack

### Graphics & UI
- **X Window System** - X-like windowing system (xwin)
- **Framebuffer GUI** - Direct framebuffer graphics (fbgui)
- **Window Manager** - Multiple window manager themes (mac1984, openlook, opencde)
- **Applications** - Calculator, Calendar, Tetris, Fireworks, 3D Demo, etc.

## Project Structure

```
ewokos/
├── kernel/          # Kernel source code
│   ├── kernel/      # Core kernel (scheduler, IPC, memory)
│   ├── lib/         # Kernel libraries
│   ├── dev/         # Device headers
│   └── platform/    # Architecture-specific code
├── system/          # System applications and libraries
│   ├── basic/       # Basic system commands
│   ├── gui/         # GUI libraries and drivers
│   ├── xwin/        # X Window system
│   └── network/     # Network stack
├── sw.extra/        # Extra applications and libraries
│   ├── apps/        # Extra GUI applications
│   ├── libs/        # Additional libraries
│   └── sdl2/        # SDL2 port
├── machine.virt/    # QEMU virt machine support
├── machine.raspix/  # Raspberry Pi support
└── machine.3rd/     # Third-party machine ports
```

## Quick Start for Beginners

### Prerequisites

#### macOS (with Homebrew)
```bash
# Install ARM toolchain
brew tap PX4/homebrew-px4
brew install gcc-arm-none-eabi-49

# Install tools for creating ext2 images
brew install e2tools

# Install QEMU
brew install qemu
```

#### Ubuntu/Debian
```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi

# Install tools
sudo apt-get install e2tools qemu-system-arm qemu-system-aarch64
```

### Building and Running machine.virt

The `machine.virt` platform is the recommended starting point for beginners as it runs on QEMU without requiring physical hardware.

#### 1. Build the Kernel

```bash
cd machine.virt/kernel
make
```

This will build the kernel image (`kernel8.img` for ARM64).

#### 2. Build the Root Filesystem

```bash
cd machine.virt/system
make
make extra # Optional: Full system with X Window and extra apps
make sd # This creates `root_aarch64.ext2` (or `root_arm.ext2` for 32-bit).

#### 3. Run on QEMU

```bash
cd machine.virt/kernel
make run
```

This launches QEMU with the EwokOS kernel and root filesystem.

#### 4. Login

When the system boots, login with:
- **Username**: `root`
- **Password**: (none, just press Enter)

### Available Make Targets

```bash
cd machine.virt/kernel

make          # Build kernel
make run      # Run on QEMU
make debug    # Run with GDB debug server
make gdb      # Connect GDB debugger
make asm      # Generate assembly listing
```

### QEMU virt Machine Features

The `machine.virt` platform supports:
- **Graphics**: RAMFB display with Cocoa/SDL output
- **Storage**: VirtIO block device (root filesystem)
- **Network**: VirtIO network device
- **Input**: VirtIO keyboard and tablet (mouse)
- **File Sharing**: 9P filesystem for host-guest sharing
- **SMP**: Multi-core support (up to 4 cores)

### Example: Running with Different Configurations

```bash
# Run with 4 CPU cores
cd machine.virt/kernel
make SMP=yes run

# Run with specific display options
QEMU_DISPLAY_OPTS=cocoa make run

# Debug mode (wait for GDB connection)
make debug

# In another terminal, connect GDB
make gdb
```

## Development Tools

### Serial Console (minicom)
```bash
# Install minicom
brew install minicom  # macOS
sudo apt-get install minicom  # Ubuntu

# Configure for USB-to-TTL serial adapter
minicom -D /dev/ttyUSB0 -b 115200
```

### Creating ext2 Images (macOS)

```bash
# Install dependencies
brew install e2fsprogs macfuse

# Create empty image
dd if=/dev/zero of=root.ext2 bs=1024 count=16384

# Create ext2 filesystem
mke2fs -b 1024 -I 128 root.ext2

# Mount and copy files
mkdir -p tmp
fuse-ext2 -o force,rw+ root.ext2 tmp
# ... copy files to tmp/ ...
umount tmp
rmdir tmp
```

## System Commands

Basic commands available in EwokOS:

| Command | Description |
|---------|-------------|
| `ls` | List directory contents |
| `ps` | List processes |
| `pwd` | Print working directory |
| `cat` | Display file contents |
| `cp` | Copy files |
| `rm` | Remove files |
| `mkdir` | Create directories |
| `mount` | Mount filesystems |
| `vi` | Text editor |
| `shell` | Command shell |

## Documentation

- [DeepWiki Documentation](https://deepwiki.com/MisaZhu/ewokos)
- Source code includes inline comments
- Architecture-specific documentation in `kernel/platform/`

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the terms specified in the LICENSE file.

## Tips for Source Code Reading

> Don't fall in love with assembly too much ;)

Start with the kernel core (`kernel/kernel/src/`) to understand scheduling and IPC, then explore the platform-specific code for your target architecture.
