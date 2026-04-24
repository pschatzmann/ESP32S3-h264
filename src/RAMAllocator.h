#pragma once

/**
 * @file RAMAllocator.h
 * @brief Header-only C++ allocator that uses internal RAM (DRAM) on ESP32
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
 * @class RAMAllocator
 * @brief STL-compatible allocator that uses internal RAM (DRAM) allocation
 *
 * RAMAllocator is a custom C++ allocator that allocates memory from internal
 * DRAM using heap_caps_aligned_calloc with MALLOC_CAP_INTERNAL. This ensures
 * allocation from fast internal memory rather than external PSRAM.
 *
 * This allocator is useful for ESP32 platforms when you specifically want
 * small, frequently-accessed buffers to be stored in fast internal RAM
 * rather than slower external PSRAM.
 *
 * @tparam T The type of objects to allocate
 *
 * @note On ESP-IDF platforms, uses heap_caps_aligned_calloc for internal RAM
 * allocation
 * @note On Arduino platforms, the heap_caps functions are shimmed to standard
 * malloc/free
 * @note Memory is zero-initialized (calloc behavior)
 * @note Provides proper alignment for the allocated type
 *
 * Example usage:
 * @code
 * #include "RAMAllocator.h"
 *
 * // Use with std::vector for small, fast buffers
 * std::vector<uint32_t, RAMAllocator<uint32_t>> smallBuffer;
 * smallBuffer.resize(256);  // Small buffer in fast internal RAM
 *
 * // Or use the convenience alias
 * RAMVec<uint32_t> fastBuffer;
 * fastBuffer.resize(256);
 * @endcode
 */
template <typename T = uint8_t>
class RAMAllocator {
 public:
  /** @brief The type of objects this allocator can allocate */
  using value_type = T;

  /**
   * @brief Default constructor
   *
   * Constructs a RAMAllocator. No initialization needed.
   */
  RAMAllocator() noexcept {}

  /**
   * @brief Copy constructor from different allocator type
   *
   * Allows construction from RAMAllocator of different type U.
   * Required for STL allocator requirements.
   *
   * @tparam U The type parameter of the source allocator
   * @param other Source allocator (unused)
   */
  template <class U>
  RAMAllocator(const RAMAllocator<U>&) noexcept {}

  /**
   * @brief Allocate memory for n objects of type T in internal RAM
   *
   * Attempts to allocate memory in the following priority order:
   * 1. Internal RAM using heap_caps_aligned_calloc (if available)
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
   * @note On ESP32, prefers internal DRAM allocation over PSRAM
   */
  T* allocate(std::size_t n) {
    if (n == 0) return nullptr;
    uint32_t actual_size = 0;
    void* p = esp_h264_aligned_calloc(
        16, 1, n * sizeof(T), &actual_size, (uint32_t)MALLOC_CAP_INTERNAL);
    return static_cast<T*>(p);
  }

  /**
   * @brief Deallocate memory previously allocated by this allocator
   *
   * Frees memory that was allocated by allocate(). Uses heap_caps_free
   * which works for both internal RAM and regular heap allocations.
   *
   * @param p Pointer to memory to deallocate (can be nullptr)
   * @param n Number of objects (ignored, but required by allocator interface)
   *
   * @note Safe to call with nullptr
   * @note The size parameter n is ignored as heap_caps_free doesn't need it
   */
  void deallocate(T* p, std::size_t /*n*/) noexcept {
    if (!p) return;
    esp_h264_free_internal(p);
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
    typedef RAMAllocator<U> other;
  };
};

/**
 * @brief Convenience type alias for vectors using RAMAllocator
 *
 * RAMVec provides a shorter way to declare vectors that use RAMAllocator.
 *
 * @tparam T The element type of the vector
 *
 * Example usage:
 * @code
 * RAMVec<uint32_t> buffer;  // Instead of std::vector<uint32_t,
 * RAMAllocator<uint32_t>> buffer.resize(256);
 * @endcode
 */
template <typename T>
using RAMVec = std::vector<T, RAMAllocator<T>>;

}  // namespace esp_h264

/**
 * @brief Equality operator for RAMAllocator instances
 *
 * All RAMAllocator instances are considered equal regardless of their
 * template type parameter, as required by STL allocator requirements.
 *
 * @tparam T First allocator's value type
 * @tparam U Second allocator's value type
 * @param lhs First allocator
 * @param rhs Second allocator
 * @return true Always returns true
 */
template <class T, class U>
inline bool operator==(const esp_h264::RAMAllocator<T>&,
                       const esp_h264::RAMAllocator<U>&) noexcept {
  return true;
}

/**
 * @brief Inequality operator for RAMAllocator instances
 *
 * All RAMAllocator instances are considered equal, so inequality
 * always returns false.
 *
 * @tparam T First allocator's value type
 * @tparam U Second allocator's value type
 * @param lhs First allocator
 * @param rhs Second allocator
 * @return false Always returns false
 */
template <class T, class U>
inline bool operator!=(const esp_h264::RAMAllocator<T>&,
                       const esp_h264::RAMAllocator<U>&) noexcept {
  return false;
}