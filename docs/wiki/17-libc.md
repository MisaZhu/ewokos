# 17 EwokOS's libc (the C Runtime)

> Language: **English** | [中文](17-libc.zh.md)
>
> Goal: take EwokOS's C runtime apart and see clearly how the names you use
> every day — `printf`, `fopen`, `malloc`, `getpid` — land layer by layer
> onto the EwokOS kernel's `svc` syscalls.
> This is the second article of the "ewokos series", continuing the library
> ecosystem map from [16](16-libs-overview.md).

Ch. 12 said that EwokOS's C library consists of `libewoksys` + `libgloss`,
linked into `EWOK_LIBC`. This chapter expands that sentence into a complete
"call chain" diagram.

## 17.1 The Pieces of the libc Puzzle

Open [system/basic/libc/](../../system/basic/libc/) and you'll see:

```
libc/
├── libewoksys/    # the EwokOS system-interface layer (the most important)
├── libgloss/      # newlib-style C library glue + syscall stubs
├── softfloat/     # software floating point (only needed by ARM32)
└── Makefile
```

Together with [openlibm](../../system/basic/libs/openlibm/) from the basic
libraries (the math library `libm`), and the cross toolchain's built-in
**newlib** (providing the "upper half" of standard C functions like
`printf`/`malloc`), they form the "C runtime" an EwokOS program sees. The
division of labor:

| Component | From | Provides | Link name |
|------|------|----------|--------|
| **newlib** | ships with the cross toolchain | standard C functions: the "formatting/buffering" logic of `printf`, `scanf`, `malloc`, `fopen` | `-lc` (toolchain) |
| **libgloss** | implemented by EwokOS itself | the **syscall stubs** newlib requires: `_read`, `_write`, `_sbrk`, `_open`… plus `mem*`/`str*` | `-lgloss`, `-lc` (EwokOS) |
| **libewoksys** | implemented by EwokOS itself | the **EwokOS system interface**: syscall wrappers, IPC, VFS, processes/threads, proto serialization, logging | `-lewoksys` |
| **openlibm** | vendored third-party | high-quality math functions: `sin`, `cos`, `sqrt`, `pow`… | `-lopenlibm` |
| **softfloat** | vendored third-party | software floating-point arithmetic (ARM32 only) | `-lsoftfloat` |

> The key insight: newlib's `printf` only handles "assembling the format
> string into a string"; for the step of actually "sending the string out",
> it calls a stub function named `_write`. **Whoever fills in this stub
> determines which operating system the libc runs on.** EwokOS fills it
> with libgloss.

## 17.2 libgloss: Wiring newlib onto EwokOS

The libgloss directory
([system/basic/libc/libgloss/](../../system/basic/libc/libgloss/)) has only
two source files:

- **`syscalls.c`** — the "system namespace" stub functions newlib demands,
  all starting with `_`:

  ```c
  int _read (int fd, void *buf, size_t size);     // → the VFS read of libewoksys
  int _write(int fd, const void *buf, size_t size);// → the VFS write of libewoksys
  int _open (const char *fname, int oflag, ...);
  int _close(int fd);
  _off_t _lseek(int fd, _off_t offset, int whence);
  int _fstat(int fd, struct stat *st);
  int _stat (const char *fname, struct stat *st);
  int _unlink(const char *path);
  int _isatty(int fd);
  pid_t _getpid(void);
  void *_sbrk(ptrdiff_t incr);                     // where malloc's memory comes from
  int _fork(void);  int _wait(int *status);
  int _execve(const char *name, char *const argv[], char *const env[]);
  void _exit(int err);
  int _gettimeofday(struct timeval *tp, void *tzvp);
  clock_t _times(struct tms *tp);
  ```

  The file opens with `#include <ewoksys/vfs.h>`, `<ewoksys/core.h>`,
  `<ewoksys/kernel_tic.h>` — every stub's implementation is a forward call
  to the corresponding libewoksys function. This is what "glue" means.

- **`compat.c`** — fills in common functions the toolchain doesn't provide
  or that got disabled by `-fno-builtin-*`:
  `memcpy`, `memmove`, `memset`, `memcmp`, `strcpy`, `strcmp`, `strlen`,
  `strchr`… plus `ewok_longjmp`.

  > **Performance note**: EwokOS's make.rule forces `-fno-builtin` for
  > `mem*`, so all `memcpy`/`memset` land in compat.c. The implementation
  > here moves data **by word** rather than byte by byte — crucial for
  > high-traffic paths like screen compositing, IPC buffers, and
  > shared-memory copies.

libgloss's Makefile packs these two `.o` files into both `libgloss.a`
**and** `libc.a`, so the `-lc -lgloss` in `EWOK_LIBC` both resolve to
EwokOS's own glue, while the "upper half" of the standard C functions still
comes from the toolchain's newlib.

## 17.3 libewoksys: the EwokOS System-Interface Layer

This is the part of libc with **the most EwokOS character**
([system/basic/libc/libewoksys/](../../system/basic/libc/libewoksys/)). It
offers two kinds of headers:

### (1) Standard C / POSIX headers — `include/`

Letting you write code as on any UNIX:

```
stdio.h  stdlib.h  string.h  unistd.h  fcntl.h  dirent.h  errno.h
pthread.h  signal.h  semaphore.h  poll.h  termios.h  time.h  sched.h
getopt.h  glob.h  fnmatch.h  libgen.h  ctype.h  math.h  setjmp.h  wchar.h ...
```

Plus the system headers under `include/sys/`:

```
sys/ipc.h  sys/mman.h  sys/shm.h  sys/select.h  sys/stat.h  sys/wait.h
sys/types.h  sys/ioctl.h  sys/time.h  sys/uio.h  sys/resource.h  sys/utsname.h ...
```

