#pragma once

#include <cstdint>

namespace app::analog {

struct SignalContext {
  std::uint32_t timestamp_ticks = 0;
  std::uint8_t sensor_id = 0;
};

}  // namespace app::analog
