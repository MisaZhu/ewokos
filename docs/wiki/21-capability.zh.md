# 21 Capability 权限机制

> 语言: [English](21-capability.md) | **中文**
>
> 本章目标：理解 EwokOS 的 capability（能力）权限模型——进程凭什么能映射
> 设备内存、注册中断、给别人发信号？看懂 cnode 能力表、内核检查点、
> mint/grant 委派机制，以及 `/etc/cap.json` 声明式策略如何让"最小权限"
> 落地到每一个驱动。
> 对应源码：[cap.h](../../kernel/kernel/include/cap.h)、
> [cap.c](../../kernel/kernel/src/cap.c)、
> [cap_policy.c](../../system/basic/sys/init/cap_policy.c)。
> 本章术语（capability/cnode/mint/grant…）见 [附录·术语表](99-glossary.zh.md)。

## 21.1 为什么需要 capability：从 root 通吃说起

第 08 章的思考题问过：`SYS_MEM_MAP`（映射设备内存）只应给受信任的驱动进程，
内核该怎么区分？当时的答案是看 `proc->info.uid`——uid 为 0（root）就放行。

这个答案能工作，但太粗糙了：

- **一根筋的开关**：uid 只有"是不是 root"两个状态。串口驱动和磁盘驱动
  都是 root，前者出了漏洞照样能读写磁盘控制器的寄存器；
- **微内核尤其需要细分**：第 01 章说过，EwokOS 把驱动、文件系统全推到
  用户态。系统里一大半进程都在做"原来内核干的事"，如果它们全是 root，
  微内核换来的隔离性就名存实亡；
- **出错范围要可控**：最小权限原则（Principle of Least Privilege）——
  每个组件只拿完成工作所必需的权限，被攻破时也最多搞砸自己那一摊。

capability（能力）就是解法：**不问"你是谁"，只问"你手里有什么票"**。
每张票精确描述"能对哪个对象做什么操作"，内核在每次特权操作时验票。

## 21.2 capability 是什么：对象 + 权限的票据

一张 capability 是一张三元组票据：

```
┌─────────────────────────────────────────┐
│  type    对象类型（设备内存？中断？进程？）  │
│  rights  权限位（可读/可写/可执行/可转发）  │
│  object  对象标识（按值存：地址、中断号…）  │
└─────────────────────────────────────────┘
```

每个进程持有一张 **cnode**（capability node，能力表），
最多 64 槽（`CNODE_SLOTS = 64`），每槽一张票。
进程想做特权操作时，内核翻它的 cnode：有没有一张票，
**类型对得上、对象盖得住、权限位齐全**？有就放行，没有就拒绝。

与传统 Unix 的对比：

| | Unix uid 模型 | capability 模型 |
|---|---|---|
| 判断依据 | 你是谁（uid==0?） | 你持有什么票 |
| 粒度 | root = 全部权限 | 每张票一个对象一类操作 |
| 传递 | setuid 位（整体变身） | 逐票转发，且只能越传越弱 |
| 收回 | 很难 | 撕票即可 |

## 21.3 EwokOS 的 cap 类型与权限位

[cap.h](../../kernel/kernel/include/cap.h) 里定义了 7 种票据类型：

| 类型 | 守护的对象 | 典型持有者 |
|---|---|---|
| `CAP_FRAME` | 一段物理地址区间（RAM / MMIO） | 驱动（映射设备寄存器） |
| `CAP_IRQ` | 一个中断号 | 驱动、timerd |
| `CAP_DMA` | 一个 DMA 内存块 | 用 `dma_user_alloc()` 的驱动 |
| `CAP_EP` | 一个 IPC 端点（服务进程） | 服务的客户端 |
| `CAP_PROCESS` | 一个目标进程 | 需要跨用户发信号/调优先级的进程 |
| `CAP_AS` | 一个地址空间 | （保留，地址空间级操作） |
| `CAP_ROOT` | 系统根权限 | 内核进程、login（见 21.9 节） |

权限位只有 4 个，语义随类型而定：

```c
#define CAP_R       (1U << 0)   /* 读     */
#define CAP_W       (1U << 1)   /* 写     */
#define CAP_X       (1U << 2)   /* 执行   */
#define CAP_GRANT   (1U << 3)   /* 可转发 */
```

