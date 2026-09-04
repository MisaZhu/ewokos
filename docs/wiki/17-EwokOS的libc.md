# 17 EwokOS 的 libc（C 运行时）

> 本章目标：拆开 EwokOS 的 C 运行时，看清楚 `printf`、`fopen`、`malloc`、`getpid`
> 这些你天天用的名字，背后是怎么一层层落到 EwokOS 内核的 `svc` 系统调用上的。
> 这是"ewokos 系列"的第二篇，承接 [16](16-EwokOS库生态总览.md) 的库生态地图。

第 12 章说过，EwokOS 的 C 库由 `libewoksys` + `libgloss` 组成，链接成 `EWOK_LIBC`。
本章把这句话展开成一张完整的"调用链"图。

## 17.1 libc 的几块拼图

打开 [system/basic/libc/](../../system/basic/libc/)，你会看到：

```
libc/
├── libewoksys/    # EwokOS 系统接口层（最重要）
├── libgloss/      # newlib 风格的 C 库胶水 + 系统调用桩
├── softfloat/     # 软件浮点（仅 ARM32 需要）
└── Makefile
```

再加上基础库里的 [openlibm](../../system/basic/libs/openlibm/)（数学库 `libm`），
以及交叉工具链自带的 **newlib**（提供 `printf`/`malloc` 等标准 C 函数的"上半身"），
共同构成 EwokOS 程序看到的"C 运行时"。它们的分工是：

| 组件 | 来自 | 提供什么 | 链接名 |
|------|------|----------|--------|
| **newlib** | 交叉工具链自带 | 标准 C 函数：`printf`、`scanf`、`malloc`、`fopen` 的"格式化/缓冲"逻辑 | `-lc`（工具链） |
| **libgloss** | EwokOS 自己实现 | newlib 需要的**系统调用桩**：`_read`、`_write`、`_sbrk`、`_open`……以及 `mem*`/`str*` | `-lgloss`、`-lc`（EwokOS） |
| **libewoksys** | EwokOS 自己实现 | **EwokOS 系统接口**：syscall 封装、IPC、VFS、进程/线程、proto 序列化、日志 | `-lewoksys` |
| **openlibm** | vendored 第三方 | 高质量数学函数：`sin`、`cos`、`sqrt`、`pow`…… | `-lopenlibm` |
| **softfloat** | vendored 第三方 | 软件浮点运算（仅 ARM32） | `-lsoftfloat` |

> 关键认知：newlib 的 `printf` 只负责"把格式串拼成字符串"，
> 真正"把字符串送出去"这一步，它调用一个名为 `_write` 的桩函数。
> **这个桩由谁来填，决定了 libc 跑在哪个操作系统上。** EwokOS 用 libgloss 填这个桩。

## 17.2 libgloss：把 newlib 接到 EwokOS 上

libgloss 目录（[system/basic/libc/libgloss/](../../system/basic/libc/libgloss/)）只有两个源文件：

- **`syscalls.c`**——newlib 要求的"系统命名空间"桩函数，全部以 `_` 开头：

  ```c
  int _read (int fd, void *buf, size_t size);     // → 走 libewoksys 的 VFS 读
  int _write(int fd, const void *buf, size_t size);// → 走 libewoksys 的 VFS 写
  int _open (const char *fname, int oflag, ...);
  int _close(int fd);
  _off_t _lseek(int fd, _off_t offset, int whence);
  int _fstat(int fd, struct stat *st);
  int _stat (const char *fname, struct stat *st);
  int _unlink(const char *path);
  int _isatty(int fd);
  pid_t _getpid(void);
  void *_sbrk(ptrdiff_t incr);                     // malloc 的内存来源
  int _fork(void);  int _wait(int *status);
  int _execve(const char *name, char *const argv[], char *const env[]);
  void _exit(int err);
  int _gettimeofday(struct timeval *tp, void *tzvp);
  clock_t _times(struct tms *tp);
  ```

  文件开头 `#include <ewoksys/vfs.h>`、`<ewoksys/core.h>`、`<ewoksys/kernel_tic.h>`——
  每个桩的实现，都是转调 libewoksys 的对应函数。这就是"胶水"的含义。

