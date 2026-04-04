#pragma once

#include <array>

#include "app/config/sensors.hpp"
#include "calibration/sensor_calibration.hpp"

namespace midismith::adc_board::app::config {

// Minimum delta (strike - rest) to consider a calibration coherent.
// Below this threshold, the sensor is assumed unconnected — ADC noise only.
constexpr float kMinimumCalibrationDeltaMa = 0.05f;

// Maximum valid strike current: Vref / Rf = 2048 mV / 1800 Ω ≈ 1.138 mA
constexpr float kMaxValidStrikeCurrentMa = 1.138f;

inline constexpr midismith::calibration::SensorCalibration kDefaultSensorCalibration{
    .rest_current_ma = 0.127f,
    .strike_current_ma = 0.642f,
    .rest_distance_mm = 7.0f,
    .strike_distance_mm = 1.9f,
};

inline constexpr std::array<midismith::calibration::SensorCalibration,
                            midismith::adc_board::app::config::sensors::kSensorCount>
    kSensorCalibrationByIndex = {
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration, kDefaultSensorCalibration, kDefaultSensorCalibration,
        kDefaultSensorCalibration,
};

}  // namespace midismith::adc_board::app::config
