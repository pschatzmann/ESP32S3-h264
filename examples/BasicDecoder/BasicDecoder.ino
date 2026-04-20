/**
 * @file BasicDecoder.ino
 * @brief Basic example demonstrating H.264 buffer decoding
 * 
 * This example shows how to use H264Decoder to decode H.264 data
 * directly from buffers without using input streams. Useful for
 * file-based decoding or custom data sources.
 * 
 * Hardware Requirements:
 * - ESP32-S3 with sufficient PSRAM
 * 
 * @author esp_h264 library
 */

#include "H264Decoder.h"

// H.264 decoder without input stream
H264Decoder<> decoder;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 Basic H.264 Decoder Example");
  Serial.println("==================================");
  
  // Configure decoder for I420 output
  auto config = decoder.defaultConfig();
  config.output_format = ESP_H264_RAW_FMT_I420;
  config.input_buffer_size = 100 * 1024;   // Smaller buffer for this example
  config.output_buffer_size = 320 * 240 * 3 / 2;  // QVGA I420 frame
  
  // Set frame callback for automatic processing
  config.frame_callback = [](const uint8_t* frame_data, uint32_t width, uint32_t height, esp_h264_raw_format_t format) {
    Serial.printf("Frame decoded: %dx%d pixels, format=%d\n", width, height, (int)format);
    
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
    return;
  }
  
  // Example: Decode sample H.264 data
  // In a real application, you would read this from a file, network, etc.
  demonstrateDecoding();
  
  Serial.println("Example completed");
}

void loop() {
  // In a real application, you might continuously decode frames here
  delay(1000);
  
  // Print periodic statistics
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {  // Every 10 seconds
    Serial.printf("Total frames decoded: %d\n", decoder.getFrameCount());
    Serial.printf("Decode errors: %d\n", decoder.getDecodeErrors());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    lastStats = millis();
  }
}

void demonstrateDecoding() {
  Serial.println("Demonstrating H.264 decoding...");
  
  // Example: Simulate H.264 data chunks
  // In reality, you would read actual H.264 NAL units from your source
  
  // Example H.264 SPS (Sequence Parameter Set) NAL unit
  // This is just a placeholder - use real H.264 data in practice
  uint8_t sps_nal[] = {
    0x00, 0x00, 0x00, 0x01,  // Start code
    0x67, 0x42, 0x00, 0x1F,  // SPS NAL header + profile/level
    0x8B, 0x68, 0x05, 0x00,  // SPS data (example)
    0x5B, 0xA1, 0x00, 0x00,
    0x03, 0x00, 0x01, 0x00,
    0x00, 0x03, 0x00, 0x32,
    0x0F, 0x18, 0x31, 0x96
  };
  
  // Example H.264 PPS (Picture Parameter Set) NAL unit
  uint8_t pps_nal[] = {
    0x00, 0x00, 0x00, 0x01,  // Start code
    0x68, 0xCE, 0x3C, 0x80   // PPS NAL (example)
  };
  
  Serial.println("Sending SPS NAL unit...");
  if (decoder.decode(sps_nal, sizeof(sps_nal))) {
    Serial.println("SPS processed successfully");
  } else {
    Serial.println("SPS processing failed");
  }
  
  Serial.println("Sending PPS NAL unit...");
  if (decoder.decode(pps_nal, sizeof(pps_nal))) {
    Serial.println("PPS processed successfully");
  } else {
    Serial.println("PPS processing failed");
  }
  
  // Note: To decode actual frames, you would need real H.264 IDR and P-frame NAL units
  // The above examples only show parameter sets which don't produce frame output
  
  Serial.println("To see actual frame decoding, feed real H.264 stream data");
  Serial.println("from CameraUDP.ino or saved .h264 files");
}

// Example function to decode from a file (if SD card or SPIFFS is available)
void decodeFromFile(const char* filename) {
  // Example implementation for file-based decoding
  Serial.printf("Decoding H.264 file: %s\n", filename);
  
  // File file = SPIFFS.open(filename, "r");
  // if (!file) {
  //   Serial.println("Failed to open file");
  //   return;
  // }
  // 
  // uint8_t buffer[1024];
  // while (file.available()) {
  //   size_t bytesRead = file.readBytes((char*)buffer, sizeof(buffer));
  //   if (bytesRead > 0) {
  //     if (!decoder.decode(buffer, bytesRead)) {
  //       Serial.println("Decode error");
  //       break;
  //     }
  //   }
  // }
  // 
  // file.close();
  // Serial.printf("File decoding completed. Frames: %d\n", decoder.getFrameCount());
}

// Example function to process frames manually (without callback)
void manualFrameProcessing() {
  // Check for available frames
  while (decoder.hasFrame()) {
    // Allocate buffer for frame data
    uint8_t frameBuffer[320 * 240 * 3 / 2];  // QVGA I420
    uint32_t width, height;
    
    if (decoder.getFrame(frameBuffer, sizeof(frameBuffer), width, height)) {
      Serial.printf("Manual processing: %dx%d frame\n", width, height);
      
      // Process frame data as needed
      // saveToStorage(frameBuffer, width, height);
      // analyzeMotion(frameBuffer, width, height);
      // extractFeatures(frameBuffer, width, height);
    }
  }
}