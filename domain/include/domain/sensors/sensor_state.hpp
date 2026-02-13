#pragma once

#include <cstdint>

namespace domain::sensors {

struct SensorState {
  float last_current_ma = 0.0f;
  float last_filtered_adc_value = 0.0f;
  float last_normalized_position = 0.0f;
  float last_processed_value = 0.0f;
  std::uint32_t last_timestamp_ticks = 0;
  std::uint16_t last_raw_value = 0;
  std::uint8_t id = 0;
};

}  // namespace domain::sensors
