#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::checksum {

namespace detail {

inline constexpr std::uint32_t kReflectedPolynomial = 0xEDB88320u;
inline constexpr std::uint32_t kInitialRemainder = 0xFFFFFFFFu;
inline constexpr std::uint32_t kFinalXorValue = 0xFFFFFFFFu;
inline constexpr std::size_t kTableEntryCount = 256;
inline constexpr std::uint8_t kBitsPerByte = 8;

constexpr std::array<std::uint32_t, kTableEntryCount> GenerateTable() noexcept {
  std::array<std::uint32_t, kTableEntryCount> table{};
  for (std::size_t entry_index = 0; entry_index < kTableEntryCount; ++entry_index) {
    std::uint32_t remainder = static_cast<std::uint32_t>(entry_index);
    for (std::uint8_t bit_index = 0; bit_index < kBitsPerByte; ++bit_index) {
      const bool lowest_bit_is_set = (remainder & 1u) != 0u;
      remainder >>= 1u;
      if (lowest_bit_is_set) {
        remainder ^= kReflectedPolynomial;
      }
    }
    table[entry_index] = remainder;
  }
  return table;
}

inline constexpr std::array<std::uint32_t, kTableEntryCount> kTable = GenerateTable();

}  // namespace detail

class Crc32Accumulator {
 public:
  constexpr void Update(std::span<const std::uint8_t> data) noexcept {
    for (const std::uint8_t data_byte : data) {
      const auto table_index = static_cast<std::uint8_t>(remainder_ ^ data_byte);
      remainder_ = (remainder_ >> detail::kBitsPerByte) ^ detail::kTable[table_index];
    }
  }

  [[nodiscard]] constexpr std::uint32_t value() const noexcept {
    return remainder_ ^ detail::kFinalXorValue;
  }

  constexpr void Reset() noexcept {
    remainder_ = detail::kInitialRemainder;
  }

 private:
  std::uint32_t remainder_ = detail::kInitialRemainder;
};

[[nodiscard]] constexpr std::uint32_t ComputeCrc32(std::span<const std::uint8_t> data) noexcept {
  Crc32Accumulator accumulator;
  accumulator.Update(data);
  return accumulator.value();
}

[[nodiscard]] constexpr std::uint32_t ComputeCrc32(const std::uint8_t* data,
                                                   std::size_t length_bytes) noexcept {
  if (data == nullptr || length_bytes == 0) {
    return ComputeCrc32(std::span<const std::uint8_t>{});
  }
  return ComputeCrc32(std::span<const std::uint8_t>{data, length_bytes});
}

inline constexpr std::array<std::uint8_t, 9> kCheckVector = {'1', '2', '3', '4', '5',
                                                             '6', '7', '8', '9'};
inline constexpr std::uint32_t kCheckValue = 0xCBF43926u;

static_assert(ComputeCrc32(kCheckVector) == kCheckValue,
              "ComputeCrc32 must produce the CRC-32/ISO-HDLC check value: the variant Python's "
              "zlib.crc32 implements, that the firmware packaging tool writes into every image, "
              "and that already stamps every configuration record in flash");

}  // namespace midismith::checksum
