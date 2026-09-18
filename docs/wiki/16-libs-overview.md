# 16 The EwokOS Library Ecosystem Overview

> Language: **English** | [中文](16-libs-overview.zh.md)
>
> Goal: get clear on how EwokOS's "libraries" are organized — where they
> live, how many layers they form, how they're compiled, and how they get
> linked into your program. This is the first article of the "ewokos
> series"; the next ones are [17](17-libc.md) on libc (the C runtime) and
> [18](18-libs.md) on the common libraries.

In the first 15 chapters we got the kernel, userland, and the graphics
system all running. But when you write a real program you'll find that
`printf` alone is not enough — you need to read JSON, decompress files,
draw a PNG, send an HTTP request, operate an ext2 partition on the SD card.
These capabilities can't all be stuffed into the kernel (the microkernel's
principle is "the kernel only manages mechanisms"); they exist as
**userland libraries**. This chapter first hands you a complete map.

## 16.1 Why EwokOS Rolls Its Own Libraries

On Linux, your `#include <stdio.h>` uses glibc, and `-lpng` uses the
system-installed libpng. EwokOS can't do that, for three reasons:

| Constraint | Consequence |
|------|------|
| **Microkernel + bare-metal cross-compiling** | no ready-made host libc is usable; system calls go through EwokOS's own `svc` table (Ch. 08), not the Linux ABI |
| **Static linking** | the vast majority of programs compile libraries straight into the executable, producing "self-contained" binaries suited to an environment without a dynamic loader |
| **Freestanding builds** | libraries are compiled with options like `-ffreestanding` and `-nostdlib`; they cannot assume the host system provides anything |

So EwokOS adopts **manual, source-level** dependency management:
third-party libraries (zlib, FreeType, wolfSSL, openlibm…) are placed into
the repo directly as source (vendored), cross-compiled into static `.a`
libraries with customized Makefiles, and their headers and libraries are
installed into a unified build directory for upper layers to link against.
No `apt`, no `vcpkg`, no lockfile.

## 16.2 The Layered Library Map

EwokOS's libraries are **layered**; lower layers don't know the upper ones
exist, and upper layers depend on lower ones:

```
        your app / command / driver
                 │
   ┌─────────────┼───────────────┬────────────────┐
   │             │               │                │
 network       window           graphics         basic
 libraries     libraries        libraries        libraries
(network/)   (xwin/libs)      (gui/libs)      (basic/libs)
 socket        x / x++         graph/font     sd/ext2/fat32
 wolfssl       widget++        freetype/png  tinyjson/zlib
 libwebsockets                 display/g2d   usb/gpio/c++
   │             │               │                │
   └─────────────┴───────┬───────┴────────────────┘
                         │
                     libc (the C runtime)
              libewoksys + libgloss + openlibm
                         │
                  the EwokOS kernel (svc syscalls)
```

Mapped onto directories, that's five "library settlements":

| Layer | Directory | Duty |
|----|------|------|
| **libc** | [system/basic/libc/](../../system/basic/libc/) | the C runtime: standard-library glue, syscall wrappers, the math library |
| **basic libraries** | [system/basic/libs/](../../system/basic/libs/) | low-level libraries dealing with hardware / filesystems / data formats |
| **graphics libraries** | [system/gui/libs/](../../system/gui/libs/) | 2D drawing, fonts, image codecs, display, input |
| **window libraries** | [system/xwin/libs/](../../system/xwin/libs/) | the X-style window system's client libraries and C++ widget toolkit |
| **network libraries** | [system/network/libs/](../../system/network/libs/) | socket, TLS, HTTP, WebSocket, NTP |

> The build order is exactly this dependency order: `basic` → `network` →
> `gui` → `xwin`. When you ran a full `make` in Ch. 02, the Makefile
> recursed down this chain.

## 16.3 What a Library Looks Like

EwokOS's library directories are highly uniform — know one and you know
them all. Take ext2 from the basic libraries as an example
([system/basic/libs/ext2/](../../system/basic/libs/ext2/)):

```
ext2/
├── include/          # public headers (ext2/ext2fs.h, ext2/ext2head.h)
├── src/              # architecture-independent sources
├── aarch64/          # AArch64-specific sources + build artifacts (.o)
├── arm/              # ARM32-specific sources + build artifacts
├── x86/              # x86-specific sources + build artifacts
└── Makefile          # compiles into libext2.a, installed to the build dir
```

Key points:

- **`include/` is the public interface**, `src/` is the implementation.
  Your program only does `#include <ext2/ext2fs.h>`;
