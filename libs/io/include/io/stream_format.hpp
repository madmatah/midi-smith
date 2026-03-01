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

inline void WriteUint64(WritableStreamRequirements& out, std::uint64_t value) noexcept {
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
  out.Write(value ? '1' : '0');
}

}  // namespace midismith::io
