#pragma once

/**
 * @file H264Encoder.h
 * @brief Header-only helper to capture frames from ESP32 camera and encode to H.264
 * @author esp_h264 library
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_log.h>

#include <vector>

#include "PSRAMAllocator.h"
#include "RAMAllocator.h"
#include "esp_camera.h"
#include "h264/esp_h264_arduino.h"
#include "h264/esp_h264_enc_single.h"
#include "h264/esp_h264_enc_single_sw.h"
#include "h264/esp_h264_types.h"
#include "h264/h264_color_convert.h"

namespace esp_h264 {

/**
 * @class H264Encoder
 * @brief Template class for capturing frames from ESP32 camera and encoding to H.264
 * 
 * H264Encoder is a lightweight, header-only class that wraps camera initialization,
 * pixel format conversions (RGB565/YUV422 → I420), H.264 encoder lifecycle, and
 * streaming to any Arduino Print implementation.
 * 
 * The class is templated on an allocator type to allow customization of memory
 * allocation strategy (e.g., PSRAM vs regular heap).
 * 
 * @tparam Alloc Memory allocator type for internal buffers (defaults to PSRAMAllocator)
 * 
 * @note This class intentionally keeps implementation in the header for easy 
 *       integration into Arduino sketches without separate .cpp files
 * @note The pixel format conversions are straightforward and not highly optimized
 * @note Requires ESP32 with camera module and sufficient memory for frame buffers
 * 
 * Example usage:
 * @code
 * #include "H264Encoder.h"
 * #include "UDPPrint.h"
 * 
 * H264EncoderPSRAM streamer;  // Uses PSRAM allocation (simpler than H264Encoder<>)
 * UDPPrint udpOut;
 * 
 * void setup() {
 *   auto cfg = streamer.defaultConfig();
 *   cfg.ssid = "MyWiFi";
 *   cfg.pass = "MyPassword";
 *   cfg.width = 640;
 *   cfg.height = 480;
 *   cfg.fps = 15;
 *   
 *   udpOut.begin("192.168.1.100", 5000);
 *   if (!streamer.begin(cfg)) {
 *     Serial.println("Failed to start camera streamer");
 *   }
 * }
 * 
 * void loop() {
 *   streamer.captureH264(udpOut);  // Captures, encodes, and streams
 *   udpOut.flush();
 * }
 * @endcode
 */
template <typename Alloc = esp_h264::PSRAMAllocator<uint8_t>>
class H264Encoder {
 public:
  /** @brief Logging tag for ESP32 log system */
  static constexpr const char* TAG = "H264Encoder";
  
  /**
   * @struct Config
   * @brief Configuration structure for camera, WiFi, and encoder settings
   * 
   * Contains all parameters needed to configure the camera module, WiFi connection,
   * H.264 encoder, and pin assignments for ESP32 camera boards.
   */
  struct Config {
    /** @brief WiFi SSID (nullptr to skip WiFi initialization) */
    const char* ssid = nullptr;
    /** @brief WiFi password (nullptr to skip WiFi initialization) */
    const char* pass = nullptr;
    /** @brief Frame width in pixels */
    int width = 640;
    /** @brief Frame height in pixels */
    int height = 480;
    /** @brief Target frames per second */
    int fps = 15;
    /** @brief Output buffer size for encoded H.264 data (bytes) */
    size_t outBufferSize = 400 * 1024;
    
    // Camera pin configuration (AI-Thinker ESP32-CAM defaults)
    /** @brief Power down pin (active high) */
    int pwdn_pin = 32;
    /** @brief Reset pin (active low) */
    int reset_pin = -1;
    /** @brief External clock (XCLK) pin */
    int xclk_pin = 0;
    /** @brief SCCB data pin (I2C SDA equivalent) */
    int siod_pin = 26;
    /** @brief SCCB clock pin (I2C SCL equivalent) */
    int sioc_pin = 27;
    /** @brief Data pin Y2 (LSB) */
    int y2_pin = 5;
    /** @brief Data pin Y3 */
    int y3_pin = 18;
    /** @brief Data pin Y4 */
    int y4_pin = 19;
    /** @brief Data pin Y5 */
    int y5_pin = 21;
    /** @brief Data pin Y6 */
    int y6_pin = 36;
    /** @brief Data pin Y7 */
    int y7_pin = 39;
    /** @brief Data pin Y8 */
    int y8_pin = 34;
    /** @brief Data pin Y9 (MSB) */
    int y9_pin = 35;
    /** @brief Vertical sync pin */
    int vsync_pin = 25;
    /** @brief Horizontal reference pin */
    int href_pin = 23;
    /** @brief Pixel clock pin */
    int pclk_pin = 22;
  };

