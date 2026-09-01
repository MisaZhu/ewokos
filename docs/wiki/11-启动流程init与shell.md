# 11 启动流程：init 与 shell

> 本章目标：从内核加载第一个进程开始，完整看懂用户态是如何
> 一层层建立起来的，理解 `.rd` 启动脚本，直到你面前出现命令行提示符。

## 11.1 第一个用户进程：/sbin/init

回顾第 10 章：内核启动末尾调用 `load_init_proc()`，
直接从 SD 卡读取 `/sbin/init` 的 ELF 并创建进程。这个进程有几个特殊性：

- **pid = 0**（EwokOS 的第一个进程编号从 0 开始）；
- **uid = -1**（特殊标记：内核加载的，拥有最高信任）；
- 它是所有进程的祖先，负责收留孤儿进程（第 07 章）。

看 [init.c](../../system/basic/sys/init/init.c) 的 `main`：

```c
int main(int argc, char** argv) {
    if(((int16_t)getuid()) >= 0) {   // 不是内核加载的？拒绝运行
        klog("process 'init' can only loaded by kernel!\n");
        return -1;
    }
    if(getpid() != 0)                // 不是 0 号？说明是核心空闲进程
        idle();

    syscall1(SYS_PROC_SET_CMD, (ewokos_addr_t)"/sbin/init");

    klog("\n[init process started]\n");
    run_before_vfs("/sbin/core");    // ① 名字服务 + 系统协调
    run_before_vfs("/sbin/vfsd");    // ② 虚拟文件系统
    run_before_vfs("/sbin/sdfsd");   // ③ SD 卡 ext2 文件系统

    switch_root();                   // ④ 执行启动脚本
    while(true)
        usleep(100000);              // init 永不退出
}
```

注意 ①②③ 的执行时机：此时 **VFS 还不存在**，文件系统也还没挂载！
所以这三个进程不能用"从文件加载"的方式启动，而是用 `run_before_vfs`：

```c
static void run_before_vfs(const char* cmd) {
    int pid = fork();
    if(pid == 0) {
        exec_from_sd(cmd);   // 直接用内核态 SD 驱动读 ext2 拿 ELF，
                             // 再调 SYS_EXEC_ELF 加载
    }
    else
        ipc_wait_ready(pid); // 父进程等服务真正就绪
}
```

这是典型的"鸡生蛋"解法：文件系统服务自己得用非常规手段先启动。

## 11.2 启动脚本：/etc/init.rd

三个核心服务就位后，文件系统可用了，`init` 转入正常流程——
按顺序执行 `/etc/init0.rd` ~ `/etc/init7.rd`（存在才执行），
最后执行 `/etc/init.rd`：

```c
static void switch_root(void) {
    char initfile[32];
    uint8_t i = 0;
    while(i < 8) {
        snprintf(initfile, 31, "/etc/init%d.rd", i);
        if(run_init(initfile) != 0)
            break;
        i++;
    }
    run_init("/etc/init.rd");
}
// run_init 内部:  fork() + proc_exec("/bin/shell <initfile>")
```

也就是说：**启动脚本是由 shell 解释执行的**。

### `.rd` 脚本语法

以树莓派的基础版启动脚本
[etc/basic/init.rd](../../machines/raspix/system/etc/basic/init.rd) 为例：

```sh
@/bin/ipcserv /drivers/raspix/uartd  /dev/tty0   # 串口驱动（终端）
@/bin/ipcserv /drivers/raspix/cpud   /dev/cpu    # CPU 信息驱动
@set_stdio /dev/tty0                              # 标准输入输出接到串口

@/bin/ipcserv /drivers/timerd                    # 定时器设备
@/bin/ipcserv /drivers/piped      /dev/pipe0     # 管道
@/bin/ipcserv /drivers/ramfsd     /tmp           # 内存文件系统
@/bin/ipcserv /drivers/nulld      /dev/null

@/bin/ipcserv /sbin/sessiond                     # 会话管理
@/bin/bgrun /bin/session -r -t /dev/tty0         # 启动终端会话 → 出现提示符！
```

`@` 开头是 shell 的特殊指令：