- **Per-architecture subdirectories** (`aarch64/`, `arm/`, `x86/`):
  arch-specific code and `.o` artifacts each stay in their place, so one
  source tree can be compiled for multiple targets simultaneously without
  interference;
- **The Makefile installs artifacts into the unified build directory**:
  headers go to `build_<arch>/<hw>/include/`, static libraries go to
  `build_<arch>/<hw>/lib/`. Upper layers only need `-I .../include -L
  .../lib` when linking.

## 16.4 How Libraries Get Linked into Your Program

You almost never have to type a long string of `-lxxx` by hand. The
platform rules file
([system/platform/aarch64/make.rule](../../system/platform/aarch64/make.rule))
predefines three "library group" variables that Makefiles reference
directly:

```make
# the C runtime — every program links this
EWOK_LIBC = --start-group -lewoksys -lc -lgloss --end-group

# the graphics stack — link this for programs that draw / use fonts / images / display
EWOK_LIB_GRAPH = -lfont -lfreetype -liconbuf -lgraph -lg2dclient \
                 -lpng -ljpeg -lgif -ltga -lsvg -lz -lopenlibm \
                 -ldisplayman -ldisplay -lkeyb -lmouse $(BSP_LFLAGS)

# the window stack — link this for GUI apps / widgets
EWOK_LIB_X = -lwidget++ -lx++ -lx -ltinyjson -lewokstl -ltinyhttpsc -lsocket
```

Three details:

1. **`--start-group ... --end-group`**: tells the linker to scan this group
   of libraries repeatedly until no new undefined symbols appear. Because
   `libewoksys`, `libc`, and `libgloss` reference each other, a single-pass
   link would miss symbols — group scanning saves the trouble;
2. **ARM32 has an extra `-lsoftfloat`**: 32-bit ARM targets use software
   floating point (see Ch. [17](17-libc.md)); AArch64/x86 have hardware FP
   and don't need it;
3. **`$(BSP_LFLAGS)`**: pulls in the board support package's (BSP's)
   libraries, which differ across machines (Raspberry Pi / QEMU virt).

So the link line of Ch. 12's `hello` program, `$(LD) ... $(EWOK_LIBC)`,
expands behind the scenes into this entire C runtime. Programs that draw
add `$(EWOK_LIB_GRAPH)`; programs that open windows add `$(EWOK_LIB_X)`.

## 16.5 How Libraries Are Built and Installed

Each layer's `libs/` directory has a "master Makefile" listing all of that
layer's libraries in `DIRS` and declaring the header dependencies between
them. Look at the basic libraries'
([system/basic/libs/Makefile](../../system/basic/libs/Makefile)):

```make
DIRS = sd ext2 ext3 fat32 elf tinyjson openlibm zlib gpio usb \
       c++/c++ c++/stl c++/object++

# dependencies between libraries: ext2/ext3 need sd's headers installed first
ext2 ext3: sd
c++/object++: tinyjson
c++/stl c++/object++: c++/c++
```

The graphics layer
([system/gui/libs/Makefile](../../system/gui/libs/Makefile)) does the same,
declaring that `graph` depends on the image codec libraries, `font` depends
on `freetype`, `display`/`textgrid` depend on `graph`, and so on. The build
system uses these to decide the compile order, guaranteeing "depended-upon
libraries install their headers first; only then do their dependents start
compiling".

> **The build flow in one sentence**: each library `make`s → produces a
> `.a` + installs headers into `build_<arch>/<hw>/` → upper libraries/apps
> find them with `-I/-L` → finally `make sd` packs the rootfs into the
> image (Ch. 14).

## 16.6 Summary

- EwokOS uses **manual, source-level** dependency management: third-party
  libraries are vendored into the repo and cross-compiled into static `.a`;
- Libraries form **five layers**: libc, basic, graphics, window, and
  network libraries, living respectively in `basic/libc`, `basic/libs`,
  `gui/libs`, `xwin/libs`, `network/libs`;
- Every library directory has a uniform structure: `include/` (interface) +
  `src/` (implementation) + arch subdirectories (artifacts) + `Makefile`;
- Linking relies on three predefined library groups: `EWOK_LIBC` /
  `EWOK_LIB_GRAPH` / `EWOK_LIB_X`;
- The build recurses in dependency order, and artifacts are uniformly
  installed into `build_<arch>/<hw>/{include,lib}`.

Next chapter: drill into the bottommost piece of the puzzle — EwokOS's
**libc** — and see how `printf` walks all the way down to the kernel.
