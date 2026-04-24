/**
 * @file BasicEncoder.ino
 * @brief Example for encoding YUV frames to H.264 and streaming via UDP using ESP32S3-h264 library.
 *
 * This example demonstrates how to use the H264Encoder class to encode YUV video frames and send them over UDP.
 *
 * - Configure your WiFi SSID and password in the cfg struct.
 * - Set DEST_IP and DEST_PORT to the receiver's address.
 * - Compile with Partition Scheme Huge App and PSRAM activated
 * - The example uses a test YUV frame from test.h.
 * 
 * You can view the result with the help of ffmpeg on the receiving end:
 * ffplay -f h264 udp://@:5000
 * @note it will
 */
#include "H264Encoder.h"
#include "UDPPrint.h"
#include "test/test.h"  // Contains test_raw and test_raw_len

// Destination for UDP streaming (set to your receiver)
const char* DEST_IP = "192.168.1.44";
const uint16_t DEST_PORT = 5000;

H264Encoder encoder;
UDPPrint udp;

void setup() {
  Serial.begin(115200);
  delay(500);
  // configure streamer (populate global cfg)
  auto cfg = encoder.defaultConfig();
  cfg.ssid = "ssid";
  cfg.password = "pwd";
  cfg.width = 320;
  cfg.height = 240;
  cfg.fps = 15;
  cfg.gop = 100;
  cfg.qp_min = 30; // Minimum quantization parameter [0, 51]. (lower is better quality, but higher bitrate)
  cfg.qp_max = 40; // Maximum quantization parameter [0, 51]. (higher is worse quality
  if (!encoder.begin(cfg)) {
    Serial.println("Streamer failed to start");
    while (true) delay(1000);
  }
  if (!udp.begin(DEST_IP, DEST_PORT)){
    Serial.println("UDP failed to start");
    while (true) delay(1000);
  }
  
  Serial.println("Streamer started");
}

void loop() {
  encoder.encodeRGB565(test_raw, test_raw_len, udp);
}
