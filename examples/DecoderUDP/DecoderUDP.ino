/**
 * @file DecoderUDP.ino
 * @brief Example sketch demonstrating H.264 stream decoding from UDP
 *
 * This example receives H.264 encoded video data via UDP and decodes it to
 * raw frames. The decoded frames can be processed for display or storage.
 *
 * Hardware Requirements:
 * - ESP32-S3 with sufficient PSRAM
 * - WiFi connection
 * - Optional: TFT display for frame visualization
 *
 * Usage:
 * 1. Configure WiFi credentials below
 * 2. Run CameraUDP.ino example on another ESP32 to send H.264 stream
 * 3. This decoder will receive and decode the stream
 *
 * @author esp_h264 library
 */

#include <WiFi.h>
#include <WiFiUdp.h>

#include "H264Decoder.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// UDP configuration
const int udpPort = 5000;
WiFiUDP udp;

// H.264 decoder with UDP input stream
H264Decoder<> decoder(&udp);

// Frame statistics
unsigned long lastStatsTime = 0;
uint32_t lastFrameCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 H.264 Decoder Example");
  Serial.println("============================");

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.printf("Connected! IP address: %s\n",
                WiFi.localIP().toString().c_str());

  // Start UDP listener
  if (udp.begin(udpPort)) {
    Serial.printf("UDP listener started on port %d\n", udpPort);
  } else {
    Serial.println("Failed to start UDP listener");
    return;
  }

  // Configure decoder
  auto config = decoder.defaultConfig();
  config.output_format = ESP_H264_RAW_FMT_RGB565_LE;  // For TFT displays
  config.input_buffer_size = 500 * 1024;      // Larger buffer for network data
  config.output_buffer_size = 640 * 480 * 2;  // RGB565 VGA frame
  config.max_read_bytes = 1500;               // UDP packet size limit

  // Optional: Set frame callback for real-time processing
  config.frame_callback = [](const uint8_t* frame_data, uint32_t width,
                             uint32_t height, esp_h264_raw_format_t format) {
    Serial.printf("Decoded frame: %dx%d, format=%d\n", width, height,
                  (int)format);
  };

  // Initialize decoder
  if (decoder.begin(config)) {
    Serial.println("Decoder initialized successfully");
  } else {
    Serial.println("Failed to initialize decoder");
    return;
  }

  Serial.println("Ready to receive H.264 stream...");
  lastStatsTime = millis();
}

void loop() {
  // Check for UDP packets
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    Serial.printf("Received UDP packet: %d bytes from %s:%d\n", packetSize,
                  udp.remoteIP().toString().c_str(), udp.remotePort());

    // Process H.264 stream data
    if (!decoder.processStream()) {
      Serial.println("Decoder processing failed");
    }
  }

  // Small delay to prevent watchdog issues
  delay(1);
}
