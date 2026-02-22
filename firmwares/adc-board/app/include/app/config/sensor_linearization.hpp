#pragma once

#include <array>
#include <cstddef>

#include "app/config/sensors.hpp"
#include "domain/sensors/linearization/cny70_response_curve.hpp"
#include "domain/sensors/linearization/sensor_calibration.hpp"
#include "domain/sensors/linearization/sensor_response_curve.hpp"

namespace midismith::adc_board::app::config {

constexpr std::size_t kSensorLookupTableSize = 256u;

using SensorResponseCurveProvider =
    midismith::adc_board::domain::sensors::linearization::SensorResponseCurve (*)(void) noexcept;

// Configurable response curve used for LUT generation.
//
// Change this in config to select a different sensor response curve implementation.
inline constexpr SensorResponseCurveProvider kSensorResponseCurveProvider =
    midismith::adc_board::domain::sensors::linearization::Cny70DatasheetSensorResponseCurve;

inline constexpr midismith::adc_board::domain::sensors::linearization::SensorCalibration
    kDefaultSensorCalibration{
        .rest_current_ma = 0.127f,
        .strike_current_ma = 0.642f,
        .rest_distance_mm = 7.0f,
        .strike_distance_mm = 1.9f,
    };

inline constexpr std::array<midismith::adc_board::domain::sensors::linearization::SensorCalibration,
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
