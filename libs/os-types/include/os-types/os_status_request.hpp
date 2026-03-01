#pragma once

#include <cstdint>

namespace midismith::os {

struct OsStatusRequest {
  std::uint32_t window_ms{0u};
};

}  // namespace midismith::os
