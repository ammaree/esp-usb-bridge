/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "led_io.h"

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
#include "led_strip.h"
#endif

static const char *TAG = "led_io";

static void gpio_led_set(led_id_t id, bool active)
{
    switch (id) {
    case LED_ID_TX:
#if (LED_TX > -1)
        gpio_set_level(LED_TX, active ? LED_TX_ON : LED_TX_OFF);
#endif
        break;
    case LED_ID_RX:
#if (LED_RX > -1)
        gpio_set_level(LED_RX, active ? LED_RX_ON : LED_RX_OFF);
#endif
        break;
    case LED_ID_JTAG:
#if (LED_JTAG > -1)
        gpio_set_level(LED_JTAG, active ? LED_JTAG_ON : LED_JTAG_OFF);
#endif
        break;
    default:
        break;
    }
}

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)

#define RGB_LVL         24          // status light, not illumination
#define TICK_MS         50
#define ACT_HOLD_TICKS  2           // stretch activity; raw pulses are far too short to see

static led_strip_handle_t s_strip;
static esp_timer_handle_t s_tick_timer;
static volatile uint32_t s_tick;
static volatile uint32_t s_serial_until;
static volatile uint32_t s_jtag_until;
static volatile bool s_boot_low;
static volatile bool s_rst_low;
static volatile bool s_flashing;
static volatile bool s_error;
static uint8_t s_last[3];

static void rgb_write(uint8_t r, uint8_t g, uint8_t b)
{
    if (r == s_last[0] && g == s_last[1] && b == s_last[2]) {
        return;                     // skip redundant RMT frames
    }
    s_last[0] = r;
    s_last[1] = g;
    s_last[2] = b;
    led_strip_set_pixel(s_strip, 0, r, g, b);
    led_strip_refresh(s_strip);
}

// Only this timer touches the strip, so no lock is needed. Highest priority state wins.
static void rgb_tick(void *arg)
{
    const uint32_t t = ++s_tick;
    const bool half = (t % 10) < 5;

    if (s_error) {
        rgb_write((t & 1) ? RGB_LVL : 0, 0, 0);                 // fast red blink
    } else if (s_rst_low) {
        rgb_write((t % 20) < 6 ? RGB_LVL : 0, 0, 0);            // slow red pulse: target held in reset
    } else if (s_boot_low) {
        rgb_write(RGB_LVL, RGB_LVL * 2 / 5, 0);                 // amber: target strapped for download
    } else if (s_flashing) {
        rgb_write(half ? RGB_LVL : 0, 0, half ? RGB_LVL : 0);   // magenta pulse: MSC/UF2 flashing
    } else if ((int32_t)(s_jtag_until - t) > 0) {
        rgb_write(0, 0, RGB_LVL);                               // blue: debug probe activity
    } else if ((int32_t)(s_serial_until - t) > 0) {
        rgb_write(0, RGB_LVL, 0);                               // green: serial activity
    } else {
        rgb_write(0, 0, 0);                                     // idle
    }
}

static void rgb_init(void)
{
    const led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BRIDGE_GPIO_RGB_LED,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
    };

    if (led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip) != ESP_OK) {
        ESP_LOGW(TAG, "RGB status LED init failed");
        s_strip = NULL;
        return;
    }
    led_strip_clear(s_strip);

    const esp_timer_create_args_t timer_args = {
        .callback = rgb_tick,
        .name = "led_tick",
    };
    if (esp_timer_create(&timer_args, &s_tick_timer) == ESP_OK) {
        esp_timer_start_periodic(s_tick_timer, TICK_MS * 1000);
    }
}

#endif // CONFIG_BRIDGE_GPIO_RGB_LED > -1

void led_io_init(void)
{
    gpio_config_t io_conf = { 0 };
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
#if (CONFIG_BRIDGE_GPIO_LED1 > -1)
    io_conf.pin_bit_mask |= 1ULL << CONFIG_BRIDGE_GPIO_LED1;
#endif
#if (CONFIG_BRIDGE_GPIO_LED2 > -1)
    io_conf.pin_bit_mask |= 1ULL << CONFIG_BRIDGE_GPIO_LED2;
#endif
#if (CONFIG_BRIDGE_GPIO_LED3 > -1)
    io_conf.pin_bit_mask |= 1ULL << CONFIG_BRIDGE_GPIO_LED3;
#endif
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    if (io_conf.pin_bit_mask) {
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

#if (CONFIG_BRIDGE_GPIO_LED1 > -1)
    gpio_set_level(CONFIG_BRIDGE_GPIO_LED1, !CONFIG_BRIDGE_GPIO_LED1_ACTIVE);
#endif
#if (CONFIG_BRIDGE_GPIO_LED2 > -1)
    gpio_set_level(CONFIG_BRIDGE_GPIO_LED2, !CONFIG_BRIDGE_GPIO_LED2_ACTIVE);
#endif
#if (CONFIG_BRIDGE_GPIO_LED3 > -1)
    gpio_set_level(CONFIG_BRIDGE_GPIO_LED3, !CONFIG_BRIDGE_GPIO_LED3_ACTIVE);
#endif

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    rgb_init();
#endif

    ESP_LOGI(TAG, "LED init done");
}

void led_io_set(led_id_t id, bool active)
{
    gpio_led_set(id, active);

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    if (active && s_strip) {
        if (id == LED_ID_JTAG) {
            s_jtag_until = s_tick + ACT_HOLD_TICKS;
        } else {
            s_serial_until = s_tick + ACT_HOLD_TICKS;
        }
    }
#endif
}

void led_io_set_target(bool boot, bool rst)
{
#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    s_boot_low = !boot;
    s_rst_low = !rst;
#else
    (void)boot;
    (void)rst;
#endif
}

void led_io_set_flashing(bool flashing)
{
#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    s_flashing = flashing;
#else
    (void)flashing;
#endif
}

void led_io_signal_error(void)
{
    const bool led_patterns[][3] = {
        {true,  true,  true},
        {true,  false, true},
        {false, true,  false},
        {true,  false, true},
        {false, true,  false},
        {true,  false, true},
        {true,  true,  true},
    };

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    s_error = true;
#endif

    for (size_t i = 0; i < sizeof(led_patterns) / sizeof(led_patterns[0]); ++i) {
        gpio_led_set(LED_ID_TX, led_patterns[i][0]);
        gpio_led_set(LED_ID_RX, led_patterns[i][1]);
        gpio_led_set(LED_ID_JTAG, led_patterns[i][2]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
