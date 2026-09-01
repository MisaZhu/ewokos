# 09 IPC 进程间通信

> 本章目标：理解微内核的血液——IPC。看懂一次 `open("/a.txt")`
> 背后客户端、内核、服务端三方的完整协作。这是理解 EwokOS 一切
> 上层设施（文件系统、窗口系统、网络）的钥匙。
> 更深入的细节仓库里有专门文档：[docs/ipc.md](../ipc.md)。

## 9.1 微内核为什么必须解决通信问题

第 01 章说过：文件系统、驱动、窗口系统都是独立的用户态进程。
它们之间必须互相调用，比如：

- `shell` 要调用 `vfsd` 读文件；
- `vfsd` 要调用 `sdfsd` 读 SD 卡；
- 窗口程序要调用 `xserverd` 画图。

内核不提供端口、管道、消息队列——它自己**就是**消息中介。
EwokOS 的 IPC 是**同步远程过程调用（RPC）**：

```
┌─────────────────────┐                        ┌──────────────────────────────┐
│ 客户端进程           │                        │ 服务端进程                    │
│                     │  SYS_IPC_CALL          │                              │
│ ipc_call(pid, cmd,  │ ─────────────────────► │ ipc 入口函数                 │
│        in, out)     │     内核分配 ipc 任务    │   ipc_get_arg(uid)           │
│                     │                        │   ... 处理 cmd ...           │
│ 阻塞，等待回复        │                        │   ipc_set_return(uid, out)   │
│                     │ ◄───────────────────── │   ipc_end()                  │
│ ipc_get_return()    │  内核唤醒客户端           │                              │
└─────────────────────┘                        └──────────────────────────────┘
```

一次调用 = 客户端阻塞 + 服务端执行 + 客户端被唤醒。简单、可预测，
像本地函数调用一样用。

## 9.2 服务端：如何成为一个服务

任何进程调用 `ipc_serv_run()` 主循环就能变成服务器
（[ipc_serv.c](../../system/basic/libc/libewoksys/ewoksys/src/ipc_serv.c)）。
框架大致是：

```c
// 伪代码：一个服务进程的主循环
void ipc_serv_run(ipc_serv_t* serv, void(*handler)(ipc_serv_t*, int32_t uid)) {
    while(1) {
        uid = ipc_serv_wait();       // 睡眠，直到有请求到来
        handler(serv, uid);          // 处理请求
        ipc_end(uid);                // 通知内核：这个请求结束了
    }
}
```

**请求是怎么"到来"的？** 回忆第 07 章的上下文劫持：
客户端发起调用后，内核把服务端进程标记为待处理，
服务端下次被调度时直接从注册的 **ipc 入口函数** 开始执行，
参数（任务号 `uid`）通过寄存器传入。处理完 `ipc_end()`，
内核恢复它原来的上下文，继续主循环——它根本感知不到切换发生过。

### 单任务模式与多任务模式

- **单任务模式（默认）**：整个进程同时只处理一个请求，
  处理期间其他客户端的请求排队（内核让调用方进入等待，
  返回 `IPC_ERROR_RETRY` 语义）。简单，但慢请求会阻塞所有人；
- **多任务模式（`IPC_MULTI_TASK`）**：内核为服务维护一个
  **工作线程池**，多个请求被分配到不同线程并行处理。
  文件系统、窗口服务器这类高并发服务用它。

## 9.3 客户端：如何调用一个服务

客户端用库函数 `ipc_call`（[ipc.c](../../system/basic/libc/libewoksys/ewoksys/src/ipc.c)）：

```c
proto_t in, out;
proto_init(&in, buf_in, sizeof(buf_in));
proto_init(&out, buf_out, sizeof(buf_out));

proto_add_int(&in, 参数1);              // 打包参数
int res = ipc_call(服务进程pid, 命令号, &in, &out);  // ★同步等待结果
if(res == 0) {
    int value = proto_read_int(&out);   // 解包结果
}
```

数据打包用 `proto_t`（[proto.c](../../system/basic/libc/libewoksys/ewoksys/src/proto.c)）：
一个带长度前缀的字节包，支持整数、字符串、二进制块。
为什么不用结构体直接传？因为两个进程的内存空间互相不可见，
必须先把数据**拷贝到内核中转区**再复制到对方空间——
打包成字节流最简单可靠。

