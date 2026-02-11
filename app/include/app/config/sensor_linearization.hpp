#pragma once

#include <array>
#include <cstddef>

#include "app/config/sensors.hpp"
#include "domain/sensors/linearization/cny70_response_curve.hpp"
#include "domain/sensors/linearization/sensor_calibration.hpp"
#include "domain/sensors/linearization/sensor_response_curve.hpp"

namespace app::config {

constexpr std::size_t kSensorLookupTableSize = 256u;

using SensorResponseCurveProvider =
    domain::sensors::linearization::SensorResponseCurve (*)(void) noexcept;

// Configurable response curve used for LUT generation.
//
// Change this in config to select a different sensor response curve implementation.
inline constexpr SensorResponseCurveProvider kSensorResponseCurveProvider =
    domain::sensors::linearization::Cny70DatasheetSensorResponseCurve;

inline constexpr domain::sensors::linearization::SensorCalibration kDefaultSensorCalibration{
    .rest_current_ma = 0.045f,
    .strike_current_ma = 1.000f,
    .rest_distance_mm = 10.0f,
    .strike_distance_mm = 0.5f,
};

inline constexpr std::array<domain::sensors::linearization::SensorCalibration,
                            app::config_sensors::kSensorCount>
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

}  // namespace app::config
