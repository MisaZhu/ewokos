# 12 Writing Your Own Apps and Commands

> Language: **English** | [中文](12-apps.zh.md)
>
> Goal: write a command-line program with your own hands, add it to the
> build system, pack it into the image, and run it in EwokOS. This is your
> first time contributing to this system as a "developer".

## 12.1 The Runtime Environment of Userland Programs

First, get clear on the differences between "writing a C program on EwokOS"
and "writing a C program on Linux":

| On Linux | On EwokOS |
|---------------|--------------|
| `#include <stdio.h>` uses glibc | uses EwokOS's own C library |
| system calls go through the Linux ABI | go through `svc #0` + the EwokOS syscall table (Ch. 08) |
| compiler = the native compiler | the cross-compiler `aarch64-none-elf-gcc` |

EwokOS's C library has two parts
([system/basic/libc/](../../system/basic/libc/)):

- **`libewoksys`**: the system-interface layer. Syscall wrappers
  (`syscall0~3`), IPC client/server, `proto_t`, fork/exec, file opening,
  and so on;
- **`libgloss`**: the "glue" of the C standard library. `printf`, `malloc`,
  `memcpy`, a `newlib`-style runtime — ultimately wiring the standard
  functions onto `libewoksys`.

The build system links the two into `EWOK_LIBC`; your program just links
against it.

## 12.2 Look at a Ready-Made Example: echo

The directory [system/basic/bin/echo/](../../system/basic/bin/echo/) is as
simple as it gets:

```
echo/
├── echo.c        # the source (just an ordinary C program)
├── Makefile
└── aarch64/      # build artifacts are stored per-architecture
```

Its [Makefile](../../system/basic/bin/echo/Makefile):

```make
SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule   # the compiler and common rules

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)      # the build dir for this arch + board
TARGET_DIR = $(BUILD_DIR)/rootfs                # the final rootfs dir

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

ECHO_OBJS = $(ARCH)/echo.o
ECHO = $(TARGET_DIR)/bin/echo

$(ECHO): $(ECHO_OBJS) $(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(ECHO_OBJS) -o $(ECHO) $(LDFLAGS) $(EWOK_LIBC)
```

Three details worth noting:

1. **`-Ttext=100`**: user programs are loaded at virtual address `0x100`
   (near the very bottom of the low address space), staying well clear of
   the kernel's high addresses (Ch. 05);
2. **`$(EWOK_LIBC)`**: links the complete C library;
3. The artifact is written directly into `rootfs/bin/`, and will later be
   packed into the image by `make sd`.

## 12.3 Follow Along: Write a `hello` Command

### Step 1: the Source

Create `system/basic/bin/hello/hello.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/klog.h>

int main(int argc, char** argv) {
    printf("Hello from my first EwokOS app!\n");
    printf("argc = %d\n", argc);
    for(int i = 0; i < argc; i++)
        printf("  argv[%d] = %s\n", i, argv[i]);

    printf("my pid = %d\n", getpid());
    return 0;
}
```

Behind familiar names like `printf` and `getpid` are exactly the mechanisms
you just learned in chapters 08 and 09.

### Step 2: the Makefile

Create `system/basic/bin/hello/Makefile` (copy echo's and rename):

```make
SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

HELLO_OBJS = $(ARCH)/hello.o
HELLO = $(TARGET_DIR)/bin/hello

$(HELLO): $(HELLO_OBJS) $(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(HELLO_OBJS) -o $(HELLO) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HELLO_OBJS)
```

### Step 3: Register with the Build System

Edit [system/basic/bin/Makefile](../../system/basic/bin/Makefile) and
append `hello` to the end of the line `DIRS = cp ipcserv bgrun ... echo
sleep ...`.

### Step 4: Build and Run

```bash
> cd machines/raspix/system
> make basic      # rebuild only the basic layer (fast). After a first build
                  # or big changes, a full make is recommended
> make sd         # pack the rootfs into the SD image
> make run
```

Once the system is up:

```
> hello a b c
Hello from my first EwokOS app!
argc = 4
  argv[0] = hello
  argv[1] = a
  argv[2] = b
  argv[3] = c
my pid = 42
```

> **Pitfall warning**: running `make` only in a driver or app directory
> does **not** automatically update the image file used for
> flashing/running. After changing code you must go back to
> `machines/raspix/system` and re-run `make sd`, or you'll still be running
> the old program.

## 12.4 Advanced Play

### Calling System Capabilities

Your program can use the entire system interface:

```c
// read a file (through vfsd)
FILE* f = fopen("/etc/passwd", "r");

// get a service and call it over IPC (Ch. 09)
proto_t in, out;
proto_init(&in, ibuf, sizeof(ibuf));
proto_init(&out, obuf, sizeof(obuf));
ipc_call(get_serv_pid("vfs"), VFS_CMD_GET_STATE, &in, &out);

// create a child process
int pid = fork();
if(pid == 0) {
    proc_exec("ls /");
}
```

### Writing Your Own Driver Service

Refer to `nulld` (the simplest device) or `timerd` under
[system/basic/drivers/](../../system/basic/drivers/). The core routine:

```c
// a device driver = an IPC service that provides a /dev/xxx file
void ipc_step(...) {
    switch(cmd) {
    case DEV_CMD_READ:  ...  // someone is reading /dev/xxx
    case DEV_CMD_WRITE: ...  // someone is writing /dev/xxx
    }
}
```

Once written, add a line `@/bin/ipcserv /drivers/hello ...` to `init.rd`;
after reboot, `/dev/xxx` exists. This is the first step toward "writing
drivers" — Ch. 13 will use the same routine for graphics drivers.

## 12.5 Exercises

1. Add a feature to `hello`: with no arguments, print system information
   (the `sysinfo` syscall);
2. Write a `sum` command: read several numbers from standard input and sum
   them (practicing pipes and `scanf`);
3. Write a minimal driver `counterd`: provide `/dev/counter`, returning an
   auto-incrementing count on each read. Refer to `nulld`'s code structure.

## 12.6 Summary

- A user program = ordinary C code + `EWOK_LIBC` (libewoksys + libgloss),
  loaded at virtual address `0x100`;
- Adding a command takes three steps: the source, a Makefile, and
  registering in `bin/Makefile`'s `DIRS`;
- After changes you must `make sd` to repack the image;
- A driver = a userland service process, written in the same way as a
  command.

Next chapter: let the system "grow eyes" — how the graphics system works.