> 大数据怎么办？图形系统一次传几百 KB 的位图，来回拷贝太亏。
> 这时用**共享内存**（`SYS_PROC_SHM_*`）：双方映射同一块物理内存，
> IPC 只传一个"指针"。第 13 章会用到。

## 9.4 内核侧：ipc.c 在做什么

内核的 [ipc.c](../../kernel/kernel/src/ipc.c) 维护：

1. **ipc 任务槽（ipc_task_t）**：每个进行中的调用占一个槽，
   记录客户端是谁、参数在哪、服务端分配的处理线程等；
2. **等待队列**：服务端忙时，客户端排队；
3. **唤醒逻辑**：服务端 `ipc_end()` → 内核把结果数据复制到
   客户端缓冲区 → 唤醒客户端进程。

整条链路（以客户端 A 调用服务端 B 为例）：

```
A: SYS_IPC_CALL(B的pid, cmd, 参数)
   └► 内核：分配任务槽，复制参数；若 B 空闲：
      标记 B 的 ipc_server.do_switch，唤醒 B
      A 进入 WAIT（阻塞）
B: 被调度 → 上下文劫持 → 执行 ipc 入口函数
   └► 处理完，SYS_IPC_SET_RETURN + SYS_IPC_END
      └► 内核：复制结果给 A，唤醒 A，还原 B 的原始上下文
A: 从阻塞处继续，拿到结果
```

错误码（[syscalls.h](../../kernel/kernel/include/syscalls.h)）：

```c
#define IPC_ERROR_RETRY      -1   // 服务端忙，内核会让调用方等待重试
#define IPC_ERROR_SELF       -2   // 不能调用自己（会死锁）
#define IPC_ERROR_NO_READY   -3   // 对方还没注册 ipc 服务
```

## 9.5 如何找到服务：名字服务（core）

调用需要对方进程的 pid，但"文件系统服务的 pid 是多少？"
每次开机可能不同。EwokOS 用 `core` 进程做**名字服务**：

```
服务启动时：  ipc_serv_reg("vfs", 服务id)  → core 记下 "vfs → pid 3"
客户端调用：  get_serv_pid("vfs")          → 向 core 查询 → 拿到 pid 3
```

`core` 还负责系统级协调（如当前目录、主机名），所以它必须是
**第二个启动的进程**——这正是第 11 章 `init` 先启动 `/sbin/core` 的原因。

## 9.6 用 IPC 的视角重看"读文件"

现在你能完整解释第 01 章那张图了：

```
应用: open("/etc/passwd")
  → libc: ipc_call(vfsd_pid, VFS_CMD_OPEN, ...)      【IPC 第 1 跳】
  → vfsd: 查挂载表，发现 "/" 由 sdfsd 提供
  → vfsd: ipc_call(sdfsd_pid, FS_CMD_OPEN, ...)      【IPC 第 2 跳】
  → sdfsd: 通过 SYS_MEM_MAP 访问 SD 卡控制器读扇区，
           解析 ext2 目录，找到 inode
  → 结果原路返回，应用拿到一个文件句柄
```

**内核全程只搬运消息。** 文件系统代码哪怕全是 bug，崩溃的也只是 `sdfsd` 一个进程。

## 9.7 动手练习

1. 启动系统后运行 `svcinfo`（或 `ps` 对照启动日志），
   找出系统中所有的服务进程（`uartd`、`vfsd`、`timerd`…）；
2. 在 [core.c](../../system/basic/sys/core/core.c) 中找到服务注册表，
   看看 `vfs`、`sd` 这些名字如何映射到 pid；
3. 思考题：客户端调用服务端时如果服务端进程崩溃了，会发生什么？
   （提示：内核会唤醒等待的客户端并返回错误——在 `ipc.c` 里找找这段逻辑。）

## 9.8 本章小结

- EwokOS IPC 是同步 RPC：客户端阻塞、内核中转、服务端执行、结果唤醒；
- 服务端靠"上下文劫持"被内核叫醒，单任务模式串行，`IPC_MULTI_TASK` 线程池并行；
- 参数用 `proto_t` 字节包中转，大数据走共享内存；
- `core` 进程提供名字服务，让服务发现与 pid 解耦；
- 文件系统、窗口、网络全建立在它之上。

下一章：沿着 `sdfsd` 的路径往下看——SD 卡与文件系统是怎么实现的。
