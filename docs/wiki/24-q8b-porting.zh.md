# 24 Q8B 移植参考（Dragon Q8B / SC8280XP 实证）

> 语言: [English](24-q8b-porting.zh) | **中文**
>
> 本章目标：把 EwokOS 从树莓派 5 移植到瑞莎 Dragon Q8B 的**实证参考资料**。
> 内容为 2026-10 实测/核实结果（Q8B 实机 + 官方文档 + 主线源码），标注证据等级：**【实测】**=Q8B 实机、【官方】=官方文档、【社区】、【推断】。

## 24.1 目标硬件基线（Q8B）

| 项 | 值 | 证据 |
|---|---|---|
| SoC | Qualcomm SC8280XP（Snapdragon 8cx Gen 3，5nm） | 【官方】 |
| CPU | 4× Cortex-X1 @3.0GHz + 4× Cortex-A78 @2.4GHz，AArch64 | 【官方】 |
| GPU | Adreno 690（a6xx 家族，OPP 270–690MHz） | 【官方/社区】 |
| 内存 | LPDDR4x，16GB，128-bit 总线（8× x16 channel） | 【官方】 |
| DDR 实际频率 | **4184 MT/s**（BIOS DRAM Settings 实测；标称 4266） | 【实测】 |
| 内存带宽 | **read ~40.5 GB/s / write ~41.6 GB/s**（8 线程 sysbench 32G 长测稳态） | 【实测】 |
| 引导 | SPI NOR 完整 UEFI（EDK2 系），非 U-Boot | 【官方】 |
| 调试串口 | 40-pin GPIO Pin6 GND / Pin8 TXD / Pin10 RXD（SoC UART17），115200 8N1 | 【官方】 |
| 网络 | 2× 2.5GbE RJ45（QPS615 交换机，AVB/TSN）、WiFi/BT 扩展 | 【官方】 |
| 存储 | UFS + microSD + 2× M.2 M-key | 【官方】 |
| 显示 | DP0→CH7218A→HDMI；DP1/DP2→USB-C DP Alt Mode | 【官方/社区】 |

### 内存带宽实测（对自研 OS 的预算基线）

```
理论峰值    = 4184 MT/s × 16 B = 66.9 GB/s
8 线程 read  = 39.7~40.6 GB/s（60%）
8 线程 write = 41.1~41.6 GB/s（62%）
4×X1 大核   = 34.0 GB/s（每核 ~8.5）
单线程拷贝   = 15.6 GiB/s（mbw MCBLOCK；双向总线流量 ~33 GB/s）
```

**结论**：内存子系统健康、DDR 全档；~40 GB/s 是 SoC NoC/控制器实际极限。GPU 显存、显示写回、双 2.5GbE、NVMe DMA 都共享这条总线。【实测】

## 24.2 引导链：RPi5 vs Q8B（移植最大差异点）

| 环节 | RPi5（现 EwokOS） | Q8B | 移植动作 |
|---|---|---|---|
| 固件加载 | GPU 固件（start.elf 等）→ config.txt | **SPI NOR UEFI（XBL→EDK2）** | 内核做成 **UEFI 应用（.efi）** |
| 启动入口 | kernel8.img（固定地址） | UEFI 按 PE/COFF 加载 .efi | 链接成 PE 镜像 + UEFI 入口 |
| 设备树 | 固件传 dtb | UEFI **自动加载并修正** `sc8280xp-radxa-dragon-q8b.dtb` 传给内核 | 直接接收 UEFI 的 DTB |
| 帧缓冲 | mailbox PROPTAG 申请 | **UEFI GOP**（Graphics Output Protocol） | 读 GOP 帧缓冲（参照 TW220 实证：0xc6200000, 2560×1600, a8r8g8b8, stride 2560×4；**Q8B 地址待实测**） |
| 调试输出 | Mini UART / PL011 | UART17（Pin6/8/10），`console=ttyMSM0,115200n8` | 改串口驱动寄存器基址（sc8280xp UART17） |
| 引导菜单 | config.txt 固定 | **UEFI Shell / Boot From File / Boot Menu(F12)** | 自研 OS 走 "Boot From File" 或 PXE |

**Q8B 无需 U-Boot**：主线 U-Boot 无 SC8280XP 支持（clk Kconfig / dts / mach-snapdragon 均无，已核实）；UEFI 已提供全部引导能力。

### UEFI 关键操作（实测/官方）

