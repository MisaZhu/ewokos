# Q8B pcied 实机验证 — 2026-11-02

已完成两路 TC9563 以太网 PCI 功能及一个 WCN6855 的上电和枚举。已连接的以太网口与 Wi-Fi 实际通信通过后，将 pcied 从 initramfs 迁到持久 SD rootfs；经 EUD MCP 复位启动后再次验证成功。最终启动恢复 `rootfs=auto`，正常桌面、串口登录及 SSH 均运行。

## 最终启动证据

09:09:11 发起 EUD MCP reboot；下列时间来自 MCP 常驻串口缓存（Asia/Shanghai）：

```text
09:10:21.260 init: rootfs selected=sd bootnet=0
09:10:21.260 init: /sbin/switchroot
09:10:22.990 q8b pcied: start, entering PCIe3A/PCIe4 power-on/probe
09:10:25.006 q8b pcied: snapshot ethernet_pf=2 wifi=1
09:10:25.006 q8b pcied: host service ready, 7 device(s)
```

`/bin/ps` 确认只有一个 pcied（PID 53），网卡和 WLAN 分别为持久路径 `/drivers/q8b/net`（PID 59）和 `/drivers/q8b/wlan`（PID 63）。启动过程没有人工启动、重启网络驱动。

| 控制器 | BDF | ID | 功能 |
| --- | --- | --- | --- |
| PCIe3A（pcie1） | 01:00.0 | 1179:0623 | QPS615 upstream bridge |
| PCIe3A | 02:01.0 | 1179:0623 | DSP1，板级配置关闭外部端口 |
| PCIe3A | 02:02.0 | 1179:0623 | DSP2，板级配置关闭外部端口 |
| PCIe3A | 02:03.0 | 1179:0623 | DSP3，连接集成以太网 |
| PCIe3A | 05:00.0 | 1179:0220 | Ethernet PF0 |
| PCIe3A | 05:00.1 | 1179:0220 | Ethernet PF1 |
| PCIe4（pcie3） | 01:00.0 | 17cb:1103 | WCN6855，subsystem 17cb:0108 |

`pcieprobe --check-q8b` 返回 `ethernet_pf=2 wifi=1`。PF0/PF1 端点链路 Gen3 x4，WCN6855 Gen2 x1。PCIe2A/3B 时钟/电源关闭，跳过 ECAM。

## 实际通信

| 接口 | 冷启动后的地址 | 验证结果 |
| --- | --- | --- |
| 有线 PF0，/dev/net0 → /dev/eth0 | 192.168.8.50/24 | 10/10 Ping，0% 丢包；SSH 执行及 SCP 回读三份驱动成功 |
| Wi-Fi，/dev/net1 → /dev/wlan0 | 192.168.5.69/24 | WPA connected、network_ready=1；10/10 Ping，0% 丢包 |

有线使用 EEPROM MAC `88:12:4e:00:03:de`，QCA8081 PHY ID `004d:d101`，当前接线协商为 **1000Mbps 全双工**。不是 2.5Gbps 吞吐测试。验证结束时 `rx=594 tx=1152 dropped=0 errors=0 fatal=00000000`；出现过一次 RBU，已恢复（status=0，全部 32 个接收描述符重新归 DMA）。

Wi-Fi 为 HE160、2×2、5240MHz，WPA 握手无 MIC/replay 错误，DP 完成计数持续增长且 tx_errors=0。该驱动本次 MAC 为 `00:03:7f:12:2d:1d`，DHCP 地址随本次启动变化；旧的 192.168.5.68 不是本次地址。

迁移前的 initramfs 隔离测试中，有线 192.168.8.50 与 Wi-Fi 192.168.5.68 同时各 5/5 Ping 成功，Wi-Fi 还通过 SSH/SCP。随后才实施启动位置迁移。

## 实现与修复

