/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ESP32-S3 1.3 寸屏显示 Hello World
 *
 * 硬件: 正点原子 ATK-DNESP32S3 V1 (模组 ATK-MWS3S N16R8)
 *       + ATK-MD0130 1.3寸 240x240 SPI LCD 模块
 *
 * 引脚（取自官方例程 12_spilcd，非推断）:
 *   SPI2_SCLK = GPIO12   SPI2_MOSI = GPIO11   SPI2_MISO = GPIO13
 *   LCD_DC(WR) = GPIO40  LCD_CS    = GPIO21
 *   LCD_RST / LCD_PWR(背光) 经 XL9555 IO 扩展芯片，I2C0 SDA=GPIO41 SCL=GPIO42
 *
 * 改动依据: components/BSP 复制自官方 12_spilcd 例程，仅改
 *   lcd.c 中 SPI_LCD_TYPE(1→0) 与 lcd_display_dir() 的 240x240 分辨率。
 ****************************************************************************************************
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "iic.h"
#include "xl9555.h"
#include "spi.h"
#include "lcd.h"

/* 显示内容与配色
 * 说明: 本屏背光由 XL9555 的数字 IO 控制，只有开/关，无法用 PWM 调亮度。
 *       因此「调暗」只能靠选低亮度的颜色值（RGB565）。
 *       下面每个值都做了亮度压降，可在注释的 RGB8 附近自行微调。 */
#define BG_COLOR        0x1082      /* RGB565→RGB(16,16,16)   近黑中性深灰 */
#define TITLE_COLOR     0x3AD1      /* RGB565→RGB(57,89,140)  柔和蓝 */
#define BODY_COLOR      0x94B2      /* RGB565→RGB(148,150,148) 浅灰（原为纯白 0xFFFF） */
#define ACCENT_COLOR    0x9365      /* RGB565→RGB(148,109,41) 暗琥珀（原为满亮度红 0xF800） */

/* 刷新周期: 屏幕上计数器每 1 秒 +1，同时串口打印一行（保持可观测） */
#define TICK_MS         1000

static i2c_obj_t i2c0_master;

/**
 * @brief  初始化顺序严格照官方例程 main.c:50-54
 *         iic → spi2 → xl9555 → lcd
 *         xl9555 必须先于 lcd: 屏幕复位(RST)与背光(PWR)都挂在它上面。
 */
static void board_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c0_master = iic_init(I2C_NUM_0);  /* I2C0: SDA=GPIO41 SCL=GPIO42 */
    spi2_init();                        /* SPI2: SCLK=12 MOSI=11 MISO=13 */
    xl9555_init(i2c0_master);           /* IO 扩展: LCD_RST / LCD_BL */
    lcd_init();                          /* 含硬件复位 + 1.3寸初始化序列 + 开背光 */
}

void app_main(void)
{
    board_init();

    printf("\n=== ESP32-S3 1.3\" LCD Hello World ===\n");
    printf("panel: %ux%u, SPI2 @ 60MHz\n", lcd_self.width, lcd_self.height);

    /* 首次整屏绘制 */
    lcd_clear(BG_COLOR);
    lcd_show_string(20, 40, 200, 32, 32, "Hello", TITLE_COLOR);
    lcd_show_string(20, 80, 200, 32, 32, "World!", TITLE_COLOR);
    lcd_draw_hline(20, 125, 200, ACCENT_COLOR);
    lcd_show_string(20, 140, 200, 16, 16, "ESP32-S3", BODY_COLOR);
    lcd_show_string(20, 162, 200, 16, 16, "ATK-DNESP32S3", BODY_COLOR);

    uint32_t count = 0;
    char buf[24];
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        count++;

        /* 屏幕: 只重绘计数行，避免整屏闪烁 */
        snprintf(buf, sizeof(buf), "count: %lu", (unsigned long)count);
        lcd_fill(20, 195, 220, 215, BG_COLOR);          /* 先清除旧内容 */
        lcd_show_string(20, 195, 200, 16, 16, buf, ACCENT_COLOR);

        /* 串口: 同一计数器打到 COM5，便于脱离屏幕判断程序是否在跑 */
        printf("[%6lu] Hello world!  uptime=%lld ms\n",
               (unsigned long)count, esp_timer_get_time() / 1000);
        fflush(stdout);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TICK_MS));
    }
}
