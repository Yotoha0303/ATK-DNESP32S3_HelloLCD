# 002 · 1.3 寸 LCD 显示 Hello World

- **状态**：已完成（屏幕显示已由用户目视确认）
- **日期**：2026-10-27
- **环路**：`02 开发`（代码）→ `05 部署`（烧录）
- **变更类型**：新增

---

## 1. 背景

需求：让 ESP32-S3 开发板在 **1.3 寸屏**上显示 hello world（最小可用）。

**前置事实的取得过程（重要）**：初次尝试时只知道模组型号 `ATK-MWS3S N16R8`，**不知道开发板型号**。
本框架 D1 禁止凭经验推测引脚，且引脚接错可能触及 Flash/PSRAM 占用的 GPIO 导致启动崩溃。
在用户桌面发现正点原子官方资料后，从**厂商源码**取得标准答案，而非从网络文章推断。

---

## 2. 硬件（实测 + 官方文档）

| 项 | 值 | 来源 |
| --- | --- | --- |
| 开发板 | **ATK-DNESP32S3 V1** | `DNESP32S3 V1.0 硬件参考手册.pdf` 表 1.1.2.1 |
| 模组 | ATK-MWS3S N16R8 | 同上 |
| 芯片 | ESP32-S3 QFN56 rev v0.2 | `esptool flash_id` 实测 |
| Flash / PSRAM | 16 MB / 8 MB | `esptool flash_id` 实测 |
| 屏幕 | ATK-MD0130 1.3 寸 240×240 SPI | 官方例程 `12_spilcd/components/BSP/LCD/lcd.c:29` |

### 引脚定义（取自官方例程，非推断）

出处：`【正点原子】DNESP32S3开发板资料（A盘）\4，程序源码\v5.1.2版本的例程\1，标准例程-IDF版\basic_routines\12_spilcd`

| 信号 | GPIO | 文件:行 |
| --- | --- | --- |
| SPI2_SCLK | GPIO12 | `components/BSP/SPI/spi.h:32` |
| SPI2_MOSI | GPIO11 | `spi.h:31` |
| SPI2_MISO | GPIO13 | `spi.h:33` |
| LCD_DC (WR) | GPIO40 | `components/BSP/LCD/lcd.h:37` |
| LCD_CS | GPIO21 | `lcd.h:38` |
| I2C0_SDA | GPIO41 | `components/BSP/IIC/iic.h:51` |
| I2C0_SCL | GPIO42 | `iic.h:52` |

**屏幕的 RST 与背光(PWR) 不由 GPIO 直驱，而经 XL9555 I/O 扩展芯片**（I2C 地址 `0x20`）：

| IO 功能 | 位掩码 | 文件:行 |
| --- | --- | --- |
| `SLCD_RST_IO` | `0x0400` | `components/BSP/XL9555/xl9555.h:59` |
| `SLCD_PWR_IO` | `0x0800` | `xl9555.h:60` |
| `LCD_BL_IO` | `0x0100` | `xl9555.h:57` |

> **这是本工程最容易踩的坑**：只初始化 SPI 而不初始化 XL9555，屏幕既不复位也没有背光，
> 表现为「全黑、毫无反应」，极易被误判为驱动写错。

---

## 3. 变更内容

### 3.1 复用厂商 BSP（不自己重写驱动）

将官方 `12_spilcd/components/BSP` 整目录复制进本工程 `components/BSP/`
（`IIC` / `LCD` / `LED` / `SPI` / `XL9555`，含 73 KB 字库 `lcdfont.h`）。

**理由**：厂商 BSP 已在该硬件上验证过；自写驱动需要重新调试初始化序列与时序，
违反 §0.2「能跑的东西不得在没有架构指令时重写」。

### 3.2 对厂商代码的**两处**必要修改

| 位置 | 原值 | 改为 | 原因 |
| --- | --- | --- | --- |
| `components/BSP/LCD/lcd.c:29` | `SPI_LCD_TYPE 1`（2.4寸） | `0`（1.3寸） | **两套初始化序列不同**，选错会黑屏 |
| `lcd.c` `lcd_display_dir()` | 横竖屏写死 240×320 / 320×240 | 1.3寸固定 **240×240** | 屏是正方形；沿用 320×240 会导致窗口越界 + 清屏只刷新部分区域 |

两处均以 `/* ===== 本地修改（本项目） ===== */` 标注，便于日后与官方更新比对。

### 3.3 新增 `main/main.c`

