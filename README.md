<div align="right">
  <strong>Languages:</strong>
  <a href="README.md">简体中文</a> |
  <a href="README_EN.md">English</a>
</div>

# ATK-DNESP32S3 1.3寸 LCD 屏幕实战工程与硬件架构规范指南

[![Target](https://img.shields.io/badge/Target-ESP32--S3_N16R8-red.svg?logo=espressif)](https://www.espressif.com/)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4.4-blue.svg?logo=espressif)](https://github.com/espressif/esp-idf)
[![Display](https://img.shields.io/badge/Display-1.3%22_240x240_ST7789-orange.svg)](components/BSP/LCD/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Release](https://img.shields.io/badge/Release-v1.0.0-green.svg)](CHANGELOG.md)
[![Port](https://img.shields.io/badge/Port-USB_JTAG%2FSerial_COM5-brightgreen.svg)]()

本项目基于**正点原子 ATK-DNESP32S3 V1 开发板**（模组型号：ATK-MWS3S N16R8）与 **ATK-MD0130 1.3寸 240×240 SPI LCD 屏幕模组**构建，依托 ESP-IDF v5.4.4 原生开发体系，实现主控芯片、I2C/SPI 双外设总线、XL9555 I/O 扩展芯片与 ST7789V 液晶控制器之间的工业级信号拓扑协同，提供生产级嵌入式显示驱动、双时钟可观测性定频上报与软硬件架构治理规范。

> 📖 **相关指南与手册导航**：
> - 📘 [**用户使用说明书 (USER_GUIDE.md)**](USER_GUIDE.md)：面向嵌入式工程师的完整实战指南（电气引脚矩阵、初始化拓扑、RGB565 调光算法、FAQ 排坑）
> - 🌐 [**从零到一全流程复盘 (from-zero-to-hello-lcd.md)**](docs/guides/from-zero-to-hello-lcd.md)：真实端到端踩坑复盘，含 6 级验证阶梯与 [HTML 交互大屏](docs/guides/from-zero-to-hello-lcd.html)
> - 📝 [**版本更新日志 (CHANGELOG.md)**](CHANGELOG.md)：从 v0.1.0 串口 Walking Skeleton 到 v1.0.0 完整发布的特性演进
> - 📊 [**质量缺口唯一台账 (docs/ledger.md)**](docs/ledger.md)：未竟事项与工程债务（GAP-001 ~ GAP-007）严格 SSOT 追踪

---

## 一、架构层级与硬件组件映射

本项目严格按照软硬件分层设计原则，将物理外设、通信总线、IO扩展、屏幕模组及固件运行时解耦为 6 大标准架构层级：

| 架构层级 | 工具 / 硬件组件 | 实际硬件 / 部署形式 | 对应目录与核心源码 | 架构职责与关键技术指标 |
| :--- | :--- | :--- | :--- | :--- |
| **1. 物理微控制器主控** | **ESP32-S3 (QFN56) rev v0.2** | 板载贴片 (ATK-MWS3S N16R8) | `sdkconfig.defaults`<br>`sdkconfig` | Xtensa® 双核 32 位 LX7 处理器 @ 160MHz，16MB Quad SPI Flash (3.3V)，8MB 内嵌 PSRAM (AP_3v3) |
| **2. 高速外设与通信总线** | **SPI2 (FSPI) + I2C0** | 原生 SoC 外设控制器 | `components/BSP/SPI/`<br>`components/BSP/IIC/` | SPI2 (GPIO11/12/13/21/40) 运行于 60MHz DMA 高速传输；I2C0 (GPIO41/42) 运行于 400kHz 标准主模式 |
| **3. 板载低速 IO 扩展矩阵** | **XL9555 I/O 扩展芯片** | I2C 从设备 (从机地址 `0x20`) | `components/BSP/XL9555/` | 解锁 GPIO 资源限制，接管屏幕硬件复位 `SLCD_RST_IO (0x0400)` 与背光供电 `SLCD_PWR_IO (0x0800)` |
| **4. 物理显示与光学输出模组**| **ATK-MD0130 1.3寸 LCD** | 物理外设插槽模组 | `components/BSP/LCD/`<br>`lcdfont.h` (73KB) | Sitronix ST7789V 驱动 IC，240×240 正方形 IPS 视口，RGB565 16位色彩，`SPI_LCD_TYPE 0` 专用初始化序列 |
| **5. 嵌入式固件运行时** | **FreeRTOS + ESP-IDF v5.4.4**| 固件二进制应用镜像 | `main/main.c` | 双核任务调度，1Hz `vTaskDelayUntil` 定频循环，16位 RGB565 软件压暗调色算法，微秒级设备运行时钟 |
| **6. 工程架构与质量门禁** | **CMake 构建 + 门禁规范** | 声明式规范与台账体系 | `CMakeLists.txt`<br>`docs/`<br>`docs/ledger.md` | 遵循四大工程纪律（D1-D4），维护 `docs/changes/` 与未竟 GAP 台账，实现 100% 可证伪自验 |

### 交互式 Mermaid 硬件信号与数据流全景图 (Hardware Signal & Data Flow Diagram)

<details open>
<summary><b>🔍 点击展开 / 收起交互式 Mermaid 硬件拓扑与数据流向图 (Interactive Hardware Topology)</b></summary>

```mermaid
flowchart TD
    subgraph HostPC ["💻 开发者宿主工作站 (Host Environment)"]
        direction TB
        Dev["👤 嵌入式工程师 / 自动化流水线"]
        IDE["🛠️ PowerShell / VS Code / CMake"]
        COM["🔌 COM5 串口监视器 (115200 波特率)"]
        Dev --> IDE
        Dev --> COM
    end

    subgraph ESP32S3 ["⚡ 层级 1: ESP32-S3 双核主控 (ATK-DNESP32S3 V1 / N16R8)"]
        direction TB
        CPU["🧠 Dual-Core Xtensa LX7 @ 160MHz"]
        Flash["💾 16MB Quad SPI Flash (3.3V)"]
        USBCore["🔌 USB-Serial/JTAG 原生控制器 (VID:303A PID:1001)"]
        
        subgraph Peripherals ["层级 2: 高速通信总线"]
            SPI2["🚀 SPI2 总线 (60MHz DMA 驱动)<br/>SCLK=GPIO12 · MOSI=GPIO11 · MISO=GPIO13"]
            I2C0["📡 I2C0 总线 (400kHz 标准主模式)<br/>SDA=GPIO41 · SCL=GPIO42"]
            GPIOCtrl["📌 专用控制引脚<br/>LCD_CS=GPIO21 · LCD_DC(WR)=GPIO40"]
        end

        CPU <--> Flash
        CPU --> USBCore
        CPU --> SPI2
        CPU --> I2C0
        CPU --> GPIOCtrl
    end

    subgraph IO_Expander ["🔀 层级 3: 板载低速 IO 扩展矩阵 (XL9555)"]
        XL9555["🎛️ XL9555 I2C 扩展芯片 (从机地址 0x20)"]
        RST_Pin["SLCD_RST_IO (掩码 0x0400 / P12)"]
        PWR_Pin["SLCD_PWR_IO (掩码 0x0800 / P13)"]
        XL9555 --> RST_Pin
        XL9555 --> PWR_Pin
    end

    subgraph DisplayModule ["📺 层级 4: ATK-MD0130 1.3寸 240×240 IPS 液晶模组"]
        ST7789["🕹️ ST7789V 驱动 IC (SPI_LCD_TYPE=0)"]
        Panel["🌈 240×240 IPS 视口 (RGB565 调光色彩呈现)"]
        Backlight["💡 物理 LED 背光板 (数字 IO 电平开关)"]
        ST7789 --> Panel
        Backlight -.->|"光学照明"| Panel
    end

    %% 连接关系
    IDE ==>|"一键编译与烧录 (idf.py -p COM5 flash)"| USBCore
    USBCore ==>|"运行时 1Hz 双时钟日志回传"| COM

    SPI2 ==>|"像素数据与显示指令 (60MHz 突发传输)"| ST7789
    GPIOCtrl ==>|"片选使能与数据/命令切换"| ST7789

    I2C0 <==>|"I2C 配置与状态读取 (400kHz)"| XL9555
    RST_Pin -->|"低电平硬件复位脉冲 (100ms 保持)"| ST7789
    PWR_Pin -->|"高电平接通背光电源"| Backlight
```

</details>

#### 🔍 核心硬件规格与关键参数检索表 (Hardware Spec & Keyword Matrix)

> **SEO 索引与规格检索说明**：为保证工程与硬件关键参数在检索体系中结构化呈现，下表提供全景参数索引：

| 核心硬件组件 / 模块名称 | 英文检索关键词 (Keywords) | 部署层级与物理引脚 | 电气特性与通信总线 | 核心职责与关键技术参数 |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32-S3 主控芯片** | `ESP32-S3`, `QFN56`, `Xtensa LX7`, `N16R8`, `AP_3v3` | 层级 1 (核心主控) | 3.3V TTL / 160MHz 双核 | 负责系统调度、FreeRTOS 任务流转、双时钟输出及外设总线初始化 |
| **原生 USB-Serial/JTAG** | `USB-Serial`, `JTAG`, `VID_303A`, `PID_1001`, `COM5` | 层级 1 (通信端点) | `D+=GPIO20, D-=GPIO19` | 复合 USB 虚拟串口设备，原生免驱，承载固件极速烧录与运行时控制台监视 |
| **SPI2 (FSPI) 总线** | `SPI2`, `FSPI`, `SCLK 12`, `MOSI 11`, `MISO 13` | 层级 2 (高速外设) | 60MHz 时钟 / DMA 缓冲区 | 驱动 ST7789V 传输高频图形数据与控制指令，单帧快速刷新无闪烁 |
| **I2C0 控制总线** | `I2C0`, `SDA 41`, `SCL 42`, `Fast-mode 400kHz` | 层级 2 (控制外设) | 开漏上拉 / 400kHz | 挂载与寻址板载外设，与 XL9555 I/O 扩展芯片建立双向数据握手 |
| **XL9555 I/O 扩展器** | `XL9555`, `I2C Expander`, `Address 0x20`, `Bitmask` | 层级 3 (IO 矩阵) | I2C 从机地址 `0x20` | 控制屏幕复位 `SLCD_RST_IO (0x0400)` 与背光供电 `SLCD_PWR_IO (0x0800)` |
| **ST7789V 1.3" 液晶屏** | `ST7789V`, `ATK-MD0130`, `240x240`, `SPI_LCD_TYPE 0` | 层级 4 (光学显示) | 4-Wire SPI / RGB565 | 1.3寸正方形视口，通过专用初始化序列与固定窗口渲染 UI 界面 |
| **RGB565 软件压暗调色** | `RGB565`, `Software Dimming`, `16-bit Color Palette` | 层级 5 (算法运行时) | 16-bit 像素格式 | 针对硬件缺乏 PWM 调光的限制，通过算法压暗像素值消除刺眼眩光 |

---

## 二、开发环境与工具链配置 (Windows & Linux 双平台)

### 1. Windows PowerShell 激活指南
本工程基于 Espressif 官方 EIM 工具链（ESP-IDF **v5.4.4**）安装：
- `IDF_PATH` = `D:\IDF-tools\v5.4.4\esp-idf`
- `IDF_TOOLS_PATH` = `C:\Espressif\tools`

在 Windows 下启动 **PowerShell 终端**（不可直接在 Git Bash 中运行），执行一键激活：

```powershell
# 1. 激活 ESP-IDF v5.4.4 环境变量
. 'C:\Espressif\tools\Microsoft.v5.4.4.PowerShell_profile.ps1'

# 2. 进入项目目录并验证工具链
cd D:\MyDocuments\esp32_env\ai-collaboration-framework\esp32s3-project
idf.py --version
# 终端回显验证: ESP-IDF v5.4.4
```

### 2. Linux Bash 激活指南
在 Linux / macOS 开发环境下，通过系统默认 Bash 终端快速载入：

```bash
# 1. 导出 ESP-IDF 环境变量
. $HOME/esp/esp-idf/export.sh

# 2. 检查编译器工具链版本
xtensa-esp32s3-elf-gcc --version
```

### 3. 免驱动 USB 原生 JTAG/Serial 特性说明
正点原子 ATK-DNESP32S3 开发板通过 ESP32-S3 芯片内置的 USB PHY 直接对外引出 USB Type-C 接口：
- **设备硬件 ID**：`USB\VID_303A&PID_1001&MI_00`
- **免驱架构**：现代操作系统原生支持通用 CDC-ACM 串口驱动，**无需在宿主机额外安装 CH340 / CP210x 等任何第三方转串口芯片驱动**。
- **端口核对 (PowerShell)**：
  ```powershell
  Get-CimInstance Win32_PnPEntity | Where-Object { $_.DeviceID -match 'VID_303A' }
  # 验证结果: Present=True, ConfigManagerErrorCode=0
  ```

---

## 三、快速开始指南（编译、烧录、监视一键闭环）

### 1. 常用命令速查表

| 操作阶段 | 完整执行指令 | 耗时与预期输出 | 成功判定标准 |
| :--- | :--- | :--- | :--- |
| **工程全量清理** | `idf.py fullclean` | 约 2 秒，清理 `build/` 目标产物 | 彻底清除增量编译缓存与宏状态 |
| **一键编译构建** | `idf.py build` | 约 15~30 秒，输出固件大小分布 | 退出码 `0`，生成 `esp32s3_hello.bin` |
| **固件极速烧录** | `idf.py -p COM5 flash` | 460800 波特率，约 5 秒 | 输出三笔 `Hash of data verified.` |
| **启动终端监视** | `idf.py -p COM5 monitor` | 实时输出控制台日志 | 观察到 `Hello world!` 连续打印，按 `Ctrl+]` 退出 |
| **一键流水线闭环** | `idf.py -p COM5 flash monitor`| 自动完成烧录并无缝唤起监视终端 | 编译 -> 烧录 -> 验证完整打通 |

### 2. 换屏幕类型与 RGB565 配色调优指南

#### A. 屏幕尺寸选型（1.3寸 vs 2.4寸）
在 [`components/BSP/LCD/lcd.c`](components/BSP/LCD/lcd.c) 第 29 行定义了硬件屏幕类型：
```c
#define SPI_LCD_TYPE    0    /* 0: 1.3寸 240x240;  1: 2.4寸 320x240 */
```
> ⚠️ **强制规范**：本项目实物为 1.3 寸屏，**必须确保设为 `0`**。若设为 `1` 将因下发 ILI9341 宽屏初始化指令序列而导致完全黑屏。改动后必须执行一次 `idf.py fullclean`。

#### B. RGB565 配色调优与软件压暗
屏幕背光供电走 XL9555 数字开关，无硬件 PWM 调光通道。为了解决高背光下的刺眼问题，颜色值在 [`main/main.c`](main/main.c) 顶部通过软件算法调优：

```c
#define BG_COLOR        0x1082   /* 背景: RGB(16,16,16)   低噪近黑深灰，柔和舒适 */
#define TITLE_COLOR     0x3AD1   /* 标题: RGB(57,89,140)  柔和雅致蓝，降亮 45% */
#define BODY_COLOR      0x94B2   /* 正文: RGB(148,150,148) 浅中性灰，彻底替代刺眼纯白 */
#define ACCENT_COLOR    0x9365   /* 强调: RGB(148,109,41) 暗琥珀色，替代满亮度大红 */
```

---

## 四、硬件通信与点亮手动自验排查清单 (逐项排查清单)

### 0. 一键自动快速诊断（基于已实测运行日志）
烧录完成后打开监视器，设备上电应输出以下自检签名：
```
=== ESP32-S3 1.3" LCD Hello World ===
panel: 240x240, SPI2 @ 60MHz
[     1] Hello world!  uptime=1000 ms
[     2] Hello world!  uptime=2000 ms
```
- **核心判定准则**：`xl9555_ioconfig` 在寻址失败时会尝试重试 3 次，若依然无应答则调用 `esp_restart()` 强制系统重启。**只要程序连续运行超过 15 秒且无复位循环，即间接证明 I2C0 通路畅通、XL9555 扩展芯片握手成功、屏幕背光与复位供电完全正常**。

---

### 1. 逐层手动检查对照表

| 架构层级 | 校验组件 / 信号 | 手动自验命令 (终端执行) | 预期输出与现象判定 |
| :--- | :--- | :--- | :--- |
| **层级 1: 物理主控** | USB PnP 识别 | `Get-CimInstance Win32_PnPEntity \| Where-Object { $_.DeviceID -match 'VID_303A' }` | 命中 `VID_303A:PID_1001`，设备状态良好正常 |
| | 芯片硬件探测 | `esptool.py --port COM5 flash_id` | 探测到 `ESP32-S3 (QFN56) rev v0.2`，Flash 16MB |
| **层级 2: 高速总线** | 控制台路由 | 检查 `sdkconfig.defaults` | 包含 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` |
| | SPI2 总线 | 启动日志打印 | 输出 `panel: 240x240, SPI2 @ 60MHz`，无 DMA 报错 |
| **层级 3: IO 扩展** | XL9555 应答 | 观测开机运行状态 | 连续运行 >15 秒无 `esp_restart`，I2C0 寻址正常 |
| **层级 4: 光学显示** | 屏幕点亮确认 | 物理目视核验 1.3寸 LCD 表面 | 呈现深灰背景、蓝色 Hello World 与每秒递增计数器 |
| **层级 5: 固件运行时** | 1Hz 定频互证 | 记录串口打印 `uptime` 毫秒序列 | 设备自报 `uptime` 增量严格等于 1000ms，双时钟对齐 |

---

### 2. 硬件架构健康评分卡 (Scorecard)

```
================================================================================
                  ATK-DNESP32S3 硬件架构健康评分卡 (Scorecard)
================================================================================
[√] 1. 原生 USB PnP 通信枚举 (VID:0x303A, PID:0x1001)       : 100% 正常可用
[√] 2. ESP32-S3 核心指纹探测 (v0.2 / 16MB Flash / 8MB PSRAM) : 100% 正常可用
[√] 3. 控制台路由重定向 (CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)   : 100% 正常可用
[√] 4. I2C0 与 XL9555 IO 扩展通信 (从机地址 0x20)            : 100% 正常可用
[√] 5. SPI2 60MHz DMA 高速传输总线 (GPIO11/12/13/21/40)      : 100% 正常可用
[√] 6. ST7789V 1.3寸初始化与正方形视口 (SPI_LCD_TYPE=0)       : 100% 正常可用
[√] 7. FreeRTOS 定频任务调度与双时钟互证 (1Hz / 1000ms)      : 100% 正常可用
--------------------------------------------------------------------------------
综合健康评级: 100% (READY FOR DEPLOYMENT / 硬件架构生产验证就绪)
================================================================================
```

---

## 五、开源许可证 (License)

本项目遵循 [Apache License 2.0](LICENSE) 国际开源许可证。
商业友好，允许自由修改、分发与商业应用。
