#pragma once

#include <cstddef>

#include "sensor-linearization/cny70_response_curve.hpp"
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

}  // namespace midismith::adc_board::app::config
