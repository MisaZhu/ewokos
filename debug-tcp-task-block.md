[OPEN] tcp-task-block

# 现象

- `netd` 压力仍主要集中在初始两个线程。
- 某个 TCP 连接任务线程（示例里是 `88`）状态为 `blk`，不符合“连接 task 在自己的新线程里独立推进且不长期阻塞”的预期。

# 假设

- H1: TCP 连接类路径仍在 `tcp_connect()` 或 `tcp_accept()` 内部执行阻塞式 `sched_sleep()`，导致连接 task 自己进入 `blk`。
- H2: 新 task 虽然已经提前创建线程，但真正的连接命令仍由旧的主 task/旧线程持有和推进，造成初始线程承担主要负载。
- H3: `task_thread()` 的状态切换把连接类请求长期停在 `PROCESS`，并没有回到事件驱动的重试路径。
- H4: `xprocs` 里看到的 `blk` 线程确实是连接 task 的 worker，但这本身来自当前设计里 connect/accept 的同步等待，并非线程创建时机问题。
- H5: 某些 TCP 唤醒路径只唤醒了用户态 `VFS_EVT_RD/WR`，没有把挂起中的连接 task 重新推进，导致旧线程忙、新线程阻塞。

# 计划

1. 给连接类 task 和 TCP 连接状态迁移加最小埋点。
2. 复现一次连接建立过程，确认谁在执行、谁在阻塞、阻塞点在哪。
3. 基于日志只修真正的阻塞根因。

# 当前证据

- 新截图表明：新建 `netd` task 线程已经开始承担负载，不再是完全空转。
- 但两个原始 `netd` 固定线程仍然压力较大；用户反馈这会拖慢 `accept` 接受新请求。
- `device_run()` 通过单次 `ipc_setup(handle_ipc, ...)` 注册设备服务，所有 `/dev/net0` 的 `OPEN/READ/WRITE/CNTL/POLL` 都串行进入同一个 `handle()` 分发路径。
- TCP 新连接入队发生在协议线程里：`tcp_segment_arrives()` 把已建立子连接 `queue_push(&pcb->parent->backlog, pcb)` 后，再唤醒监听 socket。

# 假设更新

- H1: 连接 task 本身未创建线程。`已否定`
- H2: `connect/accept/read` 在 task worker 内部同步阻塞。`部分已修复`
- H3: `/dev/net0` 的设备服务线程是单 IPC handler 串行瓶颈，导致大量 `READ/WRITE/POLL/CNTL` 抢占 `accept`。`高度怀疑`
- H4: 协议线程 `net_thread/intr_loop` 集中处理所有收包/状态迁移， backlog 入队和可读唤醒都依赖它，负载高时直接拖慢 `accept`。`高度怀疑`
