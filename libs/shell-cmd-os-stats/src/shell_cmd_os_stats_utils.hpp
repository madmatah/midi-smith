#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "io/stream_format.hpp"
#include "io/stream_requirements.hpp"

namespace midismith::shell_cmd_os_stats {

inline std::string_view Arg(int argc, char** argv, int index) noexcept {
  if (argv == nullptr) {
    return {};
  }
  if (index < 0 || index >= argc) {
    return {};
  }
  if (argv[index] == nullptr) {
    return {};
  }
  return std::string_view(argv[index]);
}

inline bool ParseUint32(std::string_view text, std::uint32_t& out_value) noexcept {
  if (text.empty()) {
    return false;
  }
  std::uint32_t value = 0u;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return false;
  }
  out_value = value;
  return true;
}

inline void WritePermilleAsPercent(midismith::io::WritableStreamRequirements& out,
                                   std::uint32_t permille) noexcept {
  midismith::io::WriteUint32(out, permille / 10u);
  out.Write('.');
  midismith::io::WriteUint32(out, permille % 10u);
}

}  // namespace midismith::shell_cmd_os_stats
