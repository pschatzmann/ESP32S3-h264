/**
 * @file BasicEncoder.ino
 * @brief Example for encoding YUV frames to H.264 and streaming via UDP using ESP32S3-h264 library.
 *
 * This example demonstrates how to use the H264Encoder class to encode YUV video frames and send 
 * them over RDP using the RDPPacketizer class.
 *
 * - Configure your WiFi SSID and password in the cfg struct.
 * - Set DEST_IP and DEST_PORT to the receiver's address.
 * - Compile with Partition Scheme Huge App and PSRAM activated
 * - The example uses a test YUV frame from test.h.
 * 
 * You can view the result e.g. with the help of QGroundControl
 */
#include "H264Encoder.h"
#include "UDPPrint.h"
#include "RDPPacketizer.h"
#include "test/test.h"  // Contains test_raw and test_raw_len

// Destination for UDP streaming (set to your receiver)
const char* DEST_IP = "192.168.1.44";
const uint16_t DEST_PORT = 5000;

H264Encoder encoder;
UDPPrint udp;
RDPPacketizer rdp(udp);

void setup() {
  Serial.begin(115200);
  delay(500);
  // configure streamer (populate global cfg)
  auto cfg = encoder.defaultConfig();
  cfg.ssid = "ssid";
  cfg.password = "pwd";
  cfg.width = 640;
  cfg.height = 480;
  cfg.fps = 15;
  cfg.bitrate = 500000; // 500 kbps
  cfg.gop_size = 30; // Keyframe every 30 frames
  cfg.
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
  encoder.encodeRGB565(test_yuv, test_yuv_len, rdp);
}
