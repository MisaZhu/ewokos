# 26 精度触控板（Precision Touchpad）方案

> 中文版 · 英文版见 [26-xtouchpad.md](26-xtouchpad.md)

## 1. 现状分析

### 1.1 现有输入链路

EwokOS 目前只有一个三层输入管道（USB 鼠标 / 键盘 / 单点触摸都是这条链）：

```
usbhostd (USB HID host) ──┐
btd (蓝牙 HID)          ──┼─► hid_srv 订阅队列 ──► hid_touchd / hid_moused / hid_keybd
                          │        (按 report id 分发给订阅者守护进程)
                          └►       每个守护进程持有 /dev/usb0 或 /dev/bt0 的事件订阅
                                            │
                                            ▼
              xtouch / xmouse / xim ──dev_cntl_by_pid(X_DCNTL_INPUT)──► xserverd (/dev/x)
                                            │
                                            ▼
                                     Qt 应用 / 窗口管理器
```

- `usbhostd` 用 `libhid`（`system/gui/libs/hid`）解析 HID 报告描述符，把设备归一化成事件，通过 `hid_dispatch_evt()` 按 report id 广播；订阅者守护进程用 `fcntl(cmd 0)` 订阅对应 report id，然后按固定步长 `HID_MAX_EVENT_SIZE` 读事件。
- `hid_touchd` 负责触摸：它只处理**单触点**的 7 字节 `[state, x_lo, x_hi, y_lo, y_hi, 0, 0]` 事件，合并（≤100Hz）后写成 6 字节 `/dev/touch0` 协议 `[state u16, x u16, y u16]`。
- `xtouch` 读 `/dev/touch0`，把触点**绝对**映射到屏幕坐标，直接发 `XEVT_MOUSE`。
- 这条链路面向的是 goodix SPI 触摸屏、单点电阻屏之类设备（触摸 = 点击，x/y = 绝对位置），没有"触点身份"、"相对位移"、"手势"的概念。

### 1.2 PTP 设备是什么、差距在哪

Precision Touchpad（PTP，HID 多触点触控板）是 Windows 8 之后定义的 HID 标准触摸板形态（ELAN / Synaptics / 苹果式触控板都符合）。它的 HID 报告描述符是：

- 应用集合 `(Digitizer, Touch Pad)`（usage page 0x0D，usage 0x05）；
- 内部有**多个** `Finger` 逻辑集合（usage 0x22），每个触点一个集合，字段包括：
  - `Contact Identifier 0x51`（触点编号，设备保证同一根手指在生命周期内编号不变）
  - `Tip Switch 0x42`（是否接触）与 `Confidence 0x47`（是否真的是手指）
  - `X 0x30 / Y 0x31`（逻辑坐标）
  - `Width 0x48 / Height 0x49`（接触面积）
- 应用级还有 `Contact Count 0x54`（实际触点个数）与物理按键（Button 页）。

它和现有链路的核心差距：

| 维度 | 现有 hid_touchd / xtouch | PTP 需要 |
| --- | --- | --- |
| 触点 | 单个 | 最多 5 个、有稳定 id |
| 坐标语义 | 绝对（映射整屏） | 相对位移 + 逻辑范围 x_max/y_max |
| 点击 | 触摸即点击 | 点按（短按单击、原地长按拖动）/ 物理按键 |
| 手势 | 无 | 无（仅跟踪第一指，不识别多指手势；双击折叠为单击） |
| 传输 | dev 事件已归一化 | 需要从 HID 位域提取（位偏移不齐） |

**结论**：不改造 `/dev/touch0` 单触点协议（goodix 触摸屏路径依赖它），而是**新增一条独立的 PTP 链路**（`/dev/touchpad0` + `xtouch -m` 精准触控手势模式），手势引擎并入现有 `xtouch`（不再需要独立守护进程），与「hid_moused→xmouse、hid_touchd→xtouch」的成对模式保持一致。双链路并存，按设备类型自动选择。

