#pragma once

/**
 * @file PSRAMAllocatorH264.h
 * @brief Header-only C++ allocator that prefers PSRAM allocation on ESP32
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <vector>

#include "H264Config.h"
#include "h264/esp_h264_alloc.h"

namespace esp_h264 {

/**
 * @class PSRAMAllocatorH264
 * @brief STL-compatible allocator that prefers PSRAM allocation with fallback
 *
 * PSRAMAllocatorH264 is a custom C++ allocator that attempts to allocate memory in
 * external PSRAM (using heap_caps_aligned_calloc with MALLOC_CAP_SPIRAM) and
 * falls back to regular heap allocation if PSRAM allocation fails.
 *
 * This allocator is particularly useful for ESP32 platforms with external PSRAM
 * where large buffers (like video frames) should be stored in PSRAM to preserve
 * internal SRAM for other uses. For small, frequently-accessed buffers that
 * need fast access, consider using RAMAllocatorH264 instead.
 *
 * @tparam T The type of objects to allocate
 *
 * @note On ESP-IDF platforms, uses heap_caps_aligned_calloc for PSRAM
 * allocation
 * @note On Arduino platforms, the heap_caps functions are shimmed to standard
 * malloc/free
 * @note Memory is zero-initialized (calloc behavior)
 * @note Provides proper alignment for the allocated type
 *
 * Example usage:
 * @code
 * #include "PSRAMAllocatorH264.h"
 *
 * // Use PSRAMAllocatorH264 with std::vector for large buffers
 * std::vector<uint8_t, PSRAMAllocatorH264<uint8_t>> largeBuffer;
 * largeBuffer.resize(1024 * 1024);  // Large buffer preferably in PSRAM
 *
 * // Or use the convenience alias
 * PSVec<uint8_t> videoFrame;    // Large video frame data in PSRAM
 * videoFrame.resize(640 * 480 * 3);
 *
 * // For small buffers in fast RAM, use RAMAllocatorH264 (separate header)
 * #include "RAMAllocatorH264.h"
 * RAMVec<uint16_t> lookupTable; // Small lookup table in fast RAM
 * @endcode
 */
template <typename T = uint8_t>
class PSRAMAllocatorH264 {
 public:
  /** @brief The type of objects this allocator can allocate */
  using value_type = T;

  /**
   * @brief Default constructor
   *
   * Constructs a PSRAMAllocatorH264. No initialization needed.
   */
  PSRAMAllocatorH264() noexcept {}

  /**
   * @brief Copy constructor from different allocator type
   *
   * Allows construction from PSRAMAllocatorH264 of different type U.
   * Required for STL allocator requirements.
   *
   * @tparam U The type parameter of the source allocator
   * @param other Source allocator (unused)
   */
  template <class U>
  PSRAMAllocatorH264(const PSRAMAllocatorH264<U>&) noexcept {}

  /**
   * @brief Allocate memory for n objects of type T
   *
   * Attempts to allocate memory in the following priority order:
   * 1. PSRAM using heap_caps_aligned_calloc (if available)
   * 2. Aligned allocation using aligned_alloc (C11)
   * 3. Standard calloc as fallback
   *
   * All allocated memory is zero-initialized.
   *
   * @param n Number of objects of type T to allocate
   * @return Pointer to allocated memory
   * @throws std::bad_alloc if allocation fails
   *
   * @note Memory is aligned by 16
   * @note On ESP32 with PSRAM, prefers external PSRAM allocation
   */
  T* allocate(std::size_t n) {
    if (n == 0) return nullptr;
    uint32_t actual_size = 0;
    void* p = esp_h264_aligned_calloc(
        16, 1, n * sizeof(T), &actual_size, (uint32_t)MALLOC_CAP_SPIRAM);
    assert(p != nullptr);
    return static_cast<T*>(p);
  }

  /**
   * @brief Deallocate memory previously allocated by this allocator
   *
   * Frees memory that was allocated by allocate(). Uses heap_caps_free
   * which works for both PSRAM and regular heap allocations.
   *
   * @param p Pointer to memory to deallocate (can be nullptr)
   * @param n Number of objects (ignored, but required by allocator interface)
   *
   * @note Safe to call with nullptr
   * @note The size parameter n is ignored as heap_caps_free doesn't need it
   */
  void deallocate(T* p, std::size_t /*n*/) noexcept {
    if (!p) return;
    heap_caps_free(p);
  }

  /**
   * @brief Rebind allocator to different type
   *
   * Provides the rebind mechanism required by STL allocators.
   * Allows containers to allocate different types using the same allocator.
   *
   * @tparam U The type to rebind to
   */
  template <class U>
  struct rebind {
    /** @brief The rebound allocator type */
    typedef PSRAMAllocatorH264<U> other;
  };
};

/**
 * @brief Convenience type alias for vectors using PSRAMAllocatorH264
 *
 * PSVec provides a shorter way to declare vectors that use PSRAMAllocatorH264.
 *
 * @tparam T The element type of the vector
 *
 * Example usage:
 * @code
 * PSVec<uint8_t> buffer;  // Instead of std::vector<uint8_t,
 * PSRAMAllocatorH264<uint8_t>> buffer.resize(1024);
 * @endcode
 */
template <typename T>
using PSVec = std::vector<T, PSRAMAllocatorH264<T>>;

}  // namespace esp_h264

/**
 * @brief Equality operator for PSRAMAllocatorH264 instances
 *
 * All PSRAMAllocatorH264 instances are considered equal regardless of their
 * template type parameter, as required by STL allocator requirements.
 *
 * @tparam T First allocator's value type
 * @tparam U Second allocator's value type
 * @param lhs First allocator
 * @param rhs Second allocator
 * @return true Always returns true
 */
template <class T, class U>
inline bool operator==(const esp_h264::PSRAMAllocatorH264<T>&,
                       const esp_h264::PSRAMAllocatorH264<U>&) noexcept {
  return true;
}

/**
 * @brief Inequality operator for PSRAMAllocatorH264 instances
 *
 * All PSRAMAllocatorH264 instances are considered equal, so inequality
 * always returns false.
 *
 * @tparam T First allocator's value type
 * @tparam U Second allocator's value type
 * @param lhs First allocator
 * @param rhs Second allocator
 * @return false Always returns false
 */
template <class T, class U>
inline bool operator!=(const esp_h264::PSRAMAllocatorH264<T>&,
                       const esp_h264::PSRAMAllocatorH264<U>&) noexcept {
  return false;
}
