#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor_state.hpp"

namespace midismith::adc_board::domain::sensors {

class SensorRegistry {
 public:
  SensorRegistry(SensorState* sensors, std::size_t sensor_count) noexcept
      : sensors_(sensors), sensor_count_(sensor_count) {
    if (sensors_ != nullptr) {
      for (std::size_t i = 0; i < sensor_count_; ++i) {
        assert(sensors_[i].id == static_cast<uint8_t>(i + 1) &&
               "Inconsistent SensorRegistry: IDs must be contiguous and start at 1");
      }
    }
  }

  SensorState* FindById(std::uint8_t id) noexcept {
    if (sensors_ == nullptr || id == 0 || id > sensor_count_) {
      return nullptr;
    }
    return &sensors_[static_cast<std::size_t>(id - 1u)];
  }

 private:
  SensorState* sensors_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace midismith::adc_board::domain::sensors
