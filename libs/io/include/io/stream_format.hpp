#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "io/stream_requirements.hpp"

namespace midismith::io {

inline void WriteUint32(WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buf[16]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

inline void WriteInt32(WritableStreamRequirements& out, std::int32_t value) noexcept {
  char buf[16]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

inline void WriteUint64(WritableStreamRequirements& out, std::uint64_t value) noexcept {
  char buf[32]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

inline void WriteInt64(WritableStreamRequirements& out, std::int64_t value) noexcept {
  char buf[32]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

inline void WriteUint8(WritableStreamRequirements& out, std::uint8_t value) noexcept {
  WriteUint32(out, value);
}

inline void WriteBool(WritableStreamRequirements& out, bool value) noexcept {
  out.Write(value ? "true" : "false");
}

namespace detail {

constexpr std::uint32_t PowerOfTen(std::uint8_t exponent) noexcept {
  std::uint32_t result = 1;
  for (std::uint8_t i = 0; i < exponent; ++i) {
    result *= 10;
  }
  return result;
}

}  // namespace detail

template <std::uint8_t kDecimalPlaces>
inline void WriteFloat(WritableStreamRequirements& out, float value) noexcept {
  constexpr std::uint32_t kMultiplier = detail::PowerOfTen(kDecimalPlaces);
  const auto integer_part = static_cast<std::uint32_t>(value);
  const auto frac_part = static_cast<std::uint32_t>(
      (value - static_cast<float>(integer_part)) * static_cast<float>(kMultiplier) + 0.5f);
  WriteUint32(out, integer_part);
  out.Write(".");
  std::uint32_t leading_zero_threshold = kMultiplier / 10;
  while (leading_zero_threshold > 1 && frac_part < leading_zero_threshold) {
    out.Write("0");
    leading_zero_threshold /= 10;
  }
  WriteUint32(out, frac_part);
}

}  // namespace midismith::io
