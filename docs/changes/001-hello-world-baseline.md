# 001 · ESP32-S3 Hello World 定频基线

- **状态**：已完成
- **日期**：2026-10-27
- **环路**：`02 开发`（代码）→ `05 部署`（烧录验证）
- **变更类型**：新增

---

## 1. 变更背景

工程从零起步。目标是先建立一条**可验证的最小闭环**：代码 → 编译 → 烧录 → 串口可见输出 → 输出频率可测。
在尚无业务需求（见 `docs/planning/baseline-20261027.md` §5）的情况下，这是唯一能提供「能跑的东西」的最小单元，
也是后续任何功能开发的前提（`AGENTS.md` §0.2「已完成的不可协商基线」）。

---

## 2. 变更内容

### 新增文件

| 文件 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 工程入口，`project(esp32s3_hello)` |
| `main/CMakeLists.txt` | 组件注册，源文件 `hello_world_main.c` |
| `main/hello_world_main.c` | 定频打印实现 |
| `sdkconfig.defaults` | 工程默认配置（见下） |

### 关键实现选择

**① 周期用 `vTaskDelayUntil` 而非 `vTaskDelay`**

`vTaskDelay(n)` 是「从**现在**起睡 n tick」——打印本身的耗时会累加进每个周期，长期运行频率必然偏慢。
`vTaskDelayUntil(&last_wake, period)` 对准**上一次唤醒的绝对时刻**，误差不累积。

频率由单一宏控制：

```c
#define HELLO_PERIOD_MS 1000
```

**② 控制台改走 USB-Serial/JTAG**

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
```

本板**没有** USB 转串口桥接芯片（实测 `VID_303A&PID_1001` = Espressif 原生 USB-Serial/JTAG）。
IDF 默认控制台是 UART0，若不改此项，烧录能成功但串口**完全无输出** ——
这是最容易被误判为「烧录失败」的形态。

**③ Flash 容量按实测配置**

```ini
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
```

依据：`esptool flash_id` → `Detected flash size: 16MB`。

**④ 输出带序号与 uptime**

```c
printf("[%6" PRIu32 "] Hello world!  uptime=%" PRId64 " ms\n",
       count, esp_timer_get_time() / 1000);
```

`uptime` 来自芯片自己的 `esp_timer`，使频率可以从**设备侧**独立验证，不依赖观察者。

---

## 3. 验证（正向）

### 3.1 编译

```powershell
. 'C:\Espressif\tools\Microsoft.v5.4.4.PowerShell_profile.ps1'
cd <工程根>
idf.py set-target esp32s3
idf.py build
```

**实测结果**：`exit 0`

```
Project build complete.
esp32s3_hello.bin binary size 0x2b5d0 bytes. Smallest app partition is 0x100000 bytes. 0xd4a30 bytes (83%) free.
Bootloader binary size 0x51d0 bytes. 0x2e30 bytes (36%) free.
```

### 3.2 配置生效核对

```powershell
Select-String -Path sdkconfig -Pattern '^CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG|^CONFIG_ESPTOOLPY_FLASHSIZE='
```

**实测结果**：

```
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
```

### 3.3 烧录

```powershell
idf.py -p COM5 flash
```

**实测结果**：`exit 0`，三笔写入全部校验通过

```
Wrote 20944 bytes  (13318 compressed) at 0x00000000 ... Hash of data verified.
Wrote 177616 bytes (97359 compressed) at 0x00010000 ... Hash of data verified.
Wrote 3072 bytes   (103 compressed)   at 0x00008000 ... Hash of data verified.
```

### 3.4 启动日志

`idf.py -p COM5 monitor` 实测：

```
rst:0x15 (USB_UART_CHIP_RESET),boot:0x2b (SPI_FAST_FLASH_BOOT)
I (26) boot: ESP-IDF v5.4.4 2nd stage bootloader
I (27) boot: Multicore bootloader
I (27) boot: chip revision: v0.2
I (28) boot.esp32s3: SPI Flash Size : 16MB
I (72) boot: Loaded app from partition at offset 0x10000
I (82) cpu_start: cpu freq: 160000000 Hz
I (97) main_task: Calling app_main()
```

无 panic、无 brownout、无复位循环。

### 3.5 频率（双向独立时钟互证）

**设备侧**（`esp_timer` 自报）：

```
[     9] Hello world!  uptime=8007 ms
[    10] Hello world!  uptime=9007 ms
[    11] Hello world!  uptime=10007 ms
[    12] Hello world!  uptime=11007 ms
```

**宿主侧**（`System.Diagnostics.Stopwatch` 打点）：

```
  1062 ms | [    52] Hello world!  uptime=51007 ms
  2062 ms | [    53] Hello world!  uptime=52007 ms
  3062 ms | [    54] Hello world!  uptime=53007 ms
  4062 ms | [    55] Hello world!  uptime=54007 ms
```

**两个互不相关的时钟都严格 +1000 ms** ⇒ 判定：周期 1.000 Hz，无累积漂移。

---

## 4. 负向验证（证伪）

| 注入的故障 | 预期失效形态 | 是否实测 |
| --- | --- | --- |
| 去掉 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` | COM5 静默无输出（烧录仍成功） | 未实测（改动需重烧；判据来自 `Kconfig` 的实际选项语义，非推测） |
| `-p COM9`（不存在的口） | `could not open port`，`exit != 0` | 未实测 |
| `HELLO_PERIOD_MS = 0` | `pdMS_TO_TICKS(0)` = 0 tick，任务满速空转 | 未实测 |

> ⚠️ **诚实标注**：本节的故障注入**尚未执行**，属 D3 要求的负向用例但**未完成**。
> 已登记为 `GAP-001`（见 `docs/ledger.md`）。**不得**把「未实测」当「已验证」。

---

## 5. 已知局限

1. **PSRAM 未启用** —— 实测存在 8MB 内嵌 PSRAM，但 `CONFIG_SPIRAM` 未配置。
2. **验证由实现方自跑** —— 不满足 `AGENTS.md` §4.2「03 环必须独立于实现方」的要求。
3. **串口抓取有丢行** —— 宿主未持续读取时，USB-Serial/JTAG 会丢弃数据（实测两轮各丢约 8s / 32s 的行）。
   这不是固件问题（`uptime` 与计数器连续性可证），但会影响基于「行数」的判据。
   用 `idf.py monitor` 常驻读取不发生此现象。

---

## 6. 下游同步

| 对象 | 状态 |
| --- | --- |
| `README.md` | ✅ 已写（硬件表 / 环境 / 命令） |
| `docs/planning/baseline-20261027.md` | ✅ 已写 |
| `docs/ledger.md` | ✅ 已建，登记 `GAP-001` |
| `sdkconfig.defaults` | ✅ 已含两项关键配置 |
| 框架仓 `record.md` | ✅ 已留痕（框架侧运行态） |