- 进 BIOS：开机按 **F2**（主界面：Radxa Platform Configuration / Device Manager / Boot Manager / Boot Maintenance Manager）
- 启动菜单：**F12**
- 调试串口（UEFI 阶段）：BIOS → Radxa Platform Configuration → **Debugging Settings** → `Synchronous Debug UART in UEFI` 改为 Enabled（默认 Disabled）+ `Debug Print Level` 改 Debug；重启后 UEFI 日志从 Pin6/8/10 输出
- 引导自研内核：Boot Maintenance Manager → **Boot From File** → 选择 .efi；或 UEFI Shell 手动执行
- 救砖/刷 BIOS：**EDL**（板载按键或 BIOS "Reboot into EDL/9008"）+ edl-ng v1.6.0

## 24.3 显示与 GPU（RPi5 V3D vs Q8B Adreno 690）

| 能力 | RPi5（videocore/v3d） | Q8B 路径 | 工作量评估 |
|---|---|---|---|
| 显示/帧缓冲 | mailbox framebuffer（fbdisplayd） | **UEFI GOP 帧缓冲**（初期）→ 后续 msm DP/DPU | GOP 显示 1–2 周（参照 ewokos framebuffer.c 改造） |
| 2D | bsp_g2d / g2dd | GOP 直写 + graph/opengl | 几乎白拿 |
| 3D | v3d_3d.c（实验性 CLE） | **Adreno 690 无裸机最小路径**：需 freedreno/mesa 路线（ir3 编译 + MSM ioctl 兼容层） | 最小闭环 4–9 人月（路线 A） |
| 现状 | — | deepin 25 已在 Q8B 用 freedreno 跑通 3D（实证） | 【官方/社区】 |

**移植顺序建议**：GOP 显示（1–2 周）→ opengl 3D 功能等价（RPi5 现有能力）→ 内核 GPU 栈（SMMU→ioctl→GMU→GEM/Submit→turnip）。

## 24.4 调试与开发环境（实证）

| 通道 | 方式 | 备注 |
|---|---|---|
| 串口 | Pin6/8/10（UART17），115200 8N1，USB-TTL（禁接 VCC） | 内核 console=ttyMSM0；UEFI 日志需开 Synchronous Debug UART |
| PXE 网络引导 | dnsmasq（proxyDHCP 共存模式）+ TFTP/HTTP + grubaa64.efi | Q8B 按 F12 选 PXE；ARM64 内核即 PE/EFI stub 镜像，可直接做 NBP |
| EDL 救砖 | 板载 EDL 键 / BIOS 菜单 → edl-ng | 变砖兜底 |
| EUD（USB 调试） | **BIOS 中有 EUD 选项【实测】**；固件级启用、不依赖 OS | 主线 Linux 对 SC8280XP 的 EUD 驱动未覆盖（binding 仅 sc7280）；OpenOCD --enable-eud 路径可用；列为中期增强项 |
| 热迭代 | 编译 → PXE 重启 → 串口看日志 | 免刷卡 |

## 24.5 关键地址/寄存器（待上机确认项）

- [ ] **GOP 帧缓冲地址**（Q8B 实机；TW220 参照 0xc6200000，2560×1600 a8r8g8b8）
- [ ] UART17 寄存器基址（sc8280xp.dtsi 中 `serial@...`，对照 40-pin 复用表 GPIO_63/64）
- [ ] BIOS EUD 选项：菜单路径 / 名称 / 默认状态 / 对应 Type-C 口
- [ ] 内存映射（UEFI 内存图：RAM 基址/大小、MMIO 保留区）

## 24.6 参考资料索引（q8b 工作目录 /Users/tony/work/q8b/）

| 文件 | 内容 |
|---|---|
| 瑞莎Dragon-Q8B开发板资料分类检索清单.md | 8 大类 60+ 条资料（含 URL 与可访问性） |
| Q8B-EUD嵌入式USB调试模式.md | EUD 原理/状态/步骤/限制 |
| GPU驱动名词解释.md | 70+ 术语（PM4/ir3/GMU/SMMU/tu_knl…） |
| freedreno项目调研与自研OS-GPU驱动可行性评估.md | GPU 路线 A 评估（4–9 人月） |
| TW220与Q8B设备树对比.md / Q8B与SurfacePro9-5G硬件差异分析.md | 同 SoC 设备 DTS 对照 |
| sc8280xp-radxa-dragon-q8b.dts / sc8280xp.dtsi | 板级与 SoC 设备树 |
| msm_analysis/ | 内核 msm 驱动源码参考（GPL 只读） |

