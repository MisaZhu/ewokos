# Debug Session: scp-stall
- **Status**: [OPEN]
- **Issue**: `scp misa@10.91.9.175:/bin/ssh /tmp/1` 在输入密码后长时间停在 `0% 0 0.0KB/s`，普通 SSH 登录可继续，但 `scp`/SFTP 数据通道没有向客户端推进。
- **Debug Server**: not-started
- **Log File**: not-started

## Reproduction Steps
1. 在宿主执行 `scp misa@10.91.9.175:/bin/ssh /tmp/1`
2. 输入密码 `misa`
3. 观察进度停在 `0%`，直到用户中断

## Hypotheses & Verification
| ID | Hypothesis | Likelihood | Effort | Evidence |
|----|------------|------------|--------|----------|
| A | `sshd` 的 internal sftp/scp 输出路径在 `send_lock` 或 channel window credit 恢复上卡住，21B/状态包发不出去 | High | Medium | Pending |
| B | `netd` 的 `SOCK_SEND` 在短写/EAGAIN 后没有稳定重新发布 `VFS_EVT_WR`，导致 `ssh_write_n()` 一直等可写 | High | Medium | Pending |
| C | `sshd` 主循环对 socket+pipe 的 `poll`/可见性组合仍有缺口，SFTP 子线程/relay 已准备好但主线程没被唤醒 | Medium | Medium | Pending |
| D | `sftp-core` 读文件路径已经拿到 `/bin/ssh`，但首个 `SSH_FXP_DATA/STATUS` 组包后被窗口/分片策略饿死 | Medium | Medium | Pending |

## Log Evidence
- 用户提供的运行时证据：`scp` 在认证后卡在 `0%`
- 近期 `netd: write send ... ret=21/224/1704` 表明发送路径至少曾经真正出过字节，并非完全没有进到 `sock_send()`
- `machine.virt` 上按 `scp -P 2222 misa@localhost:/bin/ssh /tmp/1` 顺序回归 20 次全部通过，问题不在通用命令路径，而在 raspi5 实机链路
- raspi5 实机复现：`scp misa@10.91.9.175:/bin/ssh /tmp/1` 在输入密码后稳定停在 `ssh 0% 0 0.0KB/s - stalled -`
- 用户提供的实机屏幕日志显示：
  - `sshd: serve_client banner_recv sock=5 client=SSH-2.0-OpenSSH_9.9`
  - 随后连续出现多条 `sshd: channel_out sent pid=117 ext=0 chunk=36/41/.../17`
  - 之后又出现 `sshd: channel_out sent pid=117 ext=0 chunk=4032`（多次）
  - `win_left` 仍然维持在约 `2MB` 量级，没有掉到 0
  - 最终 `sshd: serve_client done sock=5 ret=-1 err=peer closed`

由这组证据可直接推出：
- 不是认证前卡住，SFTP/通道已经建立并开始出数据
- 不是 `remote_window == 0` 导致的 SSH channel window 耗尽，因为 `win_left` 仍很大
- 更像是实机上的 socket 写完成/继续推进链条在发送数个数据包后丢了后续推进，或者客户端迟迟收不到完整响应后主动断开

## Verification Conclusion
- 现阶段判断：
  - A: 基本排除为“首包没发出去”或“remote window 先耗尽”
  - B: 最高优先级。最新实机日志已证明 `task_wakeup_tcp_writers()` 在持续发生，旧的“根本没有 writer wake”假设明显减弱；更精确的问题变成：`do_network_write()` 每次唤醒只做一次 `sock_send()`，raspi5 上又频繁短写，导致进度过度依赖 ACK 节拍
  - C: 下载路径下优先级明显下降
  - D: 仍待与 B 区分

## Current Instrumentation
- `system/network/drivers/netd/task.c`
  - `do_network_write()` 仅在 `EAGAIN/EINTR` 退让时打印 `netd: send defer ...`
  - `do_network_write()` 仅在短写时打印 `netd: send short ...`
  - `task_wakeup_tcp_writers()` 在 writer 被 ACK/窗口更新重新拉起时打印 `netd: writer wake ...`

## Current Fix Under Test
- `system/network/drivers/netd/task.c`
  - `do_network_write()` 改为在同一轮 worker 里持续 `sock_send()`，直到当前 write slot 真正耗尽或 socket 真的变成不可写
  - 保留短写 / defer 的 `slog`，用于判断 raspi5 上是否还会停在某个碎片尾巴（如 1/41/81/119 字节）
