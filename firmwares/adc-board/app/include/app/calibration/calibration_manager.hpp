#pragma once

#include "app/analog/lookup_table_regeneration_requirements.hpp"
#include "app/calibration/calibration_apply_requirements.hpp"
#include "app/calibration/calibration_query_requirements.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationManager final : public CalibrationApplyRequirements,
                                 public CalibrationQueryRequirements {
 public:
  explicit CalibrationManager(
      midismith::adc_board::app::analog::LookupTableRegenerationRequirements&
          regeneration) noexcept;

  void ApplyCalibration(const SensorCalibrationArray& data) noexcept override;

  void ApplySensorCalibration(
      std::uint8_t sensor_index,
      const midismith::calibration::SensorCalibration& calibration) noexcept override;

  const midismith::calibration::SensorCalibration& sensor_calibration(
      std::uint8_t sensor_index) const noexcept override;

 private:
  midismith::adc_board::app::analog::LookupTableRegenerationRequirements& regeneration_;
  SensorCalibrationArray data_{};
};

}  // namespace midismith::adc_board::app::calibration