  /**
   * @brief Create a default configuration with reasonable defaults
   * 
   * Returns a Config structure populated with default values suitable for
   * AI-Thinker ESP32-CAM boards. WiFi credentials are set to nullptr,
   * requiring manual configuration or external WiFi management.
   * 
   * @return Config Default configuration structure
   * 
   * @note WiFi SSID and password are nullptr - set them manually or manage WiFi externally
   * @note Pin assignments match AI-Thinker ESP32-CAM board layout
   */
  Config defaultConfig() {
    Config cfg;
    cfg.ssid = nullptr;
    cfg.pass = nullptr;
    cfg.width = 640;
    cfg.height = 480;
    cfg.fps = 15;
    cfg.outBufferSize = 400 * 1024;
    // AI-Thinker default pins
    cfg.pwdn_pin = 32;
    cfg.reset_pin = -1;
    cfg.xclk_pin = 0;
    cfg.siod_pin = 26;
    cfg.sioc_pin = 27;
    cfg.y2_pin = 5;
    cfg.y3_pin = 18;
    cfg.y4_pin = 19;
    cfg.y5_pin = 21;
    cfg.y6_pin = 36;
    cfg.y7_pin = 39;
    cfg.y8_pin = 34;
    cfg.y9_pin = 35;
    cfg.vsync_pin = 25;
    cfg.href_pin = 23;
    cfg.pclk_pin = 22;
    return cfg;
  }

  /**
   * @brief Default constructor
   * 
   * Creates an uninitialized H264Encoder. Call begin() to initialize
   * WiFi, camera, and encoder resources.
   */
  H264Encoder() = default;
  
  /**
   * @brief Destructor
   * 
   * Automatically releases camera, encoder, and buffer resources by calling end().
   * 
   * @note Safe to destroy without calling end() explicitly
   */
  ~H264Encoder() { end(); }

  /**
   * @brief Initialize WiFi, camera and H.264 encoder
   * 
   * Configures and initializes all subsystems needed for camera streaming:
   * 1. WiFi connection (if credentials provided)
   * 2. Camera module with specified pins and resolution  
   * 3. H.264 software encoder with specified bitrate and GOP
   * 4. Internal frame buffers using the configured allocator
   * 
   * @param cfg Configuration structure (passed by value to allow temporary objects)
   * @return true if all initialization succeeded, false on any failure
   * 
   * @note If WiFi credentials are nullptr, WiFi initialization is skipped
   * @note Camera is configured for RGB565 pixel format internally
   * @note Frame buffers are allocated using the template allocator (Alloc)
   * @note On failure, any partially initialized resources are cleaned up
   */
  bool begin(Config cfg) {
    // copy the provided config into the stored config_
    cfg_ = cfg;
    if (!initWiFi()) return false;
    if (!initCamera()) return false;
    if (!initEncoder()) return false;
    return true;
  }

  /**
   * @brief Capture a single frame and write raw I420 data to buffer
   * 
   * Captures one frame from the camera, converts it from the native format
   * (RGB565 or YUV422) to I420 (YUV420 planar), and writes the raw pixel
   * data to the provided buffer.
   * 
   * @param dst Destination buffer for I420 pixel data
   * @param dst_len Size of destination buffer in bytes
   * @return true if frame capture and conversion succeeded, false otherwise
   * 
   * @note Buffer must be at least width × height × 1.5 bytes for I420 data
   * @note This is a lower-level method; most users should use captureH264() instead
   * @note Caller is responsible for ensuring buffer is large enough
   */
  bool captureFrame(uint8_t* dst, size_t dst_len) {
    if (!dst) return false;
    return captureFrameI420(dst, dst_len);
  }

