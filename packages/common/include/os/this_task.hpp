#pragma once

#include <cstdint>

namespace os {

class ThisTask {
 public:
  static std::uint32_t stack_high_water_mark_bytes() noexcept;
};

}  // namespace os

namespace midismith::common::os {
using ::os::ThisTask;
}
