/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_intr_alloc_stub.h"

typedef intr_handle_t esp_h264_intr_hd_t;

/* In Arduino shim use a dummy source 0 and no-op free */
#define esp_h264_intr_alloc(flags, handler, arg, ret_handle)  esp_intr_alloc(0, flags, handler, arg, ret_handle)
#define esp_h264_intr_free(handler)                           (void)handler
