/*
 * SPDX-FileCopyrightText: 2021-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_system.h>
#include "util.h"
#include "led_io.h"

void __attribute__((noreturn)) eub_abort(void)
{
    led_io_signal_error();
    abort();
}
