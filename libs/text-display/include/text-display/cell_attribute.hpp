#pragma once

#include <cstdint>

namespace midismith::text_display {

enum class CellAttribute : std::uint8_t {
  kNormal,
  kHighlight,
  kDim,
  kTitle,
  kAccent,
  kSuccess,
  kWarning,
  kError,
  kFooter,
};

}  // namespace midismith::text_display