- **`compat.c`**——补齐工具链没提供、或被 `-fno-builtin-*` 关掉的常用函数：
  `memcpy`、`memmove`、`memset`、`memcmp`、`strcpy`、`strcmp`、`strlen`、`strchr`……
  以及 `ewok_longjmp`。

  > **性能要点**：EwokOS 的 make.rule 对 `mem*` 强制 `-fno-builtin`，
  > 所以所有 `memcpy`/`memset` 都落到 compat.c 里。这里的实现是**按字（word）搬运**
  > 而非逐字节，对屏幕合成、IPC 缓冲、共享内存拷贝这类大流量路径至关重要。

libgloss 的 Makefile 把这两个 `.o` 同时打包成 `libgloss.a` **和** `libc.a`，
所以 `EWOK_LIBC` 里的 `-lc -lgloss` 找到的都是 EwokOS 自己这份胶水，
而标准 C 函数的"上半身"仍来自工具链的 newlib。

## 17.3 libewoksys：EwokOS 系统接口层

这是 libc 里**最 EwokOS 特色**的部分
（[system/basic/libc/libewoksys/](../../system/basic/libc/libewoksys/)）。
它对外提供两类头文件：

### （1）标准 C / POSIX 头——`include/`

让你能像在任何 UNIX 上一样写代码：

```
stdio.h  stdlib.h  string.h  unistd.h  fcntl.h  dirent.h  errno.h
pthread.h  signal.h  semaphore.h  poll.h  termios.h  time.h  sched.h
getopt.h  glob.h  fnmatch.h  libgen.h  ctype.h  math.h  setjmp.h  wchar.h ...
```

以及 `include/sys/` 下的系统头：

```
sys/ipc.h  sys/mman.h  sys/shm.h  sys/select.h  sys/stat.h  sys/wait.h
sys/types.h  sys/ioctl.h  sys/time.h  sys/uio.h  sys/resource.h  sys/utsname.h ...
```

### （2）EwokOS 专有 API——`ewoksys/include/ewoksys/`

这些是 EwokOS 微内核独有的能力，标准 POSIX 里没有：

| 头文件 | 提供什么 |
|--------|----------|
| `syscall.h` | 裸系统调用封装 `syscall0~syscall3`（第 08 章的 `svc`） |
| `ipc.h` / `ipc_serv.h` | IPC 客户端调用与服务端循环（第 09 章） |
| `proto.h` | `proto_t`——IPC 消息的序列化/反序列化 |
| `vfs.h` / `vfsc.h` | 访问 vfsd 的文件接口 |
| `vdevice.h` / `devcmd.h` | 写"虚拟字符设备"驱动的框架（第 12 章的驱动套路） |
| `proc.h` / `thread.h` | 进程（fork/exec）与线程 |
| `shm.h` / `shm_pipe.h` / `dma.h` | 共享内存、共享内存管道、DMA |
| `signal.h` / `semaphore.h` / `interrupt.h` | 信号、信号量、中断 |
| `klog.h` | 日志：`slog`（写 `/dev/log`）、`klog`（强制走内核串口） |
| `core.h` / `session.h` | 与核心服务（cored）交互、会话 |
| `mstr.h` / `charbuf.h` / `buffer.h` / `queue.h` / `hashmap.h` | 常用数据结构 |
| `md5.h` / `basic_math.h` / `utf8unicode.h` / `kernel_tic.h` | 杂项工具 |

实现按功能分子目录放在 `src/`（`stdio/`、`stdlib/`、`string/`、`unistd/`、
`pthread/`、`signal/`、`sys/` 等），架构相关的汇编（如 `syscall_<arch>`、`setjmp_<arch>`）
放在 `aarch64/`、`arm/`、`x86/` 下。

## 17.4 softfloat 与 openlibm：浮点与数学