**注意**：内核 `drivers/gpu/drm/msm` 为 GPL-2.0-only（只读参考勿拷贝）；`msm_drm.h` uAPI 头文件按 MIT 可移植；Mesa freedreno/turnip 为 MIT 可复用。


## 24.7 已落地实现（EwokOS `machines/q8b/`）

2026-10 已在 EwokOS 树内落地第一里程碑，源码与构建说明见
`machines/q8b/README.md`。要点：

| 项 | 实现 |
|---|---|
| 启动 | `kernel/boot/uefi/BOOTAA64.EFI`：SFS / EFI_LOAD_FILE / EFI_LOAD_FILE2 读 `KERNEL.BIN`、`ROOTFS.IMG`，GOP/内存图/DTB，`ExitBootServices`，bootinfo @ `0x90070000`，跳 `0x90080000` |
| 内核 | RAM_BASE 感知 `mkos.lds.S`、早期页表 + 早期 vector、bootinfo 解析、GENI UART17、GICv3、CNTV、PSCI 8 核、RAM disk 优先 + SDCC2 实体卡只读 fallback |
| 用户态 | `bsp_fb`（GOP，RGB/BGR 交换）、`bsp_sd`（RAM disk 优先，否则用户态 SDCC2 驱动直接读写实体卡）、`uartd`（GENI TX/RX）、`fbdisplayd` + `displaymand/consoled/fontd` |
| 根文件系统 | 112MB ext3，UEFI 载入内存；`core/vfsd/sdfsd` 启动，串口 `tty0` + 图形 `console0` 双会话 |
| microSD | SDCC2 4-bit/25MHz，CMD17/CMD18 PIO，CSD 容量解析；实机验证 32GB 卡 33-sector（内核启动期 32+1 分块）读取、GPT 条目与 ext superblock 可从用户态读取。运行期读写由用户态 SDCC2 驱动负责（CMD24 单扇区写；CMD25 未启用）；用户态自行映射 Command DB、解析 PMIC 轨地址并完成上电/识别，不依赖内核 SD 状态；内核不再提供 SD 块 syscall |
| 回归 | `make test-loader`、`make test-kernel`：QEMU virt + EDK2，覆盖 UEFI→内核→ext3→init→core/vfsd/sdfsd 全链路 |

QEMU 测试使用 Q8B 的同一份代码，仅把 RAM_BASE/PL011/GICv2 换成 QEMU virt
（`TEST_QEMU=1`）；QCOM 专属的 GENI/GICv3/PSCI/UFS/GOP 仍需实机确认。

**踩坑记录（已修复）**：

1. `mkos.lds.S` 的 `phys_base` 必须跟 `RAM_BASE` 一致（QEMU 测试变体通过
   `--defsym=phys_base=$(RAM_BASE)`），否则 `.text` 高半区 VMA 与早期页表别名
   不一致，一跳就 prefetch abort。
2. 从 `.init`（低物理地址）访问高半区符号（如 `&interrupt_table_start`）会走
   GOT，而 GOT 可能在 ±4GB 之外；改为 `bsp/early_vectors.S` 的 `ldr =symbol`
   绝对字面量。
3. UEFI `AllocatePages` 只保证 4KB 对齐，16KB 页内核映射 `ROOTFS.IMG` 时必须
   把缓冲区向上圆整到 16KB，否则 `SYS_MEM_MAP` 解析出的物理地址会比 bootinfo
   低 0x0-0x3FFF，用户态读到错误数据。
4. RAM disk 必须在 `load_kernel_config()` 之前映射（`arch_vm()` 里做），不能等
   `init_allocable_mem()`。
5. 早期异常向量必须在开 MMU 后立即安装；否则早期 fault 会落到 UEFI 遗留的
   VBAR，表现为在 `0xbfdxxxxx` 反复 prefetch abort，看不到真正原因。

6. **EUD SRST 后进入 900e**：UEFI 判定启动成功后，EUD SRST 可能让 SBL 落到
   `05c6:900e`（Sahara/EDL）。**再发一次 EUD reboot/SRST** 即可回到 UEFI 启动
   流程（2026-10-18 实测，原生 Type-C）。MCP `reboot`/`eud_board_reboot` 已自动
   以串口缓存判定：UEFI/PXE 即成功，Sahara 才重发（最多 2 次）；旧 900e 仅作
   超时兜底。EUD 监控只读检测 9501，不自动重连/不自动使能 CTL。