- frame 的 `W`：允许把这段物理地址 `SYS_MEM_MAP` 进自己的地址空间；
- irq 的 `W`：允许注册这个中断号；`X`：允许发送软中断；
- ep 的 `X`：允许向这个服务发起 IPC 调用；
- `CAP_GRANT`：允许把这张票转发给别的进程（21.6 节）。

## 21.4 内核检查点：把守哪些系统调用

验票发生在系统调用入口（[svc.c](../../kernel/kernel/src/svc.c)）。
主要检查点：

| 系统调用 | 需要的票 | 检查方式 |
|---|---|---|
| `SYS_MEM_MAP`（mmio_map） | `CAP_FRAME` + W | 目标区间必须被某张 frame 票完整覆盖 |
| `SYS_INT_SETUP`（注册中断） | `CAP_IRQ` + W | 中断号逐号匹配 |
| `SYS_SOFT_INT`（发软中断） | `CAP_IRQ` + X | 有任意 irq 票的 X 位即可 |
| `SYS_DMA_*`（DMA 申请/映射） | `CAP_DMA` + W | 块号匹配 |
| `SYS_SIGNAL` / 调优先级 / 跨用户唤醒 | `CAP_PROCESS` + W | 按目标 pid 匹配 |
| `SYS_PROC_SET_UID` / `SET_GID` | `CAP_ROOT` | 切换用户身份的唯一通道 |
| `SYS_CORE_PROC_READY` | `CAP_ROOT` | 只有 core 进程该调 |
| IPC 调用（服务开启 `IPC_CAP_CHECK` 时） | `CAP_EP` + X | 按服务 pid 匹配（[ipc.c](../../kernel/kernel/src/ipc.c)） |

以 `SYS_MEM_MAP` 为例，检查简化后就是"在 cnode 里找一张盖得住的票"：

```c
/* 进程想映射 [paddr, paddr+size)：遍历 cnode，
   找 type==CAP_FRAME、rights 含 W、且区间完整覆盖目标的一张票 */
proc_cap_check_frame(proc, paddr, size, CAP_W);
```

注意一个安全细节：**对象按值存储、检查时重新校验**。
比如 `CAP_PROCESS` 票里存的是"目标 pid + 目标当时的 uuid"
（pid 会复用、uuid 不会，见第 07 章）。目标进程退出、pid 被回收后，
旧票自然失效——不会出现"票指向的新进程根本不是原来那个"的问题。

## 21.5 权限的生命周期：创建 / fork / exec / setuid

票据从哪来？内核给每个进程定了一条清晰的生命周期规则
（[proc.c](../../kernel/kernel/src/proc.c) + cap.c）：

```
内核创建的进程（core/vfsd 等，无父进程）
        │  直接授予 CAP_ROOT —— 它们是系统的"创世者"
        ▼
fork    │  原样复制父进程的 cnode —— 子进程继承父亲的所有票
        ▼
exec    │  重置！新程序不该继承旧程序的票：
        │    · 镜像是系统级（uid<=0）→ 重新授予 CAP_ROOT
        │    · 普通用户程序（uid>0） → cnode 清空
        │  然后查策略表（21.7 节），把声明好的票放进新 cnode
        ▼
setuid  │  变成普通用户（uid>0）→ 撕掉所有票
```

exec 重置是整条链里最关键的一步：它保证**权限跟着"程序身份"走，
而不是跟着"进程历史"走**。shell 是 root，不代表它启动的每个程序
都该是 root——程序一 exec，就从零开始，只拿策略声明过的票。

## 21.6 委派：mint / grant / revoke

票还可以在进程间流动，三个系统调用（用户态封装在
[cap.c](../../system/basic/libc/libewoksys/ewoksys/src/cap.c)）：

```c
int32_t cap_mint(type, rights, a, b);          /* 铸造一张对象票（仅 CAP_ROOT） */
int32_t cap_grant(target_pid, src_slot, mask); /* 把我的票抄一份给别人 */
int32_t cap_revoke(slot);                      /* 撕掉自己的一张票 */
```

两条硬规则：

1. **铸造权集中**：`cap_mint` 只有 CAP_ROOT 持有者能调。
   对象票（frame/irq/dma…）不能凭空变出来，必须由根 authority 铸造；