## 2. 总体方案

```
USB/蓝牙 HID 报告
   │
   ▼
libhid 新增 PTP 解析器（hid_parse_touchpad_report / touchpad_normalize_report）
   │  （描述符探测：Digitizer/Touch Pad 应用集合 + Finger 集合 → HID_DEV_TYPE_TOUCHPAD）
   ▼
usbhostd：探测 PTP → boot 协议 SET_PROTOCOL(1) + SET_IDLE(0)；
          轮询循环按报告 id 过滤、去重，normalize 成 touchpad_evt_t，
          以 HID_REPORT_ID_TOUCHPAD(5) 广播（步长 HID_TOUCHPAD_EVENT_SIZE=58）
   │
   ▼
hid_touchpadd：订阅 report 5，58 字节帧**原样**发布到 /dev/touchpad0
   │  （不做合并：手势层需要每一帧，PTP 报告率 60~125Hz）
   ▼
xtouch -m：读 /dev/touchpad0，仅跟踪第一指，转换鼠标事件
   │  （原地 300ms = 拖动、短按 = 单击、移动 = 指针；物理按键）
   ▼
xserverd（XEVT_MOUSE）
```

### 2.1 数据帧格式（58 字节，`touchpad_evt_t`）

```c
typedef struct __attribute__((packed)) {
    uint8_t id;          /* 触点编号（HID Contact ID） */
    uint8_t tip;         /* 1=接触          */
    uint8_t confidence;  /* 1=确信是手指    */
    uint8_t reserved;
    uint16_t x, y;       /* 逻辑坐标（0..x_max/y_max） */
    uint8_t width, height;
} touchpad_contact_t;                      /* 10 字节 */

typedef struct __attribute__((packed)) {
    uint8_t buttons;          /* bit0 左, bit1 右, bit2 中 */
    uint8_t contact_count;    /* 实际触点个数（≤5） */
    uint8_t in_range;
    uint8_t reserved;
    uint16_t x_max, y_max;    /* 触控板逻辑范围（供手势层缩放） */
    touchpad_contact_t contacts[HID_TOUCHPAD_MAX_CONTACTS];  /* 5×10 */
} touchpad_evt_t;             /* 8 + 50 = 58 字节 */
```

携带 `x_max/y_max` 是关键设计决定：转换层不需要任何"这台触控板"的配置，直接用 逻辑值/逻辑范围 × 屏幕尺寸 换算指针位移，跨设备一致。

### 2.2 libhid 解析器要点

文件：`hid_defs.h`（+`HID_REPORT_ID_TOUCHPAD 5`、usage 常量、`HID_TOUCHPAD_EVENT_SIZE`）、新增 `hid_touchpad.h`（结构体 + `touchpad_parser_t`）、`hid_report.c`（解析 + 归一化）。

- **识别**：`hid_detect_device_type()` 在 Digitizer 应用集合内看到 `usage==Touch Pad(0x05)` 就标记 `found_touchpad`；返回顺序 **TOUCHPAD > TOUCH**（PTP 描述符同样能满足单触点 touch parser，必须让 touchpad 赢，否则只会点按、没有手势）。
- **截获字段**：进入 `Finger` 逻辑集合后，按「首次出现优先」记录接触级字段（id/tip/confidence/x/y/width/height）的**位偏移**；指尖端落集时 `slot_bits = 报告总位数 − 首槽起始位`。之后每个触点的字段 = 首槽偏移 + `i×slot_bits`。
  - **兼容性坑 1**：PTP 描述符里 X/Y 是 Generic Desktop usage，但厂商**不会**切回 usage page 0x01 —— 它们就出现在 Digitizer 页内。解析器对 0x30/0x31 不检查页（Digitizer 页无同名 usage，无冲突）。
  - **兼容性坑 2**：没有 repeat（0x37）。HID item tag 只有 4 位，`0x37` 会撞上全局 tag 3（Physical Minimum）；长项（0xFE）与现有 parser 一样整体跳过，靠「显式重复 Finger 集合」的主流厂商做法覆盖多触点。
  - **兼容性坑 3**：不假设字节对齐。所有提取都是 LSB-first 位读取（`touchpad_extract` 带边界检查）。测试里特意用一个 54 位/槽（非字节对齐）的描述符验证。
