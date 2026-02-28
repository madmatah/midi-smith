#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::byte_codec {

template <std::unsigned_integral T>
constexpr void WriteLittleEndian(std::span<std::uint8_t> buffer, std::size_t offset, T value) {
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    buffer[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
  }
}

template <std::unsigned_integral T>
constexpr T ReadLittleEndian(std::span<const std::uint8_t> buffer, std::size_t offset) {
  T result = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    result |= static_cast<T>(buffer[offset + i]) << (8 * i);
  }
  return result;
}

}  // namespace midismith::byte_codec
