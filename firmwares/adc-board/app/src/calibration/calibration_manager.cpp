#include "app/calibration/calibration_manager.hpp"

namespace midismith::adc_board::app::calibration {

CalibrationManager::CalibrationManager(
    midismith::adc_board::app::analog::LookupTableRegenerationRequirements& regeneration) noexcept
    : regeneration_(regeneration) {}

void CalibrationManager::ApplyCalibration(const SensorCalibrationArray& data) noexcept {
  data_ = data;
  regeneration_.RegenerateAll(data_);
}

void CalibrationManager::ApplySensorCalibration(
    std::uint8_t sensor_index,
    const midismith::calibration::SensorCalibration& calibration) noexcept {
  data_[sensor_index] = calibration;
  regeneration_.RegenerateSensor(sensor_index, calibration);
}

const midismith::calibration::SensorCalibration& CalibrationManager::sensor_calibration(
    std::uint8_t sensor_index) const noexcept {
  return data_[sensor_index];
}

}  // namespace midismith::adc_board::app::calibration