- **报告级字段**：应用集合内的 Contact Count / 物理按键 / In Range。`Contact Count` 的 logical_max → `max_contacts`（缺省 1）。
- **归一化**：校验报告 id 与长度（真实设备总是发完整报告）；每触点按 `i×slot_bits` 提取并 clamp 到 u8/u16；`contact_count` clamp 到 max_contacts 与 5。
- 报告 id 1 通常兼作鼠标/按键报告 —— 解析出的报告 id 会被 usbhostd 用作过滤键。

### 2.3 usbhostd 集成

- 探测：枚举描述符时做两轮（单触点 touch probe + touchpad probe）；`touchpad_desc_ok` 为真则注册 `HID_DEV_TYPE_TOUCHPAD`。
- 初始化：boot 模式设备 `SET_PROTOCOL(1)`＋`SET_IDLE(0)`；slot type `HID_INPUT_TOUCHPAD`，`report_len = max_packet`（PTP 报告 id 1 常与鼠标报告多路复用）。
- 轮询：按 parser 报告 id 过滤 → `memcmp` 去重 → `touchpad_normalize_report()` → `hid_dispatch_evt(HID_REPORT_ID_TOUCHPAD, payload, HID_TOUCHPAD_EVENT_SIZE)`。

### 2.4 hid_touchpadd

对齐 `hid_touchd` 的骨架：`hid_connect()` 注册 report 5 订阅 → 批读（64 帧环、`ipc_disable` 批量 drain）→ 原样写 `/dev/touchpad0`，失败重连。**不合并帧**：手势层需要每一帧的触点状态。

## 3. xtouch -m 精准触控手势模式

PTP 转换引擎**并入现有 xtouch**（原独立 `xtouchpad` 守护进程方案废弃，减少一个 X 输入进程），以 `-m` 参数激活。文件：`system/xwin/sbin/xtouch/xtouch.c`。运行参数：`xtouch [-c 配置文件] [-m] [设备]`；PTP 模式默认配置 `/etc/x/xtouchpad.json`、默认设备 `/dev/touchpad0`，绝对模式默认 `/etc/x/xtouch.json`、`/dev/touch0`。

### 3.1 触点跟踪（仅第一个手指）

- 每帧取**第一个** `tip` 有效（confidence 通过）的触点作为跟踪对象，**其余手指全部忽略**——多触点触控板一律按单指处理，不识别任何多指手势；
- 新触点落地时锚定 `lx=x`（本帧位移为 0）——跨抬起的帧绝不算位移，触点重新落地也不会跳变；
- Confidence 启发式保留：只要本帧任何触点带 confidence=1，就要求跟踪触点 `confidence=1`（手掌不计入）；整帧都没有 confidence 时不要求（兼容老设备）。

### 3.2 转换逻辑（状态机）

```
IDLE ──1指落地──► WAIT ──原地≥tap_time(300ms)──► DRAG（合成 LEFT DOWN，移动=DRAG）
  │                  │                                抬起 ──► LEFT UP + IDLE
  │                  ├──原地<tap_time 抬起──► CLICK（DOWN+UP；双击折叠为单击）
  │                  └──位移>move_threshold(20)──► MOVE（纯指针移动，无按键；抬起=结束）
```