- **softfloat**（[system/basic/libc/softfloat/](../../system/basic/libc/softfloat/)）：
  软件实现的 IEEE 浮点运算。**只有 ARM32 目标需要**——libc 的 Makefile 里
  `ifeq ($(ARCH),arm)` 才编译它，`EWOK_LIBC` 也只在 arm 平台追加 `-lsoftfloat`。
  AArch64 和 x86 有硬件浮点单元，用不上。

- **openlibm**（[system/basic/libs/openlibm/](../../system/basic/libs/openlibm/)）：
  来自 JuliaMath 的独立、高质量、可移植的 C 数学库（`libm`）。
  提供 `sin/cos/tan`、`exp/log`、`pow/sqrt`、`floor/ceil` 等。
  它被归在图形库组 `EWOK_LIB_GRAPH` 里（画图常需要数学），但本质是通用数学库。

## 17.5 一次调用的完整链路

把上面几块拼起来，看 `printf("hi\n")` 到底发生了什么：

```
你的代码:  printf("hi\n")
              │
   [newlib]   ▼  格式化 + 缓冲，最终要输出时调用
           _write(1, "hi\n", 3)
              │
   [libgloss] ▼  syscalls.c 里的 _write 桩，转调
           vfs_write(...) / 标准输出设备
              │
  [libewoksys]▼  组织 IPC 消息(proto_t)，向 vfsd/console 服务发起
           ipc_call(...)
              │
   [内核 svc] ▼  ipc_call 触发 svc #0，内核把消息投递给目标服务进程
              │
      ▼  console/显示服务把字符写到串口或屏幕（第 11、13 章）
```

`fopen`/`fread` 走 vfsd（第 10 章），`malloc` 走 `_sbrk` 向内核要堆内存（第 05 章），
`fork`/`exec` 走 `_fork`/`_execve` → 内核进程管理（第 07 章）。
**每一个标准 C 函数，最后都收敛到第 08 章那张系统调用表上。** 这就是 libc 的全部秘密。

## 17.6 错误处理：errno

EwokOS 沿用 C 传统的 `errno`：

- 全局 `int errno` 定义在 libewoksys（`src/unistd/errno.c`），初值 `ENONE`（0）；
- 内核系统调用失败时返回负值，libc 的封装函数把它翻译成 POSIX `errno`
  （`EAGAIN`、`EBADF`、`ENOMEM`……）；
- 完整的 errno 数值表在 [libgloss/errno.h](../../system/basic/libc/libgloss/errno.h)，
  libewoksys 自己用的精简枚举在 `include/sys/errno.h`。

> **注意**：当前 `errno` 是单一全局变量，**没有线程本地存储（TLS）版本**，
> 多线程程序里要留意。另外 libc 不提供 `vprintf`，需要时用 `vfprintf(stdout, ...)` 代替；
> compat.o 里定义了全局 `optind`，自己的程序若也用了同名全局变量需改名以免链接冲突。

## 17.7 本章小结

- EwokOS 的 C 运行时 = **newlib（标准函数上半身）+ libgloss（系统调用桩）+
  libewoksys（EwokOS 系统接口）+ openlibm（数学）+ softfloat（仅 ARM32 浮点）**；
- **libgloss** 是"胶水"：`syscalls.c` 用 `_read/_write/_sbrk/_fork...` 把 newlib 接到
  libewoksys；`compat.c` 补 `mem*`/`str*`（按字优化）；
- **libewoksys** 是 EwokOS 特色所在：标准 POSIX 头 + `ewoksys/` 专有 API
  （syscall、ipc、proto、vfs、vdevice、proc、thread、shm、klog……）；
- 任何标准 C 调用，最终都收敛到内核的 `svc` 系统调用表（第 08 章）；
- 错误用全局 `errno` 表达，无 TLS 版本。

下一章：libc 之上的"常用库"大盘点——文件系统、图像、字体、窗口、网络、C++，
看看 EwokOS 都为你准备好了哪些轮子。
