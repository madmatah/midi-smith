#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::config {

namespace detail {

constexpr uint32_t kCrc32Polynomial = 0xEDB88320u;

constexpr auto GenerateCrc32Table() noexcept {
  struct Table {
    uint32_t entries[256]{};
  } table{};

  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t crc = i;
    for (uint32_t bit = 0; bit < 8; ++bit) {
      if (crc & 1u) {
        crc = (crc >> 1u) ^ kCrc32Polynomial;
      } else {
        crc >>= 1u;
      }
    }
    table.entries[i] = crc;
  }
  return table;
}

inline constexpr auto kCrc32Table = GenerateCrc32Table();

}  // namespace detail

constexpr uint32_t ComputeCrc32(const uint8_t* data, std::size_t length) noexcept {
  uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < length; ++i) {
    uint8_t index = static_cast<uint8_t>(crc ^ data[i]);
    crc = (crc >> 8u) ^ detail::kCrc32Table.entries[index];
  }
  return crc ^ 0xFFFFFFFFu;
}

}  // namespace midismith::config
