# 版本变更日志 (Changelog)

本项目遵循 [Semantic Versioning (语义化版本 2.0.0)](https://semver.org/lang/zh-CN/) 规范。

---

## [v1.0.0] - 2026-10-28

### 新增特性 (Features)
- **1.3 寸 SPI LCD 驱动适配与图形显示 (ATK-MD0130)**：
  - 复用正点原子 `12_spilcd` 官方 BSP 组件，成功适配 ST7789V 控制器（兼容 ILI9341 指令序列）。
  - 在 [`components/BSP/LCD/lcd.c`](components/BSP/LCD/lcd.c) 中修正关键配置：将 `SPI_LCD_TYPE` 从 `1` (2.4寸 320×240) 调整为 `0` (1.3寸 240×240)，确保下发精准的 1.3 寸屏幕初始化序列。
  - 重构 [`lcd.c`](components/BSP/LCD/lcd.c) 中 `lcd_display_dir()` 的窗口边界计算，将固定缓冲区与扫描窗口严格锁定为正方形 `240×240`，彻底解决 320×240 越界及清屏仅刷新上半区域的严重缺陷。
  - 在 [`main/main.c`](main/main.c) 中实现多行结构化排版：标题（"Hello" / "World!"）、分割线、硬件标识（"ESP32-S3" / "ATK-DNESP32S3"）及动态增量计数器（1Hz 定频刷新）。
- **XL9555 I/O 扩展芯片总线协同与时序闭环**：
  - 确立严格的硬件拓扑初始化时序：`nvs_flash_init` -> `iic_init (I2C0)` -> `spi2_init (SPI2)` -> `xl9555_init (0x20)` -> `lcd_init`。
  - 通过 I2C0 (SDA=GPIO41, SCL=GPIO42) 寻址 XL9555 (`0x20`)，精准接管屏幕复位引脚 `SLCD_RST_IO (0x0400)` 与背光电源引脚 `SLCD_PWR_IO (0x0800)`。
  - 建立强间接自验判据：利用 `xl9555_ioconfig` 失败重试 3 次后主动触发 `esp_restart` 的特性，以"连续运行未重启"作为 I2C 通路与外设正常应答的高可靠间接证据。
- **RGB565 软件压暗调色与低反差色彩方案**：
  - 针对硬件背光无 PWM 通道（仅经 XL9555 开/关数字 IO 直驱）导致亮度无法调节的问题，创新设计软件层色彩压暗方案。
  - 压低 16 位 RGB565 颜色分量（RGB8 40%~45% 衰减）：背景色采用低噪深灰 `0x1082` (16,16,16)，标题采用柔和蓝 `0x3AD1` (57,89,140)，正文采用中性浅灰 `0x94B2` (148,150,148)，计数器高亮采用暗琥珀 `0x9365` (148,109,41)，避免纯白/满亮红对人眼的眩光刺激。
- **1Hz 双时钟定频互证与可观测性串口输出**：
  - 基于 FreeRTOS `vTaskDelayUntil` 实现严格精确的 1000ms 调度周期。
  - 串口 COM5 与 LCD 面板同步输出递增计数器与 `esp_timer_get_time()` 微秒时间戳，确保脱离显示模组时依然具备完备的系统级可观测性。
- **全套规范化工程文档体系建立**：
  - 新增双语主页指南 [`README.md`](README.md) 与 [`README_EN.md`](README_EN.md)，包含语言导航、状态徽章、硬件架构映射、Mermaid 信号拓扑流向图及 SEO 关键词检索矩阵。
  - 新增面向嵌入式工程师的实战手册 [`USER_GUIDE.md`](USER_GUIDE.md)，详尽覆盖硬件规格、驱动适配细节、调光算法、自验清单与 8 类高频常见故障（FAQ）。
  - 交付复盘全景报告 [`docs/guides/from-zero-to-hello-lcd.md`](docs/guides/from-zero-to-hello-lcd.md) 及纯前端独立渲染 HTML 交互大屏 [`docs/guides/from-zero-to-hello-lcd.html`](docs/guides/from-zero-to-hello-lcd.html)。
  - 建立未竟事项唯一台账 [`docs/ledger.md`](docs/ledger.md)，登记质量缺口 `GAP-001` ~ `GAP-007` 及主动延期项 `DEFER-001` ~ `DEFER-003`。
  - 确立开源协议 [`LICENSE`](LICENSE)（Apache License 2.0）。

---

## [v0.1.0] - 2026-10-27

### 初始基线 (Initial Baseline)
- **Walking Skeleton 最小闭环建立**：
  - 建立基础 ESP-IDF 嵌入式工程骨架，打通本地编译、烧录至芯片及串口通信的全链路。
  - 在 [`sdkconfig.defaults`](sdkconfig.defaults) 中启用 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`，将系统控制台输出精确路由至 ESP32-S3 原生 USB 端口，解决默认 UART0 无输出的假死问题。
  - 锁定 16MB Flash 分区配置 `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`。
- **物理硬件实测与探测识别**：
  - 借助 `esptool flash_id` 完成硬件指纹探测：确认主控为 ESP32-S3 (QFN56) rev v0.2 双核 160MHz、16MB Quad SPI Flash (eFuse 定 3.3V)、8MB 内嵌 PSRAM。
  - 识别通信端口为 ESP32-S3 原生 USB-Serial/JTAG 复合设备（`USB\VID_303A&PID_1001&MI_00`），实测免装 CH340 / CP210x 第三方桥接驱动。
- **双独立时钟 1Hz 定频精度验证**：
  - 设备端通过 `esp_timer_get_time()` 纳秒定时器输出递增运行毫秒数，宿主端通过高精度 Stopwatch 打点，两组数据在 1000ms 周期上严格一致，无累计漂移。
