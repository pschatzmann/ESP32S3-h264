/* Public Arduino shim to map ESP-IDF headers to Arduino-friendly stubs.
 * Include this at top of your Arduino sketch or before including esp_h264 headers.
 */
#pragma once

#include "esp_h264_version.h"
#include "esp_h264_enc_single_sw.h"
#include "esp_h264_dec.h"