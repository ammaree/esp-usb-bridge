/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

/* LEDs */
#define LED_TX          CONFIG_BRIDGE_GPIO_LED1
#define LED_RX          CONFIG_BRIDGE_GPIO_LED2
#define LED_JTAG        CONFIG_BRIDGE_GPIO_LED3

#define LED_TX_ON       CONFIG_BRIDGE_GPIO_LED1_ACTIVE
#define LED_TX_OFF      (!CONFIG_BRIDGE_GPIO_LED1_ACTIVE)

#define LED_RX_ON       CONFIG_BRIDGE_GPIO_LED2_ACTIVE
#define LED_RX_OFF      (!CONFIG_BRIDGE_GPIO_LED2_ACTIVE)

#define LED_JTAG_ON     CONFIG_BRIDGE_GPIO_LED3_ACTIVE
#define LED_JTAG_OFF    (!CONFIG_BRIDGE_GPIO_LED3_ACTIVE)

typedef enum {
    LED_ID_TX,      // serial activity toward the host (LED1)
    LED_ID_RX,      // serial activity toward the target (LED2)
    LED_ID_JTAG,    // debug probe activity (LED3)
} led_id_t;

void led_io_init(void);
void led_io_set(led_id_t id, bool active);
void led_io_signal_error(void);   // blocking error blink pattern (~3.5 s)