- **原地停留 ≥ 300ms**（位移 ≤ tap_move，默认 5 逻辑单位）→ **进入 DRAG**：合成 LEFT DOWN，此后移动发 `MOUSE_STATE_DRAG`，**手指抬起发 LEFT UP 释放**。300ms 判定由主循环定时器 `tp_tick()` 驱动——即使触控板在手指停留期间完全静默（不发帧）也能按时进入拖动；
- **原地按下 < 300ms 抬起** → **CLICK**（DOWN+UP）；快速双击折叠为一次单击（300ms 窗口 + 点击点距离容差 2×tap_move）；
- **按下后移动**（位移 > move_threshold，默认 20 逻辑单位）→ **MOVE 模式**：纯指针移动（不带按键），抬起结束；<20px 的小幅度抖动不触发指针移动（防抖），仍按原地判定（300ms→拖动 / <300ms 抬起→单击）；
- **物理按键**：按键沿独立发 DOWN/UP（LEFT/RIGHT/MID）；按下会取消待定手势；按键按住期间单指移动 = `MOUSE_STATE_DRAG` 带所按按钮——传统"按住拖拽"；
- **仅检测第一个手指**：多指同时落下时只按第一指的位置/动作处理，无滚动/捏合/三指等组合手势；
- **超时释放兜底**：任何状态（DRAG 合成按下、物理按键按住、WAIT/MOVE 待定）**超过 100ms 没有新数据**即强制释放：合成拖动补 LEFT UP、仍按住的物理按键逐个补 UP，状态回 IDLE——静默的触控板永远无法让按键卡住。usbhostd 对触控板**全量透传（含字节相同帧）**，因此"手指按住不动持续被报告"不会误触发；真正无数据（移开手指/设备停发）才会 100ms 释放。

### 3.3 事件形态（对齐现有协议）

- 指针：`XEVT_MOUSE`，`relative=1`，rx/ry 相对增量（位移×gain×屏幕/逻辑范围，带小数累加避免小位移截断为 0 丢帧）；DOWN/UP/DRAG 用按钮字段；服务器在 DOWN 时记 `down_pos`，DRAG 的 from_x/from_y 和 3px 拖拽阈值都由 xserverd 推导，客户端只管增量；
- 点击/拖动：`MOUSE_STATE_DOWN/UP/DRAG` + `MOUSE_BUTTON_LEFT`。

### 3.4 配置（/etc/x/xtouchpad.json）

```jsonc
{
  "tap_enable": true,        // 点按/拖动转换开关
  "tap_time": 300,           // ms，原地停留进入 DRAG 的阈值；更早抬起 = 单击
  "tap_move": 5,             // 逻辑单位，"原地"判定半径（≤0 自动 x_max/64）
  "move_threshold": 20,      // 逻辑单位，按下后位移超过此值进入 MOVE（≤0 自动 x_max/64）
  "gain_x": 1.0, "gain_y": 1.0  // 指针速度
}
```

`tap_move`/`move_threshold` 的自动值按**本帧的 x_max** 换算，因此同一配置在不同分辨率触控板上手感一致；配置缺失时 xtouch -m 按以上默认值运行。

## 4. 接线与构建

- `system/gui/drivers/hid_touchpadd/`：新驱动（镜像 hid_touchd，只链 libewoksys）
- `system/xwin/sbin/xtouch/`：现有客户端，PTP 手势引擎并入，`-m` 激活精准触控模式
- `system/xwin/etc/x/xtouchpad.json`：PTP 模式示例配置部署到根文件系统 `/etc/x/`
- `system/gui/drivers/Makefile`：DIRS 增加 hid_touchpadd
- 机器 init.rd 接线（本方案**仅落地 q8b**，不触碰其他平台；其他平台如需接入可参照同样方式自行动 `init.rd`）：
  - `machines/q8b/system/etc/xwin/init.rd`：
    `@/bin/ipcserv /drivers/q8b/hid_touchpadd /dev/touchpad0 /dev/usb0`
    `@/bin/bgrun /sbin/x/xtouch -m`

无 PTP 设备时不影响启动：hid_touchpadd 打开节点即可（订阅不要求设备在场），xtouch -m 阻塞读零帧，零开销。

## 5. 测试与验证记录

解析器曾配套一个宿主原生单测（`tests/test_hid_touchpad.c`，22 项断言：描述符分类、5 指槽解析、54 位非对齐槽、2 触点归一化、报告 id 过滤），验证后按仓库维护要求移除，未随提交保留。

