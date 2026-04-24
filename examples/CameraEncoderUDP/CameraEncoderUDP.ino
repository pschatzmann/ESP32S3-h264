/*
  CameraUDP.ino

  Example: Capture frames from an ESP32 camera module, encode them with the
  library's software H.264 encoder (openh264 backend) and stream NAL units over
  UDP.

  Notes:
  - This sketch uses the library's SW encoder API:
      esp_h264_enc_sw_new, esp_h264_enc_open, esp_h264_enc_process,
  esp_h264_enc_close, esp_h264_enc_del
  - It assumes the camera is configured to return I420 (YUV420 planar) or
  YUYV/YUV formats.
  - On many ESP32-CAM modules the built-in camera driver returns JPEG; you will
  need to change the camera configuration to provide raw YUV frames or convert
  JPEG to YUV before encoding.
  - Adjust WIFI_SSID/WIFI_PASS and DEST_IP/DEST_PORT below. Note: the
    example sets the UDP destination using the `UDPPrint` instance (see
    `DEST_IP` / `DEST_PORT` constants). `H264Encoder` does not hold
    or manage the destination — it is transport-agnostic and writes encoded
    bytes to any `Print` implementation you provide.

  Minimal flow:
   - init camera
   - create/sw-encoder instance (configure width/height/fps)
   - for each frame: copy/convert raw pixels into `in_frame.raw_data.buffer`,
  call esp_h264_enc_process
   - send encoded output via UDP

  This example focuses on the encoder plumbing and UDP streaming. It is a
  starting point and should be adapted for camera driver specifics and error
  handling for production use.
*/

#include "H264Encoder.h"
#include "UDPPrint.h"

// Destination for UDP streaming (set to your receiver)
const char* DEST_IP = "192.168.1.100";
const uint16_t DEST_PORT = 5000;

H264Encoder encoder;
UDPPrint udp;


void setup() {
  Serial.begin(115200);
  delay(500);
  // configure streamer (populate global cfg)
  auto cfg = encoder.defaultConfig();
  cfg.ssid = "YourSSID";
  cfg.password = "YourPassword";
  cfg.width = 640;
  cfg.height = 480;
  cfg.fps = 15;
  cfg.use_camera = true;  // Enable camera initialization
  if (!encoder.begin(cfg)) {
    Serial.println("Streamer failed to start");
    while (1) delay(1000);
  }
  udp.begin(DEST_IP, DEST_PORT);
  Serial.println("Streamer started");
}

void loop() {
  encoder.captureH264(udp);
}
