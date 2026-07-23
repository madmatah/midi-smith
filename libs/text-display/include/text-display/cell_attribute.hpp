#pragma once

#include <cstdint>

namespace midismith::text_display {

enum class CellAttribute : std::uint8_t {
  kNormal,
  kHighlight,
  kDim,
};

}  // namespace midismith::text_display