构建验证（本仓库环境）：

- x86 全量机器构建 `make -C machines/x86/system` **通过**：libhid、usbhostd（x86 bsp 含 USB）、hid_touchpadd、xtouch（绝对 + -m 双模式）全部链接成功，`etc/x/xtouchpad.json` 部署进镜像（另修复了 x86 gcc 16.1.0 下 xim/vkey C++ 窄化报错）。
- aarch64/q8b：libhid、usbhostd、hid_touchpadd 以及合并后的 xtouch.c 均交叉编译零警告；usbhostd 用**真实的 q8b libbsp**（含 USB 控制器实现）全量链接成功。

## 6. 已知限制与后续工作

1. **传输层**：当前只走 USB HID over USB / 蓝牙 HID。I2C-HID 触控板（多数笔记本内置板用）需要新的 i2c-hid host，未实现，留作后续。
2. **蓝牙 PTP**：btd 复用同一 usbhostd 事件面，但未实测；若蓝牙板子报告 id/描述符行为与 USB 不同，需在 btd 侧做同样探测。
3. **无多指手势**：按当前设计仅跟踪第一指（无滚动/捏合/三指），如需多指手势需扩展状态机。
4. **手掌抑制**：只用 confidence 启发式；工业级 palm rejection（误触抑制、边缘抑制）未做。
5. **超时释放**：xtouch 侧 100ms 无数据强制释放所有按下状态（拖动停顿 >100ms、或设备停发时按键/接触自动 UP）；usbhostd 侧 300ms 无接触报告合成抬起帧作为第二层兜底。手指原地不动且设备**完全停发**时，"原地停留 300ms 进拖动"会被 100ms 兜底打断（回 IDLE，恢复发帧后按新接触重新判定）——这是"100ms 强制释放"与"300ms 原地进拖动"在停发设备上的取舍，100ms 释放优先。
6. **hid_srv 队列步长**：`HID_MAX_EVENT_SIZE` 提升到 58 字节，所有订阅者读步长一致（小事件零填充），对现有 mouse/touch 路径无影响，但队列每帧占 58B。
7. 未实现 Buttons 页的 top/barrel 等特殊键及 `HID 1.11` 的 Repeat 计数（主流 PTP 用显式重复集合，无需）。

## 7. 涉及文件清单

| 文件 | 改动 |
| --- | --- |
| `system/gui/libs/hid/include/hid/hid_defs.h` | +`HID_REPORT_ID_TOUCHPAD`、usage、`HID_TOUCHPAD_EVENT_SIZE` |
| `system/gui/libs/hid/include/hid/hid_touchpad.h` | 新增：帧结构 + 解析器状态 |
| `system/gui/libs/hid/include/hid/hid_report.h` | +`HID_DEV_TYPE_TOUCHPAD`、解析/归一化声明 |
| `system/gui/libs/hid/include/hid/hid_srv.h` | +`HID_INPUT_TOUCHPAD` |
| `system/gui/libs/hid/src/hid_report.c` | 描述符探测、位域解析、normalize |
| `system/gui/libs/hid/src/hid_srv.c` | 类型名映射 |
| `system/gui/drivers/usbhostd/usbhostd.c` | PTP 探测/初始化/轮询 |
| `system/gui/drivers/hid_touchpadd/` | 新增驱动 + Makefile |
| `system/gui/drivers/Makefile` | DIRS |
| `system/xwin/sbin/xtouch/xtouch.c` | PTP 单指转换引擎并入，`-m` 精准触控模式 |
| `system/xwin/sbin/xim/vkey/xim.cc` | 3 处 char 窄化强转（x86 gcc 16.1.0 编译修复） |
| `system/xwin/etc/x/xtouchpad.json` | PTP 模式示例配置（部署为 /etc/x/） |
| `machines/q8b/system/etc/xwin/init.rd` | 接线（仅 q8b） |