#pragma once

#include <array>
#include <cstddef>

#include "app/config/sensors.hpp"
#include "sensor-linearization/cny70_response_curve.hpp"
#include "sensor-linearization/sensor_calibration.hpp"
#include "sensor-linearization/sensor_response_curve.hpp"

namespace midismith::adc_board::app::config {

constexpr std::size_t kSensorLookupTableSize = 256u;

using SensorResponseCurveProvider =
    midismith::sensor_linearization::SensorResponseCurve (*)(void) noexcept;

// Configurable response curve used for LUT generation.
//
// Change this in config to select a different sensor response curve implementation.
inline constexpr SensorResponseCurveProvider kSensorResponseCurveProvider =
    midismith::sensor_linearization::Cny70DatasheetSensorResponseCurve;

inline constexpr midismith::sensor_linearization::SensorCalibration kDefaultSensorCalibration{
    .rest_current_ma = 0.127f,
    .strike_current_ma = 0.642f,
    .rest_distance_mm = 7.0f,
    .strike_distance_mm = 1.9f,
};

inline constexpr std::array<midismith::sensor_linearization::SensorCalibration,
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
