# Debug Session: xbrowser-https-read [OPEN]

## Symptom

- `xBrowser` 打开 `https://httpbin.org` / `https://httpbin.org/html` 不稳定失败。
- 已观测到三类日志：
  - `failed to dns resolv`
  - `error flushing`
  - `invalid read code: -1 errno=0`

## Focus

- 当前先收敛 `gethostbyname()` 的 UDP 查询设计与 `recvfrom()` ABI。

## Hypotheses

1. `recvfrom()` IPC 封包按请求长度而非实际返回长度拷贝，污染 DNS 响应内容。
2. `SOCK_RECVFROM` 没返回真实 `addr_len`，导致用户态无法可靠解析来源地址。
3. DNS 查询包/响应匹配机制不完整，旧包或非目标响应可能被误收。
4. DNS 问题修完后，HTTP/TCP read 路径仍有独立错误。

## Evidence

- `system/network/libs/socket/src/socket.c` 中 `recvfrom()` 原先对 payload 使用 `proto_read_to(..., n)`，不是 `ret`。
- `system/network/drivers/netd/task.c` 的 `SOCK_RECVFROM` 原先只返回 `ret + payload + addr`，没有单独编码 `addrlen`。
- `system/network/drivers/netd/stack/sock.c` 的 UDP `sock_recvfrom()` 原先没填写 `sin_family` 和 `*addrlen`。

## Changes

- 修复 `recvfrom()` 按实际返回长度 `ret` 拷贝 payload。
- 修复 `SOCK_RECVFROM` 回传真实 `addrlen`。
- 修复 UDP IPv4 `sock_recvfrom()` 填充 `sin_family` 与 `*addrlen`。

## Next

- 重建 network 相关组件并验证：
  - `host httpbin.org`
  - `/bin/https_test http://httpbin.org/html`
  - `xBrowser https://httpbin.org/html`