### (2) EwokOS-specific APIs — `ewoksys/include/ewoksys/`

These are capabilities unique to the EwokOS microkernel, absent from
standard POSIX:

| Header | Provides |
|--------|----------|
| `syscall.h` | bare syscall wrappers `syscall0~syscall3` (Ch. 08's `svc`) |
| `ipc.h` / `ipc_serv.h` | the IPC client calls and server loop (Ch. 09) |
| `proto.h` | `proto_t` — serialization/deserialization of IPC messages |
| `vfs.h` / `vfsc.h` | the file interface for talking to vfsd |
| `vdevice.h` / `devcmd.h` | the framework for writing "virtual character device" drivers (Ch. 12's driver routine) |
| `proc.h` / `thread.h` | processes (fork/exec) and threads |
| `shm.h` / `shm_pipe.h` / `dma.h` | shared memory, shared-memory pipes, DMA |
| `signal.h` / `semaphore.h` / `interrupt.h` | signals, semaphores, interrupts |
| `klog.h` | logging: `slog` (writes `/dev/log`), `klog` (forces the kernel serial port) |
| `core.h` / `session.h` | interacting with the core service (cored), sessions |
| `mstr.h` / `charbuf.h` / `buffer.h` / `queue.h` / `hashmap.h` | common data structures |
| `md5.h` / `basic_math.h` / `utf8unicode.h` / `kernel_tic.h` | miscellaneous utilities |

Implementations are organized by function into subdirectories under `src/`
(`stdio/`, `stdlib/`, `string/`, `unistd/`, `pthread/`, `signal/`, `sys/`,
etc.), and arch-specific assembly (like `syscall_<arch>`, `setjmp_<arch>`)
lives under `aarch64/`, `arm/`, `x86/`.

## 17.4 softfloat and openlibm: Floating Point and Math

- **softfloat**
  ([system/basic/libc/softfloat/](../../system/basic/libc/softfloat/)):
  a software implementation of IEEE floating-point arithmetic. **Only ARM32
  targets need it** — libc's Makefile compiles it only under `ifeq
  ($(ARCH),arm)`, and `EWOK_LIBC` appends `-lsoftfloat` only on the arm
  platform. AArch64 and x86 have hardware FPUs and don't use it.

- **openlibm**
  ([system/basic/libs/openlibm/](../../system/basic/libs/openlibm/)): a
  standalone, high-quality, portable C math library (`libm`) from
  JuliaMath. Provides `sin/cos/tan`, `exp/log`, `pow/sqrt`, `floor/ceil`,
  etc. It's grouped into the graphics library set `EWOK_LIB_GRAPH` (drawing
  often needs math), but is fundamentally a general-purpose math library.

## 17.5 One Call's Complete Chain

Putting the pieces together, watch what `printf("hi\n")` actually does:

```
your code:  printf("hi\n")
              │
   [newlib]   ▼  formatting + buffering; when it finally must output, calls
           _write(1, "hi\n", 3)
              │
   [libgloss] ▼  the _write stub in syscalls.c forwards to
           vfs_write(...) / the standard output device
              │
  [libewoksys]▼  assembles an IPC message (proto_t) and issues
           ipc_call(...) to the vfsd/console service
              │
   [kernel svc]▼  ipc_call triggers svc #0; the kernel delivers the message
                  to the target service process
              │
      ▼  the console/display service writes the characters to the serial
         port or the screen (chapters 11, 13)
```

`fopen`/`fread` go through vfsd (Ch. 10); `malloc` goes through `_sbrk`
asking the kernel for heap memory (Ch. 05); `fork`/`exec` go through
`_fork`/`_execve` → the kernel's process management (Ch. 07). **Every
standard C function ultimately converges onto the syscall table from Ch.
08.** That is libc's entire secret.

## 17.6 Error Handling: errno

EwokOS follows C's traditional `errno`:

- The global `int errno` is defined in libewoksys (`src/unistd/errno.c`),
  initially `ENONE` (0);
- When a kernel syscall fails it returns a negative value, and libc's
  wrapper functions translate it into a POSIX `errno` (`EAGAIN`, `EBADF`,
  `ENOMEM`…);
- The complete errno value table is in
  [libgloss/errno.h](../../system/basic/libc/libgloss/errno.h); the
  trimmed enum libewoksys uses internally is in `include/sys/errno.h`.

> **Note**: currently `errno` is a single global variable — **there is no
> thread-local-storage (TLS) version**, so be careful in multithreaded
> programs. Also, libc doesn't provide `vprintf`; use `vfprintf(stdout,
> ...)` instead when needed. compat.o defines a global `optind`; if your
> own program also uses a global of the same name, rename it to avoid a
> link conflict.

## 17.7 Summary

- EwokOS's C runtime = **newlib (the standard functions' upper half) +
  libgloss (syscall stubs) + libewoksys (the EwokOS system interface) +
  openlibm (math) + softfloat (ARM32-only floating point)**;
- **libgloss** is the "glue": `syscalls.c` wires newlib onto libewoksys
  with `_read/_write/_sbrk/_fork...`; `compat.c` supplies `mem*`/`str*`
  (word-optimized);
- **libewoksys** is where the EwokOS character lives: standard POSIX
  headers + the `ewoksys/` specific APIs (syscall, ipc, proto, vfs,
  vdevice, proc, thread, shm, klog…);
- Every standard C call ultimately converges onto the kernel's `svc`
  syscall table (Ch. 08);
- Errors are expressed via a global `errno`, with no TLS version.

Next chapter: a grand tour of the "common libraries" above libc —
filesystems, images, fonts, windows, networking, C++ — see what wheels
EwokOS has already prepared for you.
