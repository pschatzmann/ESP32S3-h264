/**
 * @file BasicEncoder.ino
 * @brief Example for encoding YUV frames to H.264 and streaming via UDP using ESP32S3-h264 library.
 *
 * This example demonstrates how to use the H264Encoder class to encode YUV video frames and send them over UDP.
 *
 * - Configure your WiFi SSID and password in the cfg struct.
 * - Set DEST_IP and DEST_PORT to the receiver's address.
 * - The example uses a test YUV frame from test.h.
 * 
 * You can view the result with the help of ffmpeg on the receiving end:
 * ffplay -f h264 udp://@:5000
 * @note it will
 */
#include "H264Encoder.h"
#include "UDPPrint.h"
#include "test.h"  // Contains test_yuv and test_yuv_size

// Destination for UDP streaming (set to your receiver)
const char* DEST_IP = "192.168.1.44";
const uint16_t DEST_PORT = 5000;

H264Encoder streamer;
UDPPrint udp;

void setup() {
  Serial.begin(115200);
  delay(500);
  // configure streamer (populate global cfg)
  auto cfg = streamer.defaultConfig();
  cfg.ssid = "ssid";
  cfg.password = "pwd";
  cfg.width = 640;
  cfg.height = 480;
  cfg.fps = 15;
  if (!streamer.begin(cfg)) {
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
  streamer.encode(test_yuv, test_yuv_len, udp);
}
