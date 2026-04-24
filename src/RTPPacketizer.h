#pragma once
#include <WiFiUdp.h>

#include <vector>

namespace esp_h264 {
/**
 * @brief RTPPacketizer class to packetize H.264 NAL units into RTP packets.
 * This class takes raw H.264 NAL units (in Annex-B format) and packetizes them
 * into RTP packets suitable for streaming over UDP.
 * 
 * This way you can e.g. Send H264 frames to QGroundcontrol
 */
class RTPPacketizer : public Print {
 public:
  static constexpr size_t MTU = 1200;
  static constexpr uint8_t RTP_VERSION = 2;
  static constexpr uint8_t RTP_PAYLOAD_TYPE = 96;

  RTPPacketizer(Print &out,
                uint32_t ssrc = 0x12345678)
      : out_(out), ssrc_(ssrc) {
        buffer_.reserve(MTU);
      }

  // Call once per encoded frame
  size_t write(const uint8_t* data, size_t len) {
    if (!data || len < 5) return 0;

    // Stable timestamp (better than millis jitter)
    timestamp_ += 3000;  // ~30 FPS (90000 / 30)

    // Parse Annex-B and extract ALL NAL units in the frame
    std::vector<std::pair<const uint8_t*, size_t>> nals;
    extractNALs(data, len, nals);
    if (nals.empty()) return 0;

    // Detect IDR
    bool is_idr = false;
    for (auto& n : nals) {
      uint8_t type = n.first[0] & 0x1F;
      if (type == 5) is_idr = true;
      if (type == 7) sps_.assign(n.first, n.first + n.second);
      if (type == 8) pps_.assign(n.first, n.first + n.second);
    }

    // Inject SPS/PPS before IDR
    std::vector<std::pair<const uint8_t*, size_t>> send_list;

    if (is_idr && !sps_.empty() && !pps_.empty()) {
      send_list.emplace_back(sps_.data(), sps_.size());
      send_list.emplace_back(pps_.data(), pps_.size());
    }

    for (auto& n : nals) {
      send_list.push_back(n);
    }

    // Send all NALs, only LAST packet gets marker bit
    for (size_t i = 0; i < send_list.size(); ++i) {
      bool last_nal = (i == send_list.size() - 1);
      sendNAL(send_list[i].first, send_list[i].second, last_nal);
    }
    return len;
  }

  virtual size_t write(uint8_t) {
    // Not used, we override writeFrame() instead
    ESP_LOGE("RTPPacketizer", "write(uint8_t) is not implemented, use write() instead");
    return 0;
  }


 private:
  Print &out_;
  std::vector<uint8_t> buffer_;

  uint16_t sequence_number_ = 0;
  uint32_t timestamp_ = 0;
  uint32_t ssrc_;

  std::vector<uint8_t> sps_;
  std::vector<uint8_t> pps_;

  // -------------------------
  // RTP send helpers
  // -------------------------

  void sendPacket(const uint8_t* payload, size_t payload_len, bool marker) {
    uint8_t rtp[12];
    buildHeader(rtp, marker);
    buffer_.clear();
    buffer_.insert(buffer_.end(), rtp, rtp + 12);
    buffer_.insert(buffer_.end(), payload, payload + payload_len);
    out_.write(buffer_.data(), buffer_.size());

    sequence_number_++;
  }

  void sendNAL(const uint8_t* data, size_t len, bool last_nal_of_frame) {
    if (len <= MTU - 12) {
      sendPacket(data, len, last_nal_of_frame);
      return;
    }

    // FU-A fragmentation
    uint8_t nal_header = data[0];
    uint8_t nal_type = nal_header & 0x1F;

    size_t payload_max = MTU - 12 - 2;
    size_t pos = 1;
    bool start = true;

    while (pos < len) {
      size_t frag_len = std::min(payload_max, len - pos);
      bool end = (pos + frag_len) >= len;

      uint8_t fua[2];
      fua[0] = (nal_header & 0xE0) | 28;
      fua[1] = (start ? 0x80 : 0x00) | (end ? 0x40 : 0x00) | nal_type;

      uint8_t buffer[MTU];
      memcpy(buffer, fua, 2);
      memcpy(buffer + 2, data + pos, frag_len);

      bool marker = last_nal_of_frame && end;
      sendPacket(buffer, frag_len + 2, marker);

      pos += frag_len;
      start = false;
    }
  }

  void buildHeader(uint8_t* hdr, bool marker) {
    hdr[0] = (RTP_VERSION << 6);
    hdr[1] = (marker ? 0x80 : 0x00) | (RTP_PAYLOAD_TYPE & 0x7F);

    hdr[2] = sequence_number_ >> 8;
    hdr[3] = sequence_number_ & 0xFF;

    hdr[4] = (timestamp_ >> 24) & 0xFF;
    hdr[5] = (timestamp_ >> 16) & 0xFF;
    hdr[6] = (timestamp_ >> 8) & 0xFF;
    hdr[7] = (timestamp_) & 0xFF;

    hdr[8] = (ssrc_ >> 24) & 0xFF;
    hdr[9] = (ssrc_ >> 16) & 0xFF;
    hdr[10] = (ssrc_ >> 8) & 0xFF;
    hdr[11] = (ssrc_) & 0xFF;
  }

  // -------------------------
  // Annex-B parser (multi-NAL safe)
  // -------------------------

  void extractNALs(const uint8_t* data, size_t len,
                   std::vector<std::pair<const uint8_t*, size_t>>& out) {
    size_t i = 0;

    while (i + 4 < len) {
      // find start code
      size_t start = i;
      while (start + 3 < len &&
             !(data[start] == 0 && data[start + 1] == 0 &&
               (data[start + 2] == 1 ||
                (data[start + 2] == 0 && data[start + 3] == 1)))) {
        start++;
      }

      if (start + 3 >= len) break;

      size_t sc_size = (data[start + 2] == 1) ? 3 : 4;
      size_t nal_start = start + sc_size;

      size_t next = nal_start;
      while (next + 3 < len &&
             !(data[next] == 0 && data[next + 1] == 0 &&
               (data[next + 2] == 1 ||
                (data[next + 2] == 0 && data[next + 3] == 1)))) {
        next++;
      }

      size_t nal_len = next - nal_start;
      if (nal_len > 0) {
        out.emplace_back(data + nal_start, nal_len);
      }

      i = next;
    }
  }
};

}  // namespace esp_h264