2. **转发只能衰减**：`cap_grant` 给出的权限位是"原票 ∩ mask"——
   永远不能越给越多。而且给出的是**拷贝**：你撕自己的票，
   不影响已经给出去的那张。

这套机制叫**委派（delegation）**：根 authority 不必事必躬亲，
可以把"一小段设备内存的访问权"精确地、安全地递到需要它的进程手里。

## 21.7 /etc/cap.json：声明式权限策略

到这里机制齐了，还差**策略**：每个驱动该拿哪些票，写在哪、谁来发？

两个现实约束决定了设计：

- exec 会重置 cnode → 发票必须发生在 exec **之后**；
- 驱动由 init 脚本里的 shell 拉起，启动顺序不可控 →
  不能靠"扫描已在运行的进程"来发票（扫的时候人家可能还没启动）。

EwokOS 的答案：**把策略灌进内核，让内核在每次 exec 时自己发票**。

```
/sbin/init 启动早期:
    读 /etc/cap.json（tinyjson 解析，见 cap_policy.c）
    逐条调用 SYS_CAP_POLICY_ADD 灌进内核策略表
        （32 条规则 × 每条 16 个 cap）
        │
此后每一次 exec（无论何时、无论第几次重启该进程）:
    内核重置 cnode 后，用新程序的 cmd 首词去匹配策略表，
    命中就把声明的 cap 直接构建进新 cnode —— 发票完成
```

策略文件长这样（[machine.virt 的真实配置](../../machine.virt/system/etc/basic/cap.json)，节选）：

```json
{
    "rules": [
        { "cmd": "/drivers/virt/ttyd", "desc": "串口驱动：PL011 一页 + irq33", "caps": [
            {"type":"frame", "paddr":"0x09000000", "size":"0x1000", "rights":"rw"},
            {"type":"irq", "irq":33, "rights":"w"}
        ]},

        { "cmd": "/drivers/virt/timerd", "desc": "软中断服务", "caps": [
            {"type":"irq", "irq":0, "rights":"x"}
        ]},

        { "cmd": "/sbin/sdfsd", "desc": "SD 卡文件系统", "caps": [
            {"type":"frame", "paddr":"0x0a000000", "size":"0x4000", "rights":"rw"},
            {"type":"irq", "irq":[48,49,50,51,52,53,54,55], "rights":"w"},
            {"type":"dma", "block":0, "rights":"rw"}
        ]},

        { "cmd": "/bin/login", "desc": "setuid 信任边界", "caps": [
            {"type":"root", "rights":"rwxg"}
        ]}
    ]
}
```

要点：

- `cmd` 与进程 `procinfo.cmd` 的**第一个空白分隔词**匹配（即可执行文件路径）；
- `rights` 字母对应 R/W/X/GRANT；
- frame 的 `paddr`/`size` 用字符串写十六进制（JSON 整数只有 32 位）；
- irq 可以写数组，展开成每号一条；
- **`"type":"root"`**：把 CAP_ROOT 本身作为策略授予（21.9 节）。

配套的两个工程细节：

- **细化映射区域**：驱动用 `mmio_map_offset(offset, size)` 只映射自己
  要用的子区域（比如 ttyd 只映射 PL011 那一页，virtio 只映射
  0x0a000000 起的 0x4000），于是 frame 票也可以只发那一小段——
  而不是整片 64MB 的 MMIO 窗口。机制（偏移映射）与策略（小范围票）互相成就；
- **没有 cap.json 时**：init 用 `access()` 先探一下，文件不存在就
  完全跳过 cap 设置、按老方式以 root 跑启动脚本——策略是**可选的增量**，
  不绑架既有系统。

有策略时 init 则以 uid 1（普通用户）跑启动脚本，各服务唯一的权限
来源就是这张 JSON。启动日志里能看到每一张发出的票：

```
init: cap policy '/etc/cap.json': 11 rule(s)
init: cap /sbin/sdfsd: frame 0xa000000+0x4000 rw
init: cap /sbin/sdfsd: irq 48 w
...
init: cap /bin/login: root rwxg
init: cap policy installed (67 entries)
```

## 21.8 实战：给自己的驱动配最小权限