- pcied 独占控制器上电、PHY、PERST、LTSSM、iATU、ECAM、桥和 BAR 配置；设备驱动通过 `/dev/pcie0` 查询快照并映射分配的 BAR。移除设备驱动中的总线初始化路径。这是驱动职责分离，没有新增内核 MMIO 权限隔离。
- WCN6855 探测前先启用 E-key 3V3、RPMh 电源和 radio GPIO，再按 GCC/GDSC、PERST、RC/PARF、PHY、DBI、链路的顺序初始化。避免 secure TCSR 访问，链路或时钟未就绪时禁止 ECAM 探测。
- PCIe3A 补齐 buses 2..7 的 CFG1 outbound iATU；此前仅有 bus 1 的 CFG0，无法访问 QPS615 下游。设置 upstream 的 1→2..5 和 DSP 的 child bus 3/4/5，并检查桥 ID、header 和寄存器读回。PCI Command/Link Control 使用 16 位访问，避免写到相邻 W1C 状态位。
- 主机在关闭 decode/DMA 后探测 BAR 大小，恢复原值，规划整个桥层级，再编程/验证 BAR 与桥窗口，最后启用 MEMORY|MASTER。同级窗口按 1MiB 分隔；64 位 BAR 保留属性。PCIe3A 分配 bus 0x40000000..0x40800000，PF0 BAR0/2/4 对应 CPU 0x490000000/0x490080000/0x490200000；PF1 对应 0x490400000/0x490480000/0x490600000。WLAN BAR0 大小 2MiB，对应 CPU 0x410000000。
- WCN SOC reset 清除 PCI command/MSI 后，由 pcied 的固定 WLAN 操作恢复 DMA/MSI。仅接受已识别的 PCIe4 WCN6855 功能；MSI sink 由服务持有，客户端不能选择寄存器或 DMA 目标。WLAN 无需映射 ECAM。
- TC9563 冷初始化补齐 MAC/PMA 时钟和复位、EEPROM MAC 读取、按 PHY 协商速度配置 SGMII/XPCS。XPCS reset 必须先于 modal PCS 选择，否则 MMD31 再次隐藏。BAR0 映射修正为实际 16KiB。
- init 通过实际 `/dev/pcie0` owner 判断是否需要过滤重复 pcied，而非把早期脚本成功当成 owner 存在。initramfs 早期脚本仅启动 timer/ramfs；持久 basic/gui/xwin 配置在设备驱动前启动 pcied。临时自动 root console 引导程序已从启动镜像移除。

参考主线：`~/work/kernel/drivers/pci/controller/dwc/pcie-designware-host.c`、`drivers/pci/setup-bus.c`、`drivers/net/ethernet/stmicro/stmmac/dwmac-tc956x.c`、`drivers/net/pcs/pcs-xpcs.c`、`drivers/misc/tc956x_pci.c` 以及 Q8B DTS。

## 检查、部署和范围

BSP、pcied、net、wlan 和更新后的 init 构建通过。资源规划测试覆盖层级窗口、peer 隔离、对齐、无重叠与空间不足；测试源码保存为 `resource-plan-test.c`。顶层和 machines 仓库的 `git diff --check` 通过。SCP 回读的持久 pcied/net/wlan 与构建产物逐字节一致；哈希见 `SHA256SUMS`。启动镜像已部署到 EUD PXE 的 `q8b/INITRAMFS.IMG`，本阶段没有更新 KERNEL.BIN 或 BOOTAA64.EFI。

最终复位到验证结束的日志未观察到 PCIe 初始化 EL2 abort、SError 或 watchdog 复位。此前 08:37 使用旧持久设备驱动的一次启动出现过 `SError swallowed cnt=1 esr=be000011 far=0002dfe0`；此次结果不构成对该历史异常根因的证明。

第二个网卡功能仅完成枚举和资源分配，未测第二物理口；未进行 2.5Gbps 吞吐、热插拔、动态 PHY 速率变化、长时间压力或整板断电测试。本次为 EUD 硬件复位启动，不等同于拔电冷启动。

证据：`rootfs-boot.log`、`rootfs-wired-ping.log`、`rootfs-wifi-ping.log`。早期 `initramfs-success.log` 和 `switchroot-success.log` 保留作枚举历史证据，以本报告及 rootfs-boot.log 为最终结论。
