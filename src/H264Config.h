#pragma once
#include "Arduino.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#  define HAVE_ESP32S3
#else
#  error "This library is only compatible with ESP32-S3. Please check your board configuration."
#endif

#ifndef H264_DEFAULT_ALLOCATOR
#define H264_DEFAULT_ALLOCATOR esp_h264::RAMAllocatorH264<uint8_t>
#endif