假设你写了一个新驱动 `/drivers/virt/myd`，要用的资源是：
MMIO `0x09030000` 一页、中断 40、一块 DMA。

三步走：

1. **驱动代码里只映射自己的地盘**：

   ```c
   mmio_map_offset(0x01030000, 0x1000);  /* 偏移到 0x09030000 那一页 */
   interrupt_setup(40, my_irq_entry, 0);
   dma_user_alloc(0);
   ```

2. **cap.json 加一条规则**：

   ```json
   { "cmd": "/drivers/virt/myd", "caps": [
       {"type":"frame", "paddr":"0x09030000", "size":"0x1000", "rights":"rw"},
       {"type":"irq", "irq":40, "rights":"w"},
       {"type":"dma", "block":0, "rights":"rw"}
   ]},
   ```

3. **重启验证**：启动日志应出现 `init: cap /drivers/virt/myd: ...` 三行，
   驱动正常工作。然后做个反向实验——把 irq 那行删掉再启动，
   `interrupt_setup` 必然失败：票少一张，事就办不成。

这个反向实验最能说明问题：**权限不是默认有、出错才拦；
而是默认没有、声明了才有**。

## 21.9 CAP_ROOT 与 login：信任边界的取舍

严格的能力系统（seL4、EROS 等）拒绝"一票通"：root task 也要
逐对象显式授权。EwokOS 保留了 `CAP_ROOT` 这个"通票"作为务实妥协——
所有 `proc_cap_check_*` 检查遇到 CAP_ROOT 直接短路通过。

为什么留着它？

- 系统是从 uid 模型演进过来的，core/vfsd 这类创世进程要碰的东西
  太多太散，逐一建模收益低；
- 兼容路径需要一个出口（没有 cap.json 的老系统照常工作）。

但 CAP_ROOT 的发放被收窄到一个**明确的信任边界**：
`setuid`/`setgid` 现在要求持 CAP_ROOT（不再是"uid==0 就行"），
而全系统唯一通过策略拿到 CAP_ROOT 的用户态程序是 `/bin/login`——
它就是 Unix 里 setuid-root 的等价物：认证通过后切到目标用户，
`setuid(uid>0)` 落地的瞬间所有票被撕掉（21.5 节），
新会话从"干净"开始。root 登录拿到的权限，来自之后每次 exec
按策略重新发放，而不是 login 把通票传下去。

这就是 EwokOS 的折中：**通票存在，但被制度性地锁进一个审计得过来的小盒子**。

## 21.10 动手练习

1. 在 QEMU 里启动系统，数一下启动日志中 `init: cap ...` 的行数，
   对照 `cap.json` 验证每一条规则展开成了几张票（注意 irq 数组）；
2. 把 ttyd 规则的 `"rights":"rw"` 改成 `"r"`，重启观察：
   frame 检查要求 W 位，ttyd 会在哪一步失败？
3. 思考题：`cap_grant` 给出的是拷贝，为什么这个设计让
   "撤销已发出的权限"变得很难？结合 21.6 节谈谈 cnode 模型下
   可行的补救办法（提示：对象按值校验 + uuid）；
4. 进阶：给 `/bin` 下某个你写的工具加一条 `CAP_PROCESS` 策略
   （cap_policy.c 里的 `a` 参数是目标 pid——想想这类"运行时才知道
   对象"的票为什么不适合写进静态 cap.json）。

## 21.11 本章小结

- capability 把"你是谁"换成"你有什么票"：type + rights + 对象，
  每进程一张 64 槽 cnode；
- 内核在系统调用入口验票：frame/irq/dma/ep/process 各有检查点，
  CAP_ROOT 短路通过；
- 生命周期：内核进程持通票 → fork 继承 → exec 重置并按策略重建 →
  setuid 到普通用户时清零；
- 委派三操作 mint/grant/revoke，铸造集中、转发只能衰减、给的是拷贝；
- `/etc/cap.json` 是声明式策略：init 灌进内核，内核在每次 exec
  按 cmd 匹配自动发票——启动顺序无关、重启自动重发；
- login 是唯一持 CAP_ROOT 的用户态程序，系统的 setuid 信任边界。

下一章进入第 3 部分：文件系统——vfsd 怎么把"一切皆文件"的路由跑起来，
它本身就是本机策略表里拿票最多的服务。