- 初始化顺序严格照官方 `12_spilcd/main/main.c:50-54`：`iic → spi2 → xl9555 → lcd`
- 屏幕：黑底，青色 "Hello" / "World!"，白字型号，红色计数行每秒刷新
- 串口：同一计数器同步打印到 COM5（即使屏不亮也能判断程序是否在跑）

---

## 4. 验证

### 4.1 编译 ✅

```
esp32s3_hello.bin binary size 0x44c00 bytes. Smallest app partition is 0x100000 bytes. 0xbb400 bytes (73%) free.
```

BSP 组件已参与编译：`[10/17] Linking C static library esp-idf\BSP\libBSP.a`

### 4.2 烧录 ✅

三笔 `Hash of data verified`，`exit 0`。

### 4.3 启动 ✅

```
I (107) i2c: This driver is an old driver, please migrate your application code to adapt `driver/i2c_master.h`
I (129) gpio: GPIO[40]| InputEn: 1| OutputEn: 0| ... Pullup: 1      ← XL9555 INT 配为输入
I (139) gpio: GPIO[40]| InputEn: 0| OutputEn: 1| ... Pullup: 1      ← LCD DC 配为输出
=== ESP32-S3 1.3" LCD Hello World ===
panel: 240x240, SPI2 @ 60MHz
```

无 panic、无复位循环。

### 4.4 I2C 通路可用（间接判据）✅

`xl9555_ioconfig()` 失败时会重试 3 次后 `esp_restart()`（`xl9555.c:139-155`）。
程序**连续运行 18 秒未重启** ⇒ I2C0(GPIO41/42) 通信正常、XL9555 应答正常
⇒ 屏幕复位与背光控制通路成立。

### 4.5 屏幕实际显示 ✅（用户目视确认）

`2026-10-27` 用户反馈「我看到屏幕上的内容了」⇒ **`SPI_LCD_TYPE 0`（1.3寸序列）与实物屏匹配，初始化成功**。
这是本次变更最关键的一条判据：它同时证明了 SPI 波形、XL9555 复位/背光通路、分辨率设置三者均正确。

### 4.6 配色调整（用户反馈「太亮」）

**根因**：本屏背光由 XL9555 的**数字 IO**（`SLCD_PWR_IO`）控制，**只有开/关，没有 PWM 通道**
（实测 `BSP` 中无任何 `ledc` 调用）。因此亮度无法软件调节，**只能降低颜色值本身**。

改动（`main/main.c`）：

| 用途 | 原值 | 新值 | 说明 |
| --- | --- | --- | --- |
| 背景 | `BLACK 0x0000` | `0x1082` RGB(16,16,16) | 近黑中性深灰，减少纯黑反差 |
| 标题 | `0x07FF` 满亮度青 | `0x3AD1` RGB(57,89,140) | 柔和蓝，亮度降至约 55% |
| 正文 | `WHITE 0xFFFF` | `0x94B2` RGB(148,150,148) | 浅灰，亮度降至约 59% |
| 强调 | `0xF800` 满亮度红 | `0x9365` RGB(148,109,41) | 暗琥珀，亮度降至约 58% |

改后重新 `build` + `flash`，`exit 0`，串口计数正常（未复位）。

---

## 5. 已知局限 / 风险

1. ~~`SPI_LCD_TYPE` 的选择依据是推理，非实测~~ → **已由用户目视确认：`0` 正确**。
   若日后换用 2.4 寸屏，须改回 `SPI_LCD_TYPE 1` 并把分辨率恢复 320×240。
2. **背光无法软件调光**：`SLCD_PWR_IO` 是数字 IO，只有开/关。
   若需要真正调亮度，须改硬件（把背光改接到带 LEDC 的 GPIO），当前板子做不到。
   现阶段只能用颜色值压亮度。
3. **使用了 IDF 已标记为 deprecated 的旧版 I2C 驱动**
   启动日志有告警（`old driver`）。功能正常，但未来 IDF 升级需迁移到 `driver/i2c_master.h`。
   现选择保留旧驱动是因为厂商 BSP 基于它编写，重写会增加风险且超出本次需求。

---

## 6. 下游同步

| 对象 | 状态 |
| --- | --- |
| `README.md` | ✅ 已更新（引脚表 / 目录 / 说明） |
| `docs/ledger.md` | ✅ 已更新 |
| `docs/changes/001-*.md` | ⚠️ 其描述的 hello world 串口版已被本次版本取代，保留为历史 |