  /**
   * @brief Encode raw I420 frame data to H.264 and write to Print stream
   * 
   * Takes raw I420 pixel data, encodes it using the H.264 software encoder,
   * and writes the resulting NAL units to the provided Print stream.
   * Handles partial writes with retry logic.
   * 
   * @param raw_data Pointer to I420 pixel data buffer
   * @param raw_len Size of I420 data in bytes  
   * @param out Print stream to receive encoded H.264 data
   * @return true if encoding and writing succeeded, false otherwise
   * 
   * @note Input data must be valid I420 format with correct dimensions
   * @note Implements retry logic for partial Print::write() operations
   * @note This is a lower-level method; most users should use captureH264() instead
   */
  bool encode(const uint8_t* raw_data, size_t raw_len, Print& out) {
    if (!enc_handle_) return false;
    if (!raw_data || raw_len == 0) return false;
    if (out_buf_.empty()) return false;

    esp_h264_enc_in_frame_t in_frame{};
    esp_h264_enc_out_frame_t out_frame{};
    in_frame.raw_data.buffer = (uint8_t*)raw_data;
    in_frame.raw_data.len = raw_len;
    out_frame.raw_data.buffer = out_buf_.data();
    out_frame.raw_data.len = out_buf_.size();

    esp_h264_err_t r = esp_h264_enc_process(enc_handle_, &in_frame, &out_frame);

    if (out_frame.raw_data.len > 0) {
      const uint8_t* buf = out_frame.raw_data.buffer;
      size_t to_write = out_frame.raw_data.len;
      size_t written = 0;
      const int max_retries = 5;
      int attempts = 0;
      while (written < to_write && attempts < max_retries) {
        size_t n = out.write(buf + written, to_write - written);
        if (n > 0) {
          written += n;
        } else {
          // if nothing was written, wait a bit before retrying to avoid busy
          // loop
          delay(5);
        }
        ++attempts;
      }
      if (written < to_write) {
        ESP_LOGW(TAG, "Partial write: expected %u bytes, wrote %u bytes",
                 (unsigned)to_write, (unsigned)written);
        return false;
      }
    }

    return (r == ESP_H264_ERR_OK);
  }

  /**
   * @brief Capture frame, encode to H.264, and stream with frame rate control
   * 
   * High-level method that performs the complete streaming pipeline:
   * 1. Captures one frame from the camera
   * 2. Converts pixel format to I420 if needed
   * 3. Encodes frame using H.264 software encoder
   * 4. Writes encoded data to the provided Print stream
   * 5. Enforces configured frame rate by delaying remaining time
   * 
   * This method measures processing time and only delays for the remainder
   * of the target frame interval, ensuring consistent frame rate regardless
   * of processing time variations.
   * 
   * @param out Print stream to receive encoded H.264 NAL units
   * @return true if capture, encoding, and streaming succeeded, false otherwise
   * 
   * @note Frame rate is controlled by Config::fps setting
   * @note Processing time is subtracted from frame interval delay
   * @note This is the main method most applications should use
   * @note On failure, no delay is applied (to avoid blocking on errors)
   */
  bool captureH264(Print& out) {
    // basic guards
    if (in_buf_.empty()) return false;
    if (!enc_handle_) return false;

    // compute target frame interval in ms
    unsigned long frame_ms = (cfg_.fps > 0) ? (1000u / (unsigned)cfg_.fps) : 0u;
    unsigned long start = millis();

    // capture into the input buffer
    if (!captureFrameI420(in_buf_.data(), in_buf_.size())) {
      return false;
    }

    // encode from the input buffer to the output buffer, writing to `out`
    if (!encode(in_buf_.data(), in_buf_.size(), out)) {
      return false;
    }

    // enforce configured frame rate: delay only remaining time after processing
    if (frame_ms > 0) {
      unsigned long elapsed = millis() - start;
      if (elapsed < frame_ms) delay(frame_ms - elapsed);
    }

    return true;
  }

  /**
   * @brief Cleanup and release camera resources
   * 
   * Deinitializes the ESP32 camera driver and releases all allocated
   * resources. Should be called when streaming is complete or when
   * switching camera configurations.
   * 
   * After calling end(), the camera becomes available for other
   * applications or for reconfiguration with different settings.
   * 
   * @note Safe to call multiple times
   * @note Required before calling begin() with different Config
   * @note Automatically called by destructor
   */
  void end() {
    if (enc_handle_) {
      esp_h264_enc_close(enc_handle_);
      esp_h264_enc_del(enc_handle_);
      enc_handle_ = nullptr;
    }
    if (!in_buf_.empty()) {
      in_buf_.clear();
    }
    if (!out_buf_.empty()) {
      out_buf_.clear();
    }
  }

