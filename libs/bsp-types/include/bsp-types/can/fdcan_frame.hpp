#pragma once

#include <array>
#include <cstdint>

namespace midismith::bsp::can {

constexpr std::uint8_t kCanFdMaxDataBytes = 64;

struct FdcanFrame {
  std::uint32_t identifier;
  std::uint8_t data_length_bytes;
  std::array<std::uint8_t, kCanFdMaxDataBytes> data;
};

}  // namespace midismith::bsp::can