7. **microSD GPT 不能只看类型 GUID**：实测卡上第 1 分区是 Linux filesystem
   GUID 但没有 ext superblock，第 3 分区才是 label=`rootfs` 的 ext4；其 incompat
   feature `0x22c6`（含 extents/64-bit 等）不受当前 EwokOS ext2/3 reader 支持。
   GPT 根盘选择现同时校验 ext magic、block/inode size 与受支持 feature，避免误选
   或尝试启动现有 Linux rootfs。运行期读写走用户态 SDCC2 驱动；内核只在启动期
   只读加载 init，不再暴露 SD 块 syscall。

8. **4K（3840x2160）下应用能启动却没有窗口**：Q8B 没有 g2d，所有画布都落在
   scattered shm 窗口（`SHM_MAX_SIZE`，旧值 128MB）。4K 一帧 31.6MB，显示端要
   两块（扫描输出 + libdisplay 的 `cache_g` 后备缓冲，约 63MB），每个窗口还要
   `ws_g`/`ws_g2`/`frame_g`/frost `backdrop` 各约 31.6MB（全屏时约 127MB），
   128MB 窗口很快耗尽，`graph_new_shm()` 返回 NULL。更隐蔽的是客户端
   `xwin_open()` 忽略了服务端 `UPDATE_INFO` 的失败：返回的窗口 `ws_g_shm_id`
   已经是 -1，客户端却认为打开成功，`xwin_repaint()` 永远画不出东西——表现为
   “进程在跑、窗口不出现”。修复：aarch64 的 `SHM_MAX_SIZE` 提到 512MB
   （4K 基线约 230MB，留约 2 倍余量，同时给 scattered shm 设上限）；
   `xwin_open()` 在 `ws_g_shm_id == -1` 时返回 NULL；`xserverd` 在分配失败时
   打印窗口标题与尺寸。实机佐证：1080p 下 `ps` 报 `shm 57 MB`，按像素数放大
   到 4K 约 230MB，超过旧 128MB 窗口。

## 24.8 NVMe（PCIe2a x4）用户态驱动（实机）

Q8B 的两个 M.2 M-key 槽由固件完成 PCIe 控制器/链路/iATU/BAR 配置，
EwokOS 侧保持纯用户态驱动：

- `machines/q8b/system/libs/bsp/src/nvme.c`：ECAM 枚举 → BAR0 映射 →
  admin/I/O 队列 → identify → 批量读流水线；`nvprobe` 为实机探针。
- 内核只加了一条放行：`check_mem_map_arch()` 允许 pcie2a（`0x5_0000_0000`
  ECAM / `0x5_1000_0000` / `0x5_2000_0000`）与 pcie3b（`0x4_4000_0000` …）
  窗口。

**实机（Lexar 1TB SSD，126f:2263，NVMe 1.3）**：

| 项 | 结果 |
| --- | --- |
| 链路 | PCIe Gen3 x4（speed=3, width=4） |
| 容量 | 1,953,525,168 × 512B（953,869 MiB），MDTS=6（256KB） |
| 顺序读（原始 DMA） | **1772 MiB/s**（256MB，128/256KB，QD≥4） |
| 顺序读（拷贝到 cached 缓冲） | **~1.2 GiB/s**（双缓冲流水线 + 128-bit 拷贝） |
| 一致性 | QD1 vs QD32 两次读 IDENTICAL |

**两个关键坑**（都表现为 identify 超时/无命名空间）：

1. ASQ/ACQ/IOSQ/IOCQ 基址必须 **4KB 对齐**；紧凑布局（ACQ=+0x400）会让
   控制器不取 admin 命令，而 CSTS 仍显示 RDY。
2. Identify Controller 的 **NN 在 0x204**（SQES/CQES 在 0x200），不是 0x184。

**pcie3b 空槽**：槽位为空时固件下电对应 RC，读 ECAM 触发外部 abort
（`SError swallowed` + 用户进程 data abort），因此默认只扫 pcie2a。

**副作用发现**：批量 SSH/scp 会让 `/drivers/q8b/net` 在连续 1500B 帧下
停住（ping/ssh 全断、串口仍活）；`scp -l 300~500` + 32KB 分片可绕过，
根因在 `bsp_eth.c` 的 RX 环，待单独修。

**代码/命令**：`nvprobe scan|gpt|bench [MB]|verify [MB]`；`make -C
machines/q8b/system/drivers/nvprobe`。下一步是 `nvmed` 块设备 daemon /
`sdfsd` NVMe 后端与写路径。
