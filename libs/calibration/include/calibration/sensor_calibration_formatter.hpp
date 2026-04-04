#pragma once

#include <cstdint>

#include "calibration/sensor_calibration.hpp"
#include "io/stream_format.hpp"
#include "io/stream_requirements.hpp"

namespace midismith::calibration {

inline void WriteSensorCalibrationLine(midismith::io::WritableStreamRequirements& out,
                                       std::uint8_t sensor_id,
                                       const SensorCalibration& calibration) noexcept {
  out.Write("sensor[");
  midismith::io::WriteUint8(out, sensor_id);
  out.Write("]  rest_mA=");
  midismith::io::WriteFloat<3>(out, calibration.rest_current_ma);
  out.Write("  strike_mA=");
  midismith::io::WriteFloat<3>(out, calibration.strike_current_ma);
  out.Write("  rest_mm=");
  midismith::io::WriteFloat<3>(out, calibration.rest_distance_mm);
  out.Write("  strike_mm=");
  midismith::io::WriteFloat<3>(out, calibration.strike_distance_mm);
  out.Write("\r\n");
}

}  // namespace midismith::calibration
