/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "led_io.h"

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
#include "led_strip.h"
#endif

static const char *TAG = "led_io";

#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)

#define LED_RGB_BRIGHTNESS 24

static led_strip_handle_t s_rgb_strip;
static SemaphoreHandle_t s_rgb_mutex;
static bool s_rgb_active[3];    // indexed by led_id_t

static void rgb_led_init(void)
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

    s_rgb_mutex = xSemaphoreCreateMutex();
    if (!s_rgb_mutex || led_strip_new_rmt_device(&strip_config, &rmt_config, &s_rgb_strip) != ESP_OK) {
        ESP_LOGW(TAG, "Could not initialize the RGB status LED");
        s_rgb_strip = NULL;
        return;
    }
    led_strip_clear(s_rgb_strip);
}

// Green = serial activity (either direction), blue = debug probe activity, red = error.
// The TX/RX distinction is left to the discrete LEDs (hardwired to the UART lines on
// boards like the ESP32-S3-USB-Bridge).
static void rgb_led_set(led_id_t id, bool active)
{
    if (!s_rgb_strip) {
        return;
    }

    xSemaphoreTake(s_rgb_mutex, portMAX_DELAY);
    s_rgb_active[id] = active;
    const uint8_t green = (s_rgb_active[LED_ID_TX] || s_rgb_active[LED_ID_RX]) ? LED_RGB_BRIGHTNESS : 0;
    const uint8_t blue = s_rgb_active[LED_ID_JTAG] ? LED_RGB_BRIGHTNESS : 0;
    led_strip_set_pixel(s_rgb_strip, 0, 0, green, blue);
    led_strip_refresh(s_rgb_strip);
    xSemaphoreGive(s_rgb_mutex);
}

static void rgb_led_error(bool on)
{
    if (!s_rgb_strip) {
        return;
    }

    xSemaphoreTake(s_rgb_mutex, portMAX_DELAY);
    led_strip_set_pixel(s_rgb_strip, 0, on ? LED_RGB_BRIGHTNESS : 0, 0, 0);
    led_strip_refresh(s_rgb_strip);
    xSemaphoreGive(s_rgb_mutex);
}

#endif // CONFIG_BRIDGE_GPIO_RGB_LED > -1

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
    rgb_led_init();
#endif

    ESP_LOGI(TAG, "LED init done");
}

void led_io_set(led_id_t id, bool active)
{
    gpio_led_set(id, active);
#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
    rgb_led_set(id, active);
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

    for (size_t i = 0; i < sizeof(led_patterns) / sizeof(led_patterns[0]); ++i) {
        gpio_led_set(LED_ID_TX, led_patterns[i][0]);
        gpio_led_set(LED_ID_RX, led_patterns[i][1]);
        gpio_led_set(LED_ID_JTAG, led_patterns[i][2]);
#if (CONFIG_BRIDGE_GPIO_RGB_LED > -1)
        rgb_led_error(led_patterns[i][0]);
#endif
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
