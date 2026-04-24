/**
 * @file BasicDecoder.ino
 * @brief Basic example demonstrating H.264 buffer decoding
 *
 * This example shows how to use H264Decoder to decode H.264 data
 * directly from a file. 
 *
 * Hardware Requirements:
 * - ESP32-S3 with sufficient PSRAM
 *
 * @author esp_h264 library
 */

#include "H264Decoder.h"
#include "SPIFFS.h"

// H.264 decoder without input stream
const char* filename = "/test_video.h264";  // H.264 file in SPIFFS
File file;
H264Decoder decoder(file);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Basic H.264 Decoder Example");
  Serial.println("==================================");

  // Configure decoder for I420 output
  auto config = decoder.defaultConfig();
  config.output_format = ESP_H264_RAW_FMT_I420;
  config.input_buffer_size = 100 * 1024;  // Smaller buffer for this example
  config.output_buffer_size = 320 * 240 * 3 / 2;  // QVGA I420 frame

  // Set frame callback for automatic processing
  config.frame_callback = [](const uint8_t* frame_data, uint32_t width,
                             uint32_t height, esp_h264_raw_format_t format) {
    Serial.printf("Frame decoded: %dx%d pixels, format=%d\n", width, height,
                  (int)format);

    // Example: Calculate average brightness
    if (format == ESP_H264_RAW_FMT_I420) {
      uint32_t totalLuminance = 0;
      const uint8_t* y_plane = frame_data;

      // Sum Y (luminance) values
      for (uint32_t i = 0; i < width * height; i++) {
        totalLuminance += y_plane[i];
      }

      uint8_t avgBrightness = totalLuminance / (width * height);
      Serial.printf("Average brightness: %d/255\n", avgBrightness);

      if (avgBrightness < 50) {
        Serial.println("Scene appears dark");
      } else if (avgBrightness > 200) {
        Serial.println("Scene appears bright");
      }
    }
  };

  // Initialize decoder
  if (decoder.begin(config)) {
    Serial.println("Decoder initialized successfully");
  } else {
    Serial.println("Failed to initialize decoder");
    while(true) delay(1000);
  }

  // open file
  file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file");
    while(true) delay(1000);
  }
}

void loop() {
  // In a real application, you might continuously decode frames here
  if (!decoder.processStream()) {
    Serial.println("End of file reached");
    file.close();
    while (1) delay(1000);
  }

  // Print periodic statistics
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {  // Every 10 seconds
    Serial.printf("Total frames decoded: %d\n", decoder.getFrameCount());
    Serial.printf("Decode errors: %d\n", decoder.getDecodeErrors());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    lastStats = millis();
  }
}