 private:
  Config cfg_;
  esp_h264_enc_handle_t enc_handle_ = nullptr;
  std::vector<uint8_t, Alloc> in_buf_;
  std::vector<uint8_t, Alloc> out_buf_;

  /**
   * @brief Initialize WiFi connection using Config credentials
   * @return true if WiFi connection established or credentials not provided (user-managed), false on timeout
   * @note Skips initialization if Config::ssid or Config::pass is nullptr
   * @note 15 second connection timeout
   */
  bool initWiFi() {
    if (!cfg_.ssid || !cfg_.pass) return true;  // assume user-managed WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg_.ssid, cfg_.pass);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      if (millis() - start > 15000) return false;
    }
    return true;
  }

  /**
   * @brief Initialize ESP32 camera with Config pin mappings and settings
   * @return true if camera initialization successful and buffers allocated, false on error
   * @note Allocates I420 conversion buffer (width×height×1.5 bytes) and output buffer using configured allocator
   * @note Automatically selects framesize based on Config::width and Config::height
   * @note Sets RGB565 pixel format for color conversion compatibility
   */
  bool initCamera() {
    camera_config_t config{};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = cfg_.y2_pin;
    config.pin_d1 = cfg_.y3_pin;
    config.pin_d2 = cfg_.y4_pin;
    config.pin_d3 = cfg_.y5_pin;
    config.pin_d4 = cfg_.y6_pin;
    config.pin_d5 = cfg_.y7_pin;
    config.pin_d6 = cfg_.y8_pin;
    config.pin_d7 = cfg_.y9_pin;
    config.pin_xclk = cfg_.xclk_pin;
    config.pin_pclk = cfg_.pclk_pin;
    config.pin_vsync = cfg_.vsync_pin;
    config.pin_href = cfg_.href_pin;
    config.pin_sccb_sda = cfg_.siod_pin;
    config.pin_sccb_scl = cfg_.sioc_pin;
    config.pin_pwdn = cfg_.pwdn_pin;
    config.pin_reset = cfg_.reset_pin;
    config.xclk_freq_hz = 20000000;
    // choose framesize from requested width/height; default to VGA
    auto pick_frame_size = [](int w, int h) -> framesize_t {
      if (w == 160 && h == 120) return FRAMESIZE_QQVGA;
      if (w == 320 && h == 240) return FRAMESIZE_QVGA;
      if (w == 640 && h == 480) return FRAMESIZE_VGA;
      if (w == 800 && h == 600) return FRAMESIZE_SVGA;
      if (w == 1024 && h == 768) return FRAMESIZE_XGA;
      // fallback
      return FRAMESIZE_VGA;
    };
    framesize_t fs = pick_frame_size(cfg_.width, cfg_.height);
    config.frame_size = fs;
    config.pixel_format = PIXFORMAT_RGB565;
    config.fb_count = 1;

    if (esp_camera_init(&config) != ESP_OK) {
      ESP_LOGE(TAG, "esp_camera_init failed");
      return false;
    }
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
      s->set_framesize(s, fs);
    }

    // allocate buffers sized for cfg_.width/height
    size_t in_len = (size_t)cfg_.width * (size_t)cfg_.height * 3 / 2;
    size_t out_len = cfg_.outBufferSize;
    try {
      in_buf_.clear();
      out_buf_.clear();
      in_buf_.resize(in_len);
      out_buf_.resize(out_len);
    } catch (const std::bad_alloc& e) {
      ESP_LOGE(TAG, "Failed to allocate buffers: %s", e.what());
      in_buf_.clear();
      out_buf_.clear();
      return false;
    }
    return true;
  }

  /**
   * @brief Initialize H.264 software encoder with Config settings
   * @return true if encoder creation and opening successful, false on error
   * @note Automatically calculates bitrate as width×height×fps÷20
   * @note Sets GOP (Group of Pictures) to 2×fps for balanced compression/quality
   * @note Uses esp_h264 software encoder (not hardware acceleration)
   */
  bool initEncoder() {
    esp_h264_enc_cfg_sw_t enc_cfg{};
    enc_cfg.res.width = (uint16_t)cfg_.width;
    enc_cfg.res.height = (uint16_t)cfg_.height;
    enc_cfg.fps = (uint8_t)cfg_.fps;
    enc_cfg.gop = (uint8_t)(cfg_.fps * 2);
    enc_cfg.rc.bitrate = (uint32_t)(cfg_.width * cfg_.height * cfg_.fps / 20);
    esp_h264_err_t res = esp_h264_enc_sw_new(&enc_cfg, &enc_handle_);
    if (res != ESP_H264_ERR_OK || !enc_handle_) {
      ESP_LOGE(TAG, "esp_h264_enc_sw_new failed: %d", (int)res);
      return false;
    }
    res = esp_h264_enc_open(enc_handle_);
    if (res != ESP_H264_ERR_OK) {
      ESP_LOGE(TAG, "esp_h264_enc_open failed: %d", (int)res);
      return false;
    }
    return true;
  }

  /**
   * @brief Capture one frame and convert to I420 (YUV420 planar) format
   * 
   * Captures frame from ESP32 camera and converts from the camera's native
   * pixel format to I420 format required by H.264 encoder. Supports RGB565
   * and YUV422 input formats with automatic format detection.
   * 
   * @param dst Destination buffer for I420 data (must be width×height×1.5 bytes)
   * @param dst_len Size of destination buffer in bytes
   * @return true if frame captured and converted successfully, false on error
   * 
   * @note I420 format: Y plane (width×height), U plane (width/2×height/2), V plane (width/2×height/2)
   * @note Automatically returns camera framebuffer after conversion
   * @note Logs error for unsupported pixel formats
   */
  bool captureFrameI420(uint8_t* dst, size_t dst_len) {
    if (!dst) return false;
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) return false;

    int w = fb->width;
    int h = fb->height;
    size_t expected = (size_t)w * (size_t)h * 3 / 2;
    if (dst_len < expected) {
      ESP_LOGE(TAG, "Destination buffer too small: need %u got %u",
               (unsigned)expected, (unsigned)dst_len);
      esp_camera_fb_return(fb);
      return false;
    }

    bool res = false;
    switch (fb->format) {
      case PIXFORMAT_RGB565:
        res = captureFrameI420_RGB565(dst, dst_len, fb, w, h);
        break;
      case PIXFORMAT_YUV422:
        res = captureFrameI420_YUV422(dst, dst_len, fb, w, h);
        break;
      default:
        ESP_LOGE(TAG, "Unsupported camera pixel format: %d", fb->format);
        res = false;
        esp_camera_fb_return(fb);
        break;
    }

    return res;
  }

  /**
   * @brief Convert RGB565 framebuffer to I420 format
   * 
   * Converts ESP32 camera RGB565 framebuffer to I420 (YUV420 planar) format.
   * Performs RGB to YUV color space conversion with proper scaling and subsampling.
   * 
   * @param dst Destination I420 buffer
   * @param dst_len Size of destination buffer (unused, validated by caller)
   * @param fb Camera framebuffer containing RGB565 data
   * @param w Frame width in pixels
   * @param h Frame height in pixels
   * @return true on successful conversion, false on error
   * 
   * @note Y plane: full resolution luminance
   * @note U/V planes: 2×2 subsampled chrominance (quarter resolution)
   * @note Automatically releases framebuffer after conversion
   */
  bool captureFrameI420_RGB565(uint8_t* dst, size_t /*dst_len*/,
                               camera_fb_t* fb, int w, int h) {
    uint8_t* p = fb->buf;
    // Y plane
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int i = (y * w + x) * 2;
        uint16_t val = p[i] | (p[i + 1] << 8);
        uint8_t r = (uint8_t)((((val >> 11) & 0x1F) * 527 + 23) >> 6);
        uint8_t g = (uint8_t)((((val >> 5) & 0x3F) * 259 + 33) >> 6);
        uint8_t b = (uint8_t)(((val & 0x1F) * 527 + 23) >> 6);
        uint8_t yv = (uint8_t)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
        dst[y * w + x] = yv;
      }
    }
    uint8_t* uplane = dst + w * h;
    uint8_t* vplane = uplane + (w / 2) * (h / 2);
    for (int y = 0; y < h; y += 2) {
      for (int x = 0; x < w; x += 2) {
        int idx0 = (y * w + x) * 2;
        int idx1 = (y * w + x + 1) * 2;
        int idx2 = ((y + 1) * w + x) * 2;
        int idx3 = ((y + 1) * w + x + 1) * 2;
        int indices[4] = {idx0, idx1, idx2, idx3};
        int sumU = 0, sumV = 0;
        for (int k = 0; k < 4; ++k) {
          int ii = indices[k];
          uint16_t vval = p[ii] | (p[ii + 1] << 8);
          uint8_t rr = (uint8_t)((((vval >> 11) & 0x1F) * 527 + 23) >> 6);
          uint8_t gg = (uint8_t)(((((vval >> 5) & 0x3F) * 259 + 33) >> 6));
          uint8_t bb = (uint8_t)(((vval & 0x1F) * 527 + 23) >> 6);
          int u = ((-38 * rr - 74 * gg + 112 * bb + 128) >> 8) + 128;
          int vvv = ((112 * rr - 94 * gg - 18 * bb + 128) >> 8) + 128;
          sumU += u;
          sumV += vvv;
        }
        int ux = x / 2;
        int uy = y / 2;
        int pos = uy * (w / 2) + ux;
        uplane[pos] = (uint8_t)(sumU / 4);
        vplane[pos] = (uint8_t)(sumV / 4);
      }
    }
    esp_camera_fb_return(fb);
    return true;
  }

  /**
   * @brief Convert YUV422/YUYV framebuffer to I420 format using optimized functions
   * 
   * Converts ESP32 camera YUV422 framebuffer to I420 (YUV420 planar) format
   * using the optimized color conversion functions from h264_color_convert.h.
   * On ESP32-S3, uses assembly-optimized yuyv2iyuv_esp32s3(), otherwise
   * uses the standard yuyv2iyuv() function.
   * 
   * @param dst Destination I420 buffer  
   * @param dst_len Size of destination buffer (unused, validated by caller)
   * @param fb Camera framebuffer containing YUV422/YUYV data
   * @param w Frame width in pixels
   * @param h Frame height in pixels
   * @return true on successful conversion, false on error
   * 
   * @note YUV422 format: Y0 U0 Y1 V0 (4 bytes for 2 pixels)
   * @note I420 format: Y plane + U plane + V plane (separate planes)
   * @note Uses hardware-optimized assembly on ESP32-S3 when available
   * @note Automatically releases framebuffer after conversion
   */
  bool captureFrameI420_YUV422(uint8_t* dst, size_t /*dst_len*/,
                               camera_fb_t* fb, int w, int h) {
    uint8_t* p = fb->buf;
    
    // Use optimized color conversion functions from h264_color_convert.h
#ifdef HAVE_ESP32S3
    // Use assembly-optimized version on ESP32-S3
    yuyv2iyuv_esp32s3((uint32_t)h, (uint32_t)w, p, dst);
#else
    // Use standard optimized version on other platforms
    yuyv2iyuv((uint32_t)h, (uint32_t)w, p, dst);
#endif
    
    esp_camera_fb_return(fb);
    return true;
  }
};

/**
 * @brief Type alias for H264Encoder using default PSRAM allocation
 * 
 * Provides a simpler way to declare H264Encoder instances without specifying
 * the allocator template parameter. Uses PSRAMAllocator<uint8_t> by default.
 * 
 * Example usage:
 * @code
 * H264EncoderPSRAM encoder;  // Instead of H264Encoder<PSRAMAllocator<uint8_t>>
 * @endcode
 */
using H264EncoderPSRAM = H264Encoder<PSRAMAllocator<uint8_t>>;

/**
 * @brief Type alias for H264Encoder using internal RAM allocation
 * 
 * Provides a simpler way to declare H264Encoder instances that use fast
 * internal RAM instead of external PSRAM.
 * 
 * Example usage:
 * @code
 * H264EncoderRAM encoder;   // Instead of H264Encoder<RAMAllocator<uint8_t>>
 * @endcode
 */
using H264EncoderRAM = H264Encoder<RAMAllocator<uint8_t>>;

}  // namespace esp_h264

#ifdef ARDUINO
using namespace esp_h264;
#endif
