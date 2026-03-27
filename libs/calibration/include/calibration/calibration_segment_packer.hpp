#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "calibration/sensor_calibration.hpp"

namespace midismith::calibration {

template <std::size_t kCalibrationsPerSegmentCount>
class CalibrationSegmentPacker {
 public:
  static constexpr std::size_t kCalibrationsPerSegment = kCalibrationsPerSegmentCount;
  static constexpr std::size_t kCalibrationSizeBytes = sizeof(SensorCalibration);
  static constexpr std::size_t kSegmentPayloadSizeBytes =
      kCalibrationsPerSegment * kCalibrationSizeBytes;

  static void PackSegment(const SensorCalibration* source, std::size_t total_sensors,
                          std::size_t segment_index, std::uint8_t* payload_out) noexcept {
    if (source == nullptr || payload_out == nullptr) {
      return;
    }

    std::memset(payload_out, 0, kSegmentPayloadSizeBytes);
    const std::size_t first_sensor_index = segment_index * kCalibrationsPerSegment;
    for (std::size_t slot_index = 0; slot_index < kCalibrationsPerSegment; ++slot_index) {
      const std::size_t sensor_index = first_sensor_index + slot_index;
      if (sensor_index >= total_sensors) {
        break;
      }
      std::memcpy(payload_out + slot_index * kCalibrationSizeBytes, &source[sensor_index],
                  sizeof(SensorCalibration));
    }
  }

  static void UnpackSegment(const std::uint8_t* payload, std::size_t segment_index,
                            SensorCalibration* destination, std::size_t max_sensors) noexcept {
    if (payload == nullptr || destination == nullptr) {
      return;
    }

    const std::size_t first_sensor_index = segment_index * kCalibrationsPerSegment;
    for (std::size_t slot_index = 0; slot_index < kCalibrationsPerSegment; ++slot_index) {
      const std::size_t sensor_index = first_sensor_index + slot_index;
      if (sensor_index >= max_sensors) {
        break;
      }
      std::memcpy(&destination[sensor_index], payload + slot_index * kCalibrationSizeBytes,
                  sizeof(SensorCalibration));
    }
  }

  static constexpr std::size_t ComputeTotalSegments(std::size_t sensor_count) noexcept {
    return (sensor_count + kCalibrationsPerSegment - 1u) / kCalibrationsPerSegment;
  }
};

}  // namespace midismith::calibration