| 指令 | 作用 |
|------|------|
| `@/bin/ipcserv <程序> [挂载点]` | 以后台服务方式启动程序（可注册到 core） |
| `@/bin/bgrun <命令>` | 后台运行 |
| `@set_stdio <设备>` | 切换标准输入输出设备 |
| `@export VAR=值` | 设置环境变量（如 `TZ` 时区、`XTHEME` 主题） |
| `@echo ...` | 打印 |

完整版系统（带图形）的脚本在
[etc/xwin/init.rd](../../machines/raspix/system/etc/xwin/init.rd)，
里面会依次启动显示驱动、字体、开机画面（splash）、X 服务器……
每条 `@/bin/splash -m "..." -p xx` 就是开机进度条更新一次。

> 配置文件体系小结：内核配置是 `/etc/kernel/kernel.conf`（简单键值对），
> 各服务用 JSON 配置（如 `/etc/console.json`），启动编排用 `.rd` 脚本。
> 三者都是"谁用谁读"，没有集中的配置管理器。

## 11.3 会话与 shell：提示符是怎么出现的

启动脚本最后一行 `/bin/session -r -t /dev/tty0` 启动了一个终端会话：

```
session ──► 打开 /dev/tty0（串口设备文件，背后是 uartd 进程）
    │
    └──► fork + exec /bin/shell
              │
              └──► 死循环：读一行 → 解析 → fork+exec 执行命令 → 等待 → 打印提示符
```

`shell`（[system/basic/bin/shell/](../../system/basic/bin/shell/)）
就是你在提示符后面交互的程序。它做的事情每个 Unix 老手都熟悉：

1. 从标准输入读一行；
2. 解析命令与参数（支持 `|` 管道、`>` 重定向、`&` 后台）；
3. `fork()` 子进程，`exec()` 目标程序；
4. 没有 `&` 就 `waitpid` 等子进程结束；
5. 打印下一个提示符。

其中管道用 `/dev/pipe0`（piped 进程）实现，重定向就是
`open` 一个文件然后替换标准输入输出——全部建立在第 09、10 章的机制上。

## 11.4 完整启动时序图

把本章与前面章节串起来：

```
内核启动完成
  │
  ├─ load_init_proc(): 从 SD 卡读 /sbin/init，创建进程0
  ▼
init(pid 0):
  ├─ exec_from_sd /sbin/core      （名字服务）
  ├─ exec_from_sd /sbin/vfsd      （VFS 挂载表建立）
  ├─ exec_from_sd /sbin/sdfsd     （挂载 /，SD 卡文件系统）
  ├─ shell 执行 /etc/init0.rd ... /etc/init.rd
  │     ├─ uartd      → /dev/tty0
  │     ├─ timerd     → /dev/timer
  │     ├─ ramfsd     → /tmp
  │     ├─ logd       → /dev/log
  │     ├─ （图形目标还有）displayd / fontd / xserverd ...
  │     └─ session → shell
  ▼
提示符出现，等待你的命令
```

## 11.5 跟着做：改造你的启动脚本

一个很有成就感的实验——修改启动流程：

```bash
> cd machines/raspix/system/etc/basic
> 编辑 init.rd，加一行：
  @echo Hello from my init script!
> cd machines/raspix/system
> make && make sd          # 重新构建并打包镜像
> make run
```

你将看到自己写的文字出现在启动日志中。
这就是"配置系统启动行为"的全部过程——不需要改一行 C 代码。

## 11.6 动手练习

1. 启动后运行 `ps`，按启动顺序核对每个进程与 11.4 的时序图；
2. 修改 `init.rd`：把 `timerd` 注释掉，观察系统会发生什么
   （提示：`sleep`、`ps` 的时间统计会受影响）；
3. 阅读 [shell 源码目录](../../system/basic/bin/shell/)，
   找到"解析管道符 `|`"的代码段。

## 11.7 本章小结

- `init`（pid 0）是唯一的"内核直接加载"进程，之后它负责拉起一切；
- VFS 就绪前，核心服务用 `exec_from_sd` 直接从 SD 卡加载；
- 启动编排由 `/etc/init*.rd` 脚本完成，经 shell 解释，`@` 前缀是指令；
- `session + shell` 把串口变成你面前的命令行。

下一章：不再只是启动别人的程序——写一个属于你自己的命令。
