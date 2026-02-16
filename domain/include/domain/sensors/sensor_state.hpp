#pragma once

#include <cstdint>

namespace domain::sensors {

struct SensorState {
  float last_current_ma = 0.0f;
  float last_filtered_adc_value = 0.0f;
  float last_shank_position_norm = 0.0f;
  float last_shank_position_mm = 0.0f;
  float last_processed_value = 0.0f;
  float last_shank_speed_m_per_s = 0.0f;
  float last_hammer_speed_m_per_s = 0.0f;
  std::uint32_t last_timestamp_ticks = 0;
  std::uint16_t last_raw_value = 0;
  std::uint8_t last_midi_velocity = 0;
  std::uint8_t id = 0;
  bool is_note_on = false;
};

}  // namespace domain::sensors
