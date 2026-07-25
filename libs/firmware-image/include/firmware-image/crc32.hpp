#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::firmware_image {

inline constexpr std::uint32_t kCrc32ReflectedPolynomial = 0xEDB88320u;
inline constexpr std::uint32_t kCrc32InitialRemainder = 0xFFFFFFFFu;
inline constexpr std::uint32_t kCrc32FinalXorValue = 0xFFFFFFFFu;
inline constexpr std::uint8_t kBitsPerByte = 8;

class Crc32Accumulator {
 public:
  constexpr void Update(std::span<const std::uint8_t> data) noexcept {
    for (const std::uint8_t data_byte : data) {
      remainder_ ^= data_byte;
      for (std::uint8_t bit_index = 0; bit_index < kBitsPerByte; ++bit_index) {
        const bool lowest_bit_is_set = (remainder_ & 1u) != 0u;
        remainder_ >>= 1u;
        if (lowest_bit_is_set) {
          remainder_ ^= kCrc32ReflectedPolynomial;
        }
      }
    }
  }

  [[nodiscard]] constexpr std::uint32_t value() const noexcept {
    return remainder_ ^ kCrc32FinalXorValue;
  }

  constexpr void Reset() noexcept {
    remainder_ = kCrc32InitialRemainder;
  }

 private:
  std::uint32_t remainder_ = kCrc32InitialRemainder;
};

[[nodiscard]] constexpr std::uint32_t ComputeCrc32(std::span<const std::uint8_t> data) noexcept {
  Crc32Accumulator accumulator;
  accumulator.Update(data);
  return accumulator.value();
}

inline constexpr std::array<std::uint8_t, 9> kCrc32CheckVector = {'1', '2', '3', '4', '5',
                                                                  '6', '7', '8', '9'};
inline constexpr std::uint32_t kCrc32CheckValue = 0xCBF43926u;

static_assert(ComputeCrc32(kCrc32CheckVector) == kCrc32CheckValue,
              "ComputeCrc32 must produce the CRC-32/ISO-HDLC check value, the variant Python's "
              "zlib.crc32 implements and the packaging tool relies on");

}  // namespace midismith::firmware_image
