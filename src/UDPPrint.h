#pragma once

/**
 * @file UDPPrint.h -
 * @brief header-only Arduino Print implementation that sends data over
 */

#include <Arduino.h>
#include <WiFiUdp.h>
#include <WiFi.h>

namespace esp_h264 {

/**
 * @brief A Print implementation that buffers and sends data over UDP
 *
 * UDPPrint extends Arduino's Print class to send data over UDP networks.
 * It internally buffers data and sends UDP packets when the buffer fills
 * or when explicitly flushed.
 *
 * @note WiFi connection must be established before using this class
 * @note Data is buffered up to BUF_SIZE (1400 bytes) before automatic
 * transmission
 *
 * Example usage:
 * @code
 * #include "UDPPrint.h"
 *
 * UDPPrint up;
 * up.begin("192.168.1.100", 5000);
 * up.print("Hello World");
 * up.flush();  // Send immediately
 * @endcode
 */
class UDPPrint : public Print {
 public:
  UDPPrint() : udp_(new WiFiUDP()), destPort_(0), bufPos_(0) {}
  ~UDPPrint() { delete udp_; }

  bool initWiFi(const char* ssid, const char* password) {
    ESP_LOGD(TAG, "initWiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      if (millis() - start > 15000) return false;
      Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  // Begin with numeric IP string
  bool begin(const char* destIp, uint16_t destPort) {
    destIp_ = String(destIp);
    destPort_ = destPort;
    // Don't call udp_->begin() to bind to a local port; beginPacket will handle
    // it
    bufPos_ = 0;
    return true;
  }

  // Begin with IPAddress
  bool begin(const IPAddress& addr, uint16_t destPort) {
    destIp_ = addr.toString();
    destPort_ = destPort;
    bufPos_ = 0;
    return true;
  }

  // Write a single byte
  size_t write(uint8_t ch) override {
    buffer_[bufPos_++] = ch;
    if (bufPos_ >= sizeof(buffer_)) flush();
    return 1;
  }

  // Write a buffer
  size_t write(const uint8_t* data, size_t size) override {
    size_t written = 0;
    while (size > 0) {
      size_t space = sizeof(buffer_) - bufPos_;
      if (space == 0) flush();
      size_t tocopy = (size < space) ? size : space;
      memcpy(buffer_ + bufPos_, data + written, tocopy);
      bufPos_ += tocopy;
      written += tocopy;
      size -= tocopy;
    }
    flush();
    ESP_LOGI(TAG, "Buffered and sent %u bytes to %s:%u",
             (unsigned)written, destIp_.c_str(), destPort_);
    return written;
  }

  // Send buffered data immediately
  void flush() {
    if (bufPos_ == 0) return;
    if (!udp_) return;

    size_t to_send = bufPos_;

    bool sent = false;
    size_t written = 0;
    int retry = 10;
    while (!sent && retry-- > 0) {
      if (!udp_->beginPacket(destIp_.c_str(), destPort_)) {
        // unable to start packet
        bufPos_ = 0;
        return;
      }
      written = udp_->write(buffer_, bufPos_);
      sent = udp_->endPacket();
      delay(write_delay_ms);
    }

    if (!sent) {
      ESP_LOGE(TAG,
               "Failed to send UDP packet %u bytes to %s:%u after retries",
               (unsigned)to_send, destIp_.c_str(), destPort_);
    }
    bufPos_ = 0;
  }

  // Convenience: send immediate string
  size_t print(const char* s) {
    size_t len = write((const uint8_t*)s, strlen(s));
    flush();
    return len;
  }

  size_t println(const char* s) {
    char buffer[strlen(s) + 2]{};
    strcpy(buffer, s);
    strcat(buffer, "\n");
    return print(buffer);
  }

  /// Set delay after sending a UDP packet (default 100ms)
  void setWriteDelay(unsigned long ms) { write_delay_ms = ms; }

 private:
  WiFiUDP* udp_;
  String destIp_;
  uint16_t destPort_;
  const char* TAG = "UDPPrint";
  static constexpr size_t BUF_SIZE = 1200;
  uint8_t buffer_[BUF_SIZE];
  size_t bufPos_;
  size_t write_delay_ms = 20;
};

}  // namespace esp_